/*
   +----------------------------------------------------------------------+
   | PHP Version 7, 8                                                     |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jouni Ahto <jouni.ahto@exdec.fi>                            |
   |          Andrew Avdeev <andy@simgts.mv.ru>                           |
   |          Ard Biesheuvel <a.k.biesheuvel@its.tudelft.nl>              |
   |          Martin Koeditz <martin.koeditz@it-syn.de>                   |
   |          others                                                      |
   +----------------------------------------------------------------------+
   | You'll find history on Github                                        |
   | https://github.com/FirebirdSQL/php-firebird/commits/master           |
   +----------------------------------------------------------------------+
 */

#include <firebird/fb_c_api.h>
#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"

#include "blob.h"
#include "fbp_statement.h"

enum execute_fn {
    EXECUTE,
    EXECUTE2,
    EXECUTE_IMMEDIATE,
};

void fbp_statement_ctor(firebird_stmt *stmt, firebird_trans *tr)
{
    stmt->stmt_handle = 0;
    stmt->db_handle = tr->db_handle;
    stmt->tr_handle = &tr->tr_handle;
}

// TODO: move xsqlda functions into its own module or utils
void fbp_alloc_xsqlda(XSQLDA *sqlda)
{
    int i;

    for (i = 0; i < sqlda->sqld; i++) {
        XSQLVAR *var = &sqlda->sqlvar[i];

        switch (var->sqltype & ~1) {
            case SQL_TEXT:
                var->sqldata = safe_emalloc(sizeof(char), var->sqllen, 0);
                break;
            case SQL_VARYING:
                var->sqldata = safe_emalloc(sizeof(char), var->sqllen + sizeof(short), 0);
                break;
#ifdef SQL_BOOLEAN
            case SQL_BOOLEAN:
                var->sqldata = emalloc(sizeof(FB_BOOLEAN));
                break;
#endif
            case SQL_SHORT:
                var->sqldata = emalloc(sizeof(short));
                break;
            case SQL_LONG:
                var->sqldata = emalloc(sizeof(ISC_LONG));
                break;
            case SQL_FLOAT:
                var->sqldata = emalloc(sizeof(float));
                    break;
            case SQL_DOUBLE:
                var->sqldata = emalloc(sizeof(double));
                break;
            case SQL_INT64:
                var->sqldata = emalloc(sizeof(ISC_INT64));
                break;
            case SQL_TIMESTAMP:
                var->sqldata = emalloc(sizeof(ISC_TIMESTAMP));
                break;
            case SQL_TYPE_DATE:
                var->sqldata = emalloc(sizeof(ISC_DATE));
                break;
            case SQL_TYPE_TIME:
                var->sqldata = emalloc(sizeof(ISC_TIME));
                break;
            case SQL_ARRAY:
                fbp_fatal("ARRAY type is not supported.");
                break;
            case SQL_BLOB:
                var->sqldata = emalloc(sizeof(ISC_QUAD));
                break;
#if FB_API_VER >= 40
            // These are converted to VARCHAR via isc_dpb_set_bind tag at connect
            // case SQL_DEC16:
            // case SQL_DEC34:
            // case SQL_INT128:
            case SQL_TIMESTAMP_TZ:
                var->sqldata = emalloc(sizeof(ISC_TIMESTAMP_TZ));
                break;
            case SQL_TIME_TZ:
                var->sqldata = emalloc(sizeof(ISC_TIME_TZ));
                break;
#endif
            default:
                php_error(E_WARNING, "Unhandled sqltype: %d for sqlname %s %s:%d", var->sqltype, var->sqlname, __FILE__, __LINE__);
                break;
        } /* switch */

        // XXX: Engine should allocate this for outbound XSQLDA? No?
        if (var->sqltype & 1) { /* sql NULL flag */
            var->sqlind = emalloc(sizeof(short));
        } else {
            var->sqlind = NULL;
        }
    } /* for */
}

void fbp_free_xsqlda(XSQLDA *sqlda)
{
    int i;
    XSQLVAR *var;

    if (sqlda) {
        var = sqlda->sqlvar;
        for (i = 0; i < min(sqlda->sqld, sqlda->sqln); i++, var++) {
            efree(var->sqldata);
            if (var->sqlind) { // XXX: should free for out sqlda or not?
                efree(var->sqlind);
            }
        }
        efree(sqlda);
    }
}

int fbp_statement_bind(firebird_stmt *stmt, XSQLDA *sqlda, zval *b_vars)
{
    int i, rv = SUCCESS;

    if(sqlda->sqld > 0) {
        // In case of repeated calls to execute()
        if(!stmt->bind_buf) {
            stmt->bind_buf = safe_emalloc(sizeof(firebird_bind_buf), stmt->in_sqlda->sqld, 0);
        }
    }

    for (i = 0; i < sqlda->sqld; ++i) {
        zval *b_var = &b_vars[i];
        XSQLVAR *var = &sqlda->sqlvar[i];

        var->sqlind = &stmt->bind_buf[i].sqlind;

        if (Z_TYPE_P(b_var) == IS_NULL) {
            stmt->bind_buf[i].sqlind = -1;
            sqlda->sqlvar->sqldata = NULL;
            continue;
        }

        stmt->bind_buf[i].sqlind = 0;

        var->sqldata = (void*)&stmt->bind_buf[i].val;

        switch (var->sqltype & ~1) {
            struct tm t;

            case SQL_TIMESTAMP:
            // TODO: case SQL_TIMESTAMP_TZ:
            // TODO: case SQL_TIME_TZ:
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
                if (Z_TYPE_P(b_var) == IS_LONG) {
                    struct tm *res;
                    res = php_gmtime_r(&Z_LVAL_P(b_var), &t);
                    if (!res) {
                        return FAILURE;
                    }
                } else {
#ifdef HAVE_STRPTIME
                    char *format = INI_STR("firebird.timestampformat");

                    convert_to_string(b_var);

                    switch (var->sqltype & ~1) {
                        case SQL_TYPE_DATE:
                            format = INI_STR("firebird.dateformat");
                            break;
                        case SQL_TYPE_TIME:
                        // TODO: case SQL_TIME_TZ:
                            format = INI_STR("firebird.timeformat");
                    }
                    if (!strptime(Z_STRVAL_P(b_var), format, &t)) {
                        /* strptime() cannot handle it, so let IB have a try */
                        break;
                    }
#else /* ifndef HAVE_STRPTIME */
                    break; /* let IB parse it as a string */
#endif
                }

                switch (var->sqltype & ~1) {
                    default: /* == case SQL_TIMESTAMP */
                        isc_encode_timestamp(&t, &stmt->bind_buf[i].val.tsval);
                        break;
                    case SQL_TYPE_DATE:
                        isc_encode_sql_date(&t, &stmt->bind_buf[i].val.dtval);
                        break;
                    case SQL_TYPE_TIME:
                    // TODO: case SQL_TIME_TZ:
                        isc_encode_sql_time(&t, &stmt->bind_buf[i].val.tmval);
                        break;
                }
                continue;

            case SQL_BLOB:
                ISC_QUAD bl_id;
                if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_Id_ce) {
                    bl_id = get_firebird_blob_id_from_zval(b_var)->bl_id;
                } else if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_ce) {
                    bl_id = get_firebird_blob_from_zval(b_var)->bl_id;
                } else {
                    convert_to_string(b_var);

                    firebird_blob blob = {0};
                    fbp_blob_ctor(&blob, stmt->db_handle, stmt->tr_handle);

                    if (fbp_blob_create(&blob) ||
                        fbp_blob_put(&blob, Z_STRVAL_P(b_var), Z_STRLEN_P(b_var)) ||
                        fbp_blob_close(&blob)) {
                            return FAILURE;
                    }

                    bl_id = blob.bl_id;
                }
                stmt->bind_buf[i].val.qval = bl_id;
                continue;
#ifdef SQL_BOOLEAN
            case SQL_BOOLEAN:

                switch (Z_TYPE_P(b_var)) {
                    case IS_LONG:
                    case IS_DOUBLE:
                    case IS_TRUE:
                    case IS_FALSE:
                        *(FB_BOOLEAN *)var->sqldata = zend_is_true(b_var) ? FB_TRUE : FB_FALSE;
                        break;
                    case IS_STRING:
                    {
                        zend_long lval;
                        double dval;

                        if ((Z_STRLEN_P(b_var) == 0)) {
                            *(FB_BOOLEAN *)var->sqldata = FB_FALSE;
                            break;
                        }

                        switch (is_numeric_string(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), &lval, &dval, 0)) {
                            case IS_LONG:
                                *(FB_BOOLEAN *)var->sqldata = (lval != 0) ? FB_TRUE : FB_FALSE;
                                break;
                            case IS_DOUBLE:
                                *(FB_BOOLEAN *)var->sqldata = (dval != 0) ? FB_TRUE : FB_FALSE;
                                break;
                            default:
                                if (!zend_binary_strncasecmp(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), "true", 4, 4)) {
                                    *(FB_BOOLEAN *)var->sqldata = FB_TRUE;
                                } else if (!zend_binary_strncasecmp(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), "false", 5, 5)) {
                                    *(FB_BOOLEAN *)var->sqldata = FB_FALSE;
                                } else {
                                    fbp_error("Parameter %d: cannot convert string to boolean", i+1);
                                    rv = FAILURE;
                                    continue;
                                }
                        }
                        break;
                    }
                    case IS_NULL:
                        stmt->bind_buf[i].sqlind = -1;
                        break;
                    default:
                        fbp_error("Parameter %d: must be boolean", i+1);
                        rv = FAILURE;
                        continue;
                }
                var->sqltype = SQL_BOOLEAN;
                continue;
#endif
            case SQL_ARRAY:
                fbp_fatal("ARRAY type is not supported.");
                continue;
        } /* switch */

        /* we end up here if none of the switch cases handled the field */
        convert_to_string(b_var);
        var->sqldata = Z_STRVAL_P(b_var);
        var->sqllen	 = Z_STRLEN_P(b_var);
        var->sqltype = SQL_TEXT;
    } /* for */
    return rv;
}

int fbp_update_statement_info(firebird_stmt *stmt)
{
    char request_buffer[] = { isc_info_sql_records };
    char info_buffer[64] = { 0 };

    // isc_info_req_select_count - It tracks the number of rows fetched by a running request (not a SELECT statement).
    stmt->insert_count = 0;
    stmt->update_count = 0;
    stmt->delete_count = 0;
    stmt->affected_count = 0;

    if (isc_dsql_sql_info(FBG(status), &stmt->stmt_handle, sizeof(request_buffer), request_buffer, sizeof(info_buffer), info_buffer)) {
        return FAILURE;
    }

    const ISC_UCHAR* p = info_buffer + 3;

    FBDEBUG("Parsing SQL info buffer");

    if (info_buffer[0] == isc_info_sql_records)
    {
        while ((*p != isc_info_end) && (p - (const ISC_UCHAR *)&info_buffer < sizeof(info_buffer)))
        {
            const ISC_UCHAR count_is = *p++;
            const ISC_SHORT len = isc_portable_integer(p, 2); p += 2;
            const ISC_ULONG count = isc_portable_integer(p, len); p += len;
            switch(count_is) {
                case isc_info_req_insert_count: stmt->insert_count += count; break;
                case isc_info_req_update_count: stmt->update_count += count; break;
                case isc_info_req_delete_count: stmt->delete_count += count; break;
                case isc_info_req_select_count: continue;
                default: {
                    fbp_fatal("BUG: unrecognized isc_dsql_sql_info item: %d with value: %d", count_is, count);
                } break;
            }
        }
    } else {
        fbp_fatal("Unexpected isc_dsql_sql_info response: %d", info_buffer[0]);
    }

    stmt->affected_count = stmt->insert_count + stmt->update_count + stmt->delete_count;

    FBDEBUG_NOFL(" insert_count: %zu", stmt->insert_count);
    FBDEBUG_NOFL(" update_count: %zu", stmt->update_count);
    FBDEBUG_NOFL(" delete_count: %zu", stmt->delete_count);

    return SUCCESS;
}

// int fbp_statement_execute(zval *stmt_o, zval *bind_args, uint32_t num_bind_args, zend_class_entry *ce, zval *ce_o)
int fbp_statement_execute(firebird_stmt *stmt, zval *bind_args, uint32_t num_bind_args)
{
    /* has placeholders */
    if (stmt->in_sqlda->sqld > 0) {
        if (fbp_statement_bind(stmt, stmt->in_sqlda, bind_args)) {
            return FAILURE;
        }
    }

    // ### isc_dsql_exec_immed2
        // isc_dsql_exec_immed2() prepares the DSQL statement specified in
        // statement, executes it once, and discards it. statement can return a
        // single set of values (i.e, it can be an EXECUTE PROCEDURE or singleton
        // SELECT) in the output XSQLDA.

        // If statement requires input parameter values (that is, if it contains
        // parameter ­markers), these values must be supplied in the input XSQLDA,
        // in_xsqlda.

        // For statements that return multiple rows of data, use isc_dsql_prepare(),
        // ­isc_dsql_execute2(), and isc_dsql_fetch().

    // ### ­isc_dsql_execute_immediate
        // To execute a statement that does not return any data a single time, call
        // ­isc_dsql_execute_immediate() instead of isc_dsql_prepare() and
        // ­isc_dsql_execute2().

        // isc_dsql_execute_immediate() prepares the DSQL statement specified in
        // <­statement>, executes it once, and discards it. The statement must not
        // be one that returns data (that is, it must not be a SELECT or EXECUTE
        // PROCEDURE ­statement).

        // If <statement> requires input parameter values (that is, if it
        // contains parameter markers), these values must be supplied in the
        // input XSQLDA, xsqlda.

        // To create a database using isc_dsql_execute_immediate(), supply a
        // CREATE DATABASE statement and have db_handle and trans_handle point
        // to handles with a NULL value.

    enum execute_fn exfn = EXECUTE;

    switch(stmt->statement_type) {
        case isc_info_sql_stmt_select:
        case isc_info_sql_stmt_select_for_upd:
            exfn = EXECUTE;
            break;

        case isc_info_sql_stmt_exec_procedure:
        case isc_info_sql_stmt_insert:
        case isc_info_sql_stmt_update:
        case isc_info_sql_stmt_delete:
            exfn = EXECUTE2;
            break;

        case isc_info_sql_stmt_ddl:
        case isc_info_sql_stmt_start_trans: // TODO: warn/throw about already started transaction
        case isc_info_sql_stmt_commit:
        case isc_info_sql_stmt_rollback:
        case isc_info_sql_stmt_savepoint:
        case isc_info_sql_stmt_set_generator:
            exfn = EXECUTE_IMMEDIATE;
            break;

        // Seems that these are not used anymore
        case isc_info_sql_stmt_get_segment:
        case isc_info_sql_stmt_put_segment:
            break;
        default:
            fbp_fatal("BUG: Unrecognized stmt->statement_type: %d. Possibly a new server feature.", stmt->statement_type);
            break;
    }

    ISC_STATUS isc_result;
    if (exfn == EXECUTE_IMMEDIATE) {
        FBDEBUG("isc_dsql_execute_immediate(): handle=%d, type=%d", stmt->stmt_handle, stmt->statement_type);
        if (isc_dsql_free_statement(FBG(status), &stmt->stmt_handle, DSQL_drop)) {
            return FAILURE;
        }
        isc_result = isc_dsql_execute_immediate(FBG(status), stmt->db_handle, stmt->tr_handle, 0, stmt->query, SQL_DIALECT_CURRENT, stmt->in_sqlda);
    } else if (exfn == EXECUTE2) {
        FBDEBUG("isc_dsql_execute2(): handle=%d, type=%d", stmt->stmt_handle, stmt->statement_type);
        isc_result = isc_dsql_execute2(FBG(status), stmt->tr_handle, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda, stmt->out_sqlda);
    } else {
        FBDEBUG("isc_dsql_execute(): handle=%d, type=%d", stmt->stmt_handle, stmt->statement_type);
        isc_result = isc_dsql_execute(FBG(status), stmt->tr_handle, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda);
    }

    if (isc_result) {
        return FAILURE;
    }

    stmt->has_more_rows = stmt->out_sqlda->sqld > 0;

    fbp_update_statement_info(stmt);

    return SUCCESS;
}

// int statement_prepare(ISC_STATUS_ARRAY status, zval *stmt_o, const ISC_SCHAR *sql)
int fbp_statement_prepare(firebird_stmt *stmt, const ISC_SCHAR *sql)
{
    // TODO: configure auto-close, throw or warning
    // TODO: if (stmt->stmt_handle > 0) {
    //     // Unreachable, unless a BUG
    //     zend_throw_exception_ex(spl_ce_RuntimeException, 0, "Statement already active");
    //     return FAILURE;
    // }

    if (isc_dsql_allocate_statement(FBG(status), stmt->db_handle, &stmt->stmt_handle)) {
        return FAILURE;
    }

    stmt->out_sqlda = (XSQLDA *) emalloc(XSQLDA_LENGTH(1));
    stmt->out_sqlda->sqln = 1;
    stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;

    if (isc_dsql_prepare(FBG(status), stmt->tr_handle, &stmt->stmt_handle, 0, sql, SQL_DIALECT_CURRENT, stmt->out_sqlda)) {
        return FAILURE;
    }

    if (stmt->out_sqlda->sqld > stmt->out_sqlda->sqln) {
        stmt->out_sqlda = erealloc(stmt->out_sqlda, XSQLDA_LENGTH(stmt->out_sqlda->sqld));
        stmt->out_sqlda->sqln = stmt->out_sqlda->sqld;
        stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;
        if (isc_dsql_describe(FBG(status), &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->out_sqlda)) {
            return FAILURE;
        }
    }

    fbp_alloc_xsqlda(stmt->out_sqlda);

    /* maybe have input placeholders? */
    stmt->in_sqlda = emalloc(XSQLDA_LENGTH(1));
    stmt->in_sqlda->sqln = 1;
    stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;
    if (isc_dsql_describe_bind(FBG(status), &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
        return FAILURE;
    }

    /* not enough input variables ? */
    if (stmt->in_sqlda->sqld > stmt->in_sqlda->sqln) {
        stmt->in_sqlda = erealloc(stmt->in_sqlda, XSQLDA_LENGTH(stmt->in_sqlda->sqld));
        stmt->in_sqlda->sqln = stmt->in_sqlda->sqld;
        stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;

        if (isc_dsql_describe_bind(FBG(status), &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
            return FAILURE;
        }
    }

    static char info_type[] = { isc_info_sql_stmt_type };
    char result[8];

    /* find out what kind of statement was prepared */
    if (isc_dsql_sql_info(FBG(status), &stmt->stmt_handle, sizeof(info_type), info_type, sizeof(result), result)) {
        return FAILURE;
    }

    // Do we need isc_portable_integer or accessing directly result[3] is fine?
    // int len = isc_portable_integer(&result[1], 2);
    // int st = isc_portable_integer(&result[3], len);
    stmt->statement_type = result[3];

    stmt->query = sql; // TODO: copy?

    return SUCCESS;
}

void fbp_statement_free(firebird_stmt *s)
{
    if (s->in_sqlda) {
        efree(s->in_sqlda);
    }
    if (s->out_sqlda) {
        fbp_free_xsqlda(s->out_sqlda);
    }
    // if (s->in_array) {
    //     efree(s->in_array);
    // }
    // if (s->out_array) {
    //     efree(s->out_array);
    // }
    if(s->bind_buf) {
        efree(s->bind_buf);
    }
}
