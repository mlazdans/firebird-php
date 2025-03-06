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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#if HAVE_FIREBIRD

#include "php_ini.h"
#include "zend_exceptions.h"
#include "ext/standard/php_standard.h"
// #include "ext/standard/md5.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"
#include "SAPI.h"
#include "zend_interfaces.h"

#include <time.h>

#define ROLLBACK    0
#define COMMIT      1
#define RETAIN      2

#define CHECK_LINK(link) { if (link==NULL) { php_error_docref(NULL, E_WARNING, "A link to the server could not be established"); RETURN_FALSE; } }

ZEND_DECLARE_MODULE_GLOBALS(firebird)
static PHP_GINIT_FUNCTION(firebird);

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_firebird_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_firebird_construct, 0, 0, 0)
    ZEND_ARG_TYPE_INFO(0, database, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, username, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, password, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, charset, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, buffers, IS_LONG, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, dialect, IS_LONG, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, role, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

/* {{{ extension definition structures */
static const zend_function_entry firebird_functions[] = {
    PHP_FE_END
};

zend_module_entry firebird_module_entry = {
    STANDARD_MODULE_HEADER,
    "firebird",
    firebird_functions,
    PHP_MINIT(firebird),
    PHP_MSHUTDOWN(firebird),
    NULL,
    PHP_RSHUTDOWN(firebird),
    PHP_MINFO(firebird),
    PHP_FIREBIRD_VERSION,
    PHP_MODULE_GLOBALS(firebird),
    PHP_GINIT(firebird),
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_FIREBIRD
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(firebird)
#endif

/* True globals, no need for thread safety */
// int le_link, le_plink, le_trans;

/* }}} */

/* error handling ---------------------------- */

/* {{{ proto string firebird_errmsg(void)
   Return error message */
// PHP_FUNCTION(firebird_errmsg)
// {
//     if (zend_parse_parameters_none() == FAILURE) {
//         return;
//     }

//     if (IBG(sql_code) != 0) {
//         RETURN_STRING(IBG(errmsg));
//     }

//     RETURN_FALSE;
// }
/* }}} */

/* {{{ proto int firebird_errcode(void)
   Return error code */
// PHP_FUNCTION(firebird_errcode)
// {
//     if (zend_parse_parameters_none() == FAILURE) {
//         return;
//     }

//     if (IBG(sql_code) != 0) {
//         RETURN_LONG(IBG(sql_code));
//     }
//     RETURN_FALSE;
// }
/* }}} */

/* print interbase error and save it for firebird_errmsg() */
// void _php_firebird_error(void) /* {{{ */
// {
// 	char *s = IBG(errmsg);
// 	const ISC_STATUS *statusp = IB_STATUS;

// 	IBG(sql_code) = isc_sqlcode(IB_STATUS);

// 	while ((s - IBG(errmsg)) < MAX_ERRMSG && fb_interpret(s, MAX_ERRMSG - strlen(IBG(errmsg)) - 1, &statusp)) {
// 		strcat(IBG(errmsg), " ");
// 		s = IBG(errmsg) + strlen(IBG(errmsg));
// 	}

/* print php interbase module error and save it for firebird_errmsg() */
// void _php_firebird_module_error(char *msg, ...) /* {{{ */
// {
// 	va_list ap;

// 	va_start(ap, msg);

// 	/* vsnprintf NUL terminates the buf and writes at most n-1 chars+NUL */
// 	vsnprintf(IBG(errmsg), MAX_ERRMSG, msg, ap);
// 	va_end(ap);

// 	IBG(sql_code) = -999; /* no SQL error */

// 	php_error_docref(NULL, E_WARNING, "%s", IBG(errmsg));
// }
/* }}} */

/* {{{ internal macros, functions and structures */
typedef struct {
    isc_db_handle *db_ptr;
    zend_long tpb_len;
    char *tpb_ptr;
} ISC_TEB;

/* }}} */

/* Fill ib_link and trans with the correct database link and transaction. */
// void _php_firebird_get_link_trans(INTERNAL_FUNCTION_PARAMETERS, /* {{{ */
// 	zval *link_id, firebird_db_link **ib_link, firebird_trans **trans)
// {
// 	IBDEBUG("Transaction or database link?");
// 	if (Z_RES_P(link_id)->type == le_trans) {
// 		/* Transaction resource: make sure it refers to one link only, then
// 		   fetch it; database link is stored in ib_trans->db_link[]. */
// 		IBDEBUG("Type is le_trans");
// 		*trans = (firebird_trans *)zend_fetch_resource_ex(link_id, LE_TRANS, le_trans);
// 		if ((*trans)->link_cnt > 1) {
// 			_php_firebird_module_error("Link id is ambiguous: transaction spans multiple connections."
// 				);
// 			return;
// 		}
// 		*ib_link = (*trans)->db_link[0];
// 		return;
// 	}
// 	IBDEBUG("Type is le_[p]link or id not found");
// 	/* Database link resource, use default transaction. */
// 	*trans = NULL;
// 	*ib_link = (firebird_db_link *)zend_fetch_resource2_ex(link_id, LE_LINK, le_link, le_plink);
// }
/* }}} */

/* destructors ---------------------- */

// static void _php_firebird_commit_link(firebird_db_link *link) /* {{{ */
// {
// 	unsigned short i = 0, j;
// 	firebird_tr_list *l;
// 	firebird_event *e;
// 	IBDEBUG("Checking transactions to close...");

// 	for (l = link->tr_list; l != NULL; ++i) {
// 		firebird_tr_list *p = l;
// 		if (p->trans != 0) {
// 			if (i == 0) {
// 				if (p->trans->handle != 0) {
// 					IBDEBUG("Committing default transaction...");
// 					if (isc_commit_transaction(IB_STATUS, &p->trans->handle)) {
// 						_php_firebird_error();
// 					}
// 				}
// 				efree(p->trans); /* default transaction is not a registered resource: clean up */
// 			} else {
// 				if (p->trans->handle != 0) {
// 					/* non-default trans might have been rolled back by other call of this dtor */
// 					IBDEBUG("Rolling back other transactions...");
// 					if (isc_rollback_transaction(IB_STATUS, &p->trans->handle)) {
// 						_php_firebird_error();
// 					}
// 				}
// 				/* set this link pointer to NULL in the transaction */
// 				for (j = 0; j < p->trans->link_cnt; ++j) {
// 					if (p->trans->db_link[j] == link) {
// 						p->trans->db_link[j] = NULL;
// 						break;
// 					}
// 				}
// 			}
// 		}
// 		l = l->next;
// 		efree(p);
// 	}
// 	link->tr_list = NULL;

// 	for (e = link->event_head; e; e = e->event_next) {
// 		_php_firebird_free_event(e);
// 		e->link = NULL;
// 	}
// }

/* }}} */

// static void php_firebird_commit_link_rsrc(zend_resource *rsrc) /* {{{ */
// {
// 	firebird_db_link *link = (firebird_db_link *) rsrc->ptr;

// 	_php_firebird_commit_link(link);
// }
/* }}} */

// static void _php_firebird_close_link(zend_resource *rsrc) /* {{{ */
// {
// 	firebird_db_link *link = (firebird_db_link *) rsrc->ptr;

// 	_php_firebird_commit_link(link);
// 	if (link->handle != 0) {
// 		IBDEBUG("Closing normal link...");
// 		isc_detach_database(IB_STATUS, &link->handle);
// 	}
// 	IBG(num_links)--;
// 	efree(link);
// }
/* }}} */

// static void _php_firebird_close_plink(zend_resource *rsrc) /* {{{ */
// {
// 	firebird_db_link *link = (firebird_db_link *) rsrc->ptr;

// 	_php_firebird_commit_link(link);
// 	IBDEBUG("Closing permanent link...");
// 	if (link->handle != 0) {
// 		isc_detach_database(IB_STATUS, &link->handle);
// 	}
// 	IBG(num_persistent)--;
// 	IBG(num_links)--;
// 	free(link);
// }
/* }}} */

// static void _php_firebird_free_trans(zend_resource *rsrc) /* {{{ */
// {
// 	firebird_trans *trans = (firebird_trans *)rsrc->ptr;
// 	unsigned short i;

// 	IBDEBUG("Cleaning up transaction resource...");
// 	if (trans->handle != 0) {
// 		IBDEBUG("Rolling back unhandled transaction...");
// 		if (isc_rollback_transaction(IB_STATUS, &trans->handle)) {
// 			_php_firebird_error();
// 		}
// 	}

// 	/* now remove this transaction from all the connection-transaction lists */
// 	for (i = 0; i < trans->link_cnt; ++i) {
// 		if (trans->db_link[i] != NULL) {
// 			firebird_tr_list **l;
// 			for (l = &trans->db_link[i]->tr_list; *l != NULL; l = &(*l)->next) {
// 				if ( (*l)->trans == trans) {
// 					firebird_tr_list *p = *l;
// 					*l = p->next;
// 					efree(p);
// 					break;
// 				}
// 			}
// 		}
// 	}
// 	efree(trans);
// }
/* }}} */

/* TODO this function should be part of either Zend or PHP API */
static PHP_INI_DISP(php_firebird_password_displayer_cb)
{
    if ((type == PHP_INI_DISPLAY_ORIG && ini_entry->orig_value)
            || (type == PHP_INI_DISPLAY_ACTIVE && ini_entry->value)) {
        PUTS("********");
    } else if (!sapi_module.phpinfo_as_text) {
        PUTS("<i>no value</i>");
    } else {
        PUTS("no value");
    }
}

#define PUTS_TP(str) do {       \
    if(has_puts) {              \
        PUTS(" | ");            \
    }                           \
    PUTS(str);                  \
    has_puts = 1;               \
} while (0)

static PHP_INI_DISP(php_firebird_trans_displayer)
{
    int has_puts = 0;
    char *value;

    if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified) {
        value = ZSTR_VAL(ini_entry->orig_value);
    } else if (ini_entry->value) {
        value = ZSTR_VAL(ini_entry->value);
    } else {
        value = NULL;
    }

    if (value) {
        zend_long trans_argl = atol(value);

        if (trans_argl != PHP_FIREBIRD_DEFAULT) {
            /* access mode */
            if (PHP_FIREBIRD_READ == (trans_argl & PHP_FIREBIRD_READ)) {
                PUTS_TP("FIREBIRD_READ");
            } else if (PHP_FIREBIRD_WRITE == (trans_argl & PHP_FIREBIRD_WRITE)) {
                PUTS_TP("FIREBIRD_WRITE");
            }

            /* isolation level */
            if (PHP_FIREBIRD_COMMITTED == (trans_argl & PHP_FIREBIRD_COMMITTED)) {
                PUTS_TP("FIREBIRD_COMMITTED");
                if (PHP_FIREBIRD_REC_VERSION == (trans_argl & PHP_FIREBIRD_REC_VERSION)) {
                    PUTS_TP("FIREBIRD_REC_VERSION");
                } else if (PHP_FIREBIRD_REC_NO_VERSION == (trans_argl & PHP_FIREBIRD_REC_NO_VERSION)) {
                    PUTS_TP("FIREBIRD_REC_NO_VERSION");
                }
            } else if (PHP_FIREBIRD_CONSISTENCY == (trans_argl & PHP_FIREBIRD_CONSISTENCY)) {
                PUTS_TP("FIREBIRD_CONSISTENCY");
            } else if (PHP_FIREBIRD_CONCURRENCY == (trans_argl & PHP_FIREBIRD_CONCURRENCY)) {
                PUTS_TP("FIREBIRD_CONCURRENCY");
            }

            /* lock resolution */
            if (PHP_FIREBIRD_NOWAIT == (trans_argl & PHP_FIREBIRD_NOWAIT)) {
                PUTS_TP("FIREBIRD_NOWAIT");
            } else if (PHP_FIREBIRD_WAIT == (trans_argl & PHP_FIREBIRD_WAIT)) {
                PUTS_TP("FIREBIRD_WAIT");
                if (PHP_FIREBIRD_LOCK_TIMEOUT == (trans_argl & PHP_FIREBIRD_LOCK_TIMEOUT)) {
                    PUTS_TP("FIREBIRD_LOCK_TIMEOUT");
                }
            }
        } else {
            PUTS_TP("FIREBIRD_DEFAULT");
        }
    }
}

/* {{{ startup, shutdown and info functions */
PHP_INI_BEGIN()
    // PHP_INI_ENTRY_EX("ibase.allow_persistent", "1", PHP_INI_SYSTEM, NULL, zend_ini_boolean_displayer_cb)
    // PHP_INI_ENTRY_EX("ibase.max_persistent", "-1", PHP_INI_SYSTEM, NULL, display_link_numbers)
    // PHP_INI_ENTRY_EX("ibase.max_links", "-1", PHP_INI_SYSTEM, NULL, display_link_numbers)
    // PHP_INI_ENTRY("ibase.default_db", NULL, PHP_INI_SYSTEM, NULL)
    // PHP_INI_ENTRY("ibase.default_user", NULL, PHP_INI_ALL, NULL)
    // PHP_INI_ENTRY_EX("ibase.default_password", NULL, PHP_INI_ALL, NULL, php_firebird_password_displayer_cb)
    // PHP_INI_ENTRY("ibase.default_charset", NULL, PHP_INI_ALL, NULL)
    // PHP_INI_ENTRY("ibase.timestampformat", IB_DEF_DATE_FMT " " IB_DEF_TIME_FMT, PHP_INI_ALL, NULL)
    // PHP_INI_ENTRY("ibase.dateformat", IB_DEF_DATE_FMT, PHP_INI_ALL, NULL)
    // PHP_INI_ENTRY("ibase.timeformat", IB_DEF_TIME_FMT, PHP_INI_ALL, NULL)
    // STD_PHP_INI_ENTRY_EX("ibase.default_trans_params", "0", PHP_INI_ALL, OnUpdateLongGEZero, default_trans_params, zend_firebird_globals, firebird_globals, php_firebird_trans_displayer)
    // STD_PHP_INI_ENTRY_EX("ibase.default_lock_timeout", "0", PHP_INI_ALL, OnUpdateLongGEZero, default_lock_timeout, zend_firebird_globals, firebird_globals, display_link_numbers)
PHP_INI_END()

static PHP_GINIT_FUNCTION(firebird)
{
#if defined(COMPILE_DL_FIREBIRD) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    // firebird_globals->num_persistent = firebird_globals->num_links = 0;
    // firebird_globals->sql_code = *firebird_globals->errmsg = 0;
    // firebird_globals->default_link = NULL;
}

static zend_class_entry *firebird_connection_ce;

PHP_METHOD(Connection, __construct) {
    zend_string *database = NULL, *username = NULL, *password = NULL, *charset = NULL, *role = NULL;
    zend_long buffers = 0, dialect = 0;
    bool buffers_is_null = 1, dialect_is_null = 1;

    php_printf("__construct\n");

    ZEND_PARSE_PARAMETERS_START(1, 7)
        Z_PARAM_STR(database)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(username)
        Z_PARAM_STR_OR_NULL(password)
        Z_PARAM_STR_OR_NULL(charset)
        Z_PARAM_LONG_OR_NULL(buffers, buffers_is_null)
        Z_PARAM_LONG_OR_NULL(dialect, dialect_is_null)
        Z_PARAM_STR_OR_NULL(role)
    ZEND_PARSE_PARAMETERS_END();

    if(database){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "database", sizeof("database") - 1, database);
        php_printf("database: %s\n", ZSTR_VAL(database));
    }

    if(username){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "username", sizeof("username") - 1, username);
        php_printf("username: %s\n", ZSTR_VAL(username));
    }

    if(password){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "password", sizeof("password") - 1, password);
        php_printf("password: %s\n", ZSTR_VAL(password));
    }

    if(charset){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "charset", sizeof("charset") - 1, charset);
        php_printf("charset: %s\n", ZSTR_VAL(charset));
    }

    if(!buffers_is_null){
        zend_update_property_long(firebird_connection_ce, Z_OBJ_P(getThis()), "buffers", sizeof("buffers") - 1, buffers);
        php_printf("buffers: %d\n", buffers);
    }

    if(!dialect_is_null){
        zend_update_property_long(firebird_connection_ce, Z_OBJ_P(getThis()), "dialect", sizeof("dialect") - 1, dialect);
        php_printf("dialect: %d\n", dialect);
    }

    if(role){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "role", sizeof("role") - 1, role);
        php_printf("role: %s\n", ZSTR_VAL(role));
    }
}

void dump_buffer(const unsigned char *buffer, int len){
    for (int i=0; i<len; i++) {
        if(buffer[i] < 31 || buffer[i] > 126)
            php_printf("0x%02x ", buffer[i]);
        else
            php_printf("%c", buffer[i]);
    }
    php_printf("\n");
}

PHP_METHOD(Connection, connect) {
    ZEND_PARSE_PARAMETERS_NONE();

    php_printf("connect()\n");

    zval rv;
    zval *database, *val;

    long SQLCODE;
    ISC_STATUS_ARRAY status;
    isc_db_handle db = 0;

    static char const dpb_args_str[] = { isc_dpb_user_name, isc_dpb_password, isc_dpb_lc_ctype, isc_dpb_sql_role_name };
    const char *class_args_str[] = { "username", "password", "charset", "role" };

    database = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "database", sizeof("database") - 1, 1, &rv);
    if (!Z_STRLEN_P(database)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
        RETURN_FALSE;
    }

    char dpb_buffer[257] = {0}, *dpb;
    short dpb_len, buf_len = sizeof(dpb_buffer);

    dpb = dpb_buffer;

    // TODO: isc_dpb_version2
    *dpb++ = isc_dpb_version1; buf_len--;

    int len = sizeof(class_args_str) / sizeof(class_args_str[0]);
    for(int i = 0; i < len; i++){
        val = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), class_args_str[i], strlen(class_args_str[i]), 1, &rv);
        if (!Z_ISNULL_P(val) && Z_STRLEN_P(val)) {
            php_printf("arg%d: %s = %s\n", i, class_args_str[i], Z_STRVAL_P(val));
            dpb_len = slprintf(dpb, buf_len, "%c%c%s", dpb_args_str[i], (unsigned char)Z_STRLEN_P(val), Z_STRVAL_P(val));
            dpb += dpb_len;
            buf_len -= dpb_len;
        }
    }

    val = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "buffers", sizeof("buffers") - 1, 1, &rv);
    if (!Z_ISNULL_P(val)) {
        dpb_len = slprintf(dpb, buf_len, "%c\2%c%c", isc_dpb_num_buffers,
            (char)(Z_LVAL_P(val) >> 8), (char)(Z_LVAL_P(val) & 0xff));
        dpb += dpb_len;
        buf_len -= dpb_len;
    }

    // TODO: something does not add up here. isc_spb_prp_wm_sync is related to services, why the val == isc_spb_prp_wm_sync check?
    //       Disabling for now
    // val = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "sync", sizeof("sync") - 1, 1, &rv);
    // if (!Z_ISNULL_P(val)) {
    //     dpb_len = slprintf(dpb, buf_len, "%c\1%c", isc_dpb_force_write, Z_LVAL_P(val) == isc_spb_prp_wm_sync);
    //     dpb += dpb_len;
    //     buf_len -= dpb_len;
    // }

#if FB_API_VER >= 40
    // Do not handle directly INT128 or DECFLOAT, convert to VARCHAR at server instead
    const char *compat = "int128 to varchar;decfloat to varchar";
    dpb_len = slprintf(dpb, buf_len, "%c%c%s", isc_dpb_set_bind, strlen(compat), compat);
    dpb += dpb_len;
    buf_len -= dpb_len;
#endif

    if (isc_attach_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db, (short)(dpb-dpb_buffer), dpb_buffer)) {
        if (status[0] == 1 && status[1]){
            char msg[1024] = {0};
            char *s = msg;
            const ISC_STATUS* pstatus = status;

            while ((s - msg) < sizeof(msg) && fb_interpret(s, sizeof(msg) - (s - msg), &pstatus)) {
                s = msg + strlen(msg);
                *s++ = '\n';
            }

            if(s != msg){
                zval *error_msg = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "error_msg", sizeof("error_msg") - 1, 1, &rv);
                php_printf("%s\n", msg);
                ZVAL_STRINGL(error_msg, msg, s - msg - 1); // -1 trim last newline
                zend_update_property(firebird_connection_ce, Z_OBJ_P(getThis()), "error_msg", sizeof("error_msg") - 1, error_msg);
            }

            // smart_string buf = {0};
            // if(fb_interpret(msg, sizeof(msg), &pstatus))smart_string_appends(&buf, msg);
            // while(fb_interpret(msg, sizeof(msg), &pstatus)) {
            //     smart_string_appends(&buf, "\n");
            //     smart_string_appends(&buf, msg);
            // }
            // if(buf.c){
            //     zval *error_msg = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "error_msg", sizeof("error_msg") - 1, 1, &rv);
            //     smart_string_0(&buf);
            //     php_printf("%s\n", buf.c);
            //     ZVAL_STRING(error_msg, buf.c);
            //     zend_update_property(firebird_connection_ce, Z_OBJ_P(getThis()), "error_msg", sizeof("error_msg") - 1, error_msg);
            // }
            // smart_string_free(&buf);

            zval *error_code = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "error_code", sizeof("error_code") - 1, 1, &rv);
            ZVAL_LONG(error_code, (zend_long)isc_sqlcode(status));
            zend_update_property(firebird_connection_ce, Z_OBJ_P(getThis()), "error_code", sizeof("error_code") - 1, error_code);

            zval *error_code_long = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "error_code_long", sizeof("error_code_long") - 1, 1, &rv);
            ZVAL_LONG(error_code_long, (zend_long)isc_portable_integer((const ISC_UCHAR*)&status[1], 4));
            zend_update_property(firebird_connection_ce, Z_OBJ_P(getThis()), "error_code_long", sizeof("error_code_long") - 1, error_code_long);
        }
        RETURN_FALSE;
    }

    php_printf("Connected!\n");

    RETURN_TRUE;
}

const zend_function_entry firebird_connection_functions[] = {
    PHP_ME(Connection, __construct, arginfo_firebird_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Connection, connect, arginfo_firebird_void, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_MINIT_FUNCTION(firebird)
{
    REGISTER_INI_ENTRIES();

    // le_link = zend_register_list_destructors_ex(_php_firebird_close_link, NULL, LE_LINK, module_number);
    // le_plink = zend_register_list_destructors_ex(php_firebird_commit_link_rsrc, _php_firebird_close_plink, LE_PLINK, module_number);
    // le_trans = zend_register_list_destructors_ex(_php_firebird_free_trans, NULL, LE_TRANS, module_number);

    REGISTER_LONG_CONSTANT("FIREBIRD_DEFAULT", PHP_FIREBIRD_DEFAULT, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_CREATE", PHP_FIREBIRD_CREATE, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_TEXT", PHP_FIREBIRD_FETCH_BLOBS, CONST_PERSISTENT); /* deprecated, for BC only */
    REGISTER_LONG_CONSTANT("FIREBIRD_FETCH_BLOBS", PHP_FIREBIRD_FETCH_BLOBS, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_FETCH_ARRAYS", PHP_FIREBIRD_FETCH_ARRAYS, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_UNIXTIME", PHP_FIREBIRD_UNIXTIME, CONST_PERSISTENT);

    /* transactions */
    REGISTER_LONG_CONSTANT("FIREBIRD_WRITE", PHP_FIREBIRD_WRITE, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_READ", PHP_FIREBIRD_READ, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_COMMITTED", PHP_FIREBIRD_COMMITTED, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_CONSISTENCY", PHP_FIREBIRD_CONSISTENCY, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_CONCURRENCY", PHP_FIREBIRD_CONCURRENCY, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_REC_VERSION", PHP_FIREBIRD_REC_VERSION, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_REC_NO_VERSION", PHP_FIREBIRD_REC_NO_VERSION, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_NOWAIT", PHP_FIREBIRD_NOWAIT, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_WAIT", PHP_FIREBIRD_WAIT, CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FIREBIRD_LOCK_TIMEOUT", PHP_FIREBIRD_LOCK_TIMEOUT, CONST_PERSISTENT);

    // php_firebird_query_minit(INIT_FUNC_ARGS_PASSTHRU);
    // php_firebird_blobs_minit(INIT_FUNC_ARGS_PASSTHRU);
    // php_firebird_events_minit(INIT_FUNC_ARGS_PASSTHRU);
    // php_firebird_service_minit(INIT_FUNC_ARGS_PASSTHRU);

#ifdef ZEND_SIGNALS
    // firebird replaces some signals at runtime, suppress warnings.
    SIGG(check) = 0;
#endif

    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Connection", firebird_connection_functions);
    firebird_connection_ce = zend_register_internal_class(&tmp_ce);

#define DECLARE_PROP_INT(class_ce, name) DECLARE_PROP(class_ce, name, MAY_BE_LONG)
#define DECLARE_PROP_STRING(class_ce, name) DECLARE_PROP(class_ce, name, MAY_BE_STRING)
#define DECLARE_PROP(class_ce, name, type) do {                                       \
    zval prop_##name##_def_val;                                                       \
    ZVAL_UNDEF(&prop_##name##_def_val);                                               \
    zend_string *prop_##name##_name = zend_string_init(#name, sizeof(#name) - 1, 1);  \
    zend_declare_typed_property(class_ce, prop_##name##_name, &prop_##name##_def_val, \
        ZEND_ACC_PROTECTED, NULL,                                                     \
        (zend_type) ZEND_TYPE_INIT_MASK(type));                                       \
} while (0)

    DECLARE_PROP_STRING(firebird_connection_ce, database);
    DECLARE_PROP_STRING(firebird_connection_ce, username);
    DECLARE_PROP_STRING(firebird_connection_ce, password);
    DECLARE_PROP_STRING(firebird_connection_ce, charset);
    DECLARE_PROP_INT(firebird_connection_ce, buffers);
    DECLARE_PROP_INT(firebird_connection_ce, dialect);
    DECLARE_PROP_STRING(firebird_connection_ce, role);

    DECLARE_PROP_STRING(firebird_connection_ce, error_msg);
    DECLARE_PROP_INT(firebird_connection_ce, error_code);
    DECLARE_PROP_INT(firebird_connection_ce, error_code_long);
    // bcmath_number_ce->default_object_handlers = &bcmath_number_obj_handlers;
    // memcpy(&bcmath_number_obj_handlers, &std_object_handlers, sizeof(zend_object_handlers));
    // bcmath_number_obj_handlers.offset = XtOffsetOf(bcmath_number_obj_t, std);
    // bcmath_number_obj_handlers.free_obj = bcmath_number_free;
    // bcmath_number_obj_handlers.clone_obj = bcmath_number_clone;
    // bcmath_number_obj_handlers.do_operation = bcmath_number_do_operation;
    // bcmath_number_obj_handlers.compare = bcmath_number_compare;
    // bcmath_number_obj_handlers.write_property = bcmath_number_write_property;
    // bcmath_number_obj_handlers.unset_property = bcmath_number_unset_property;
    // bcmath_number_obj_handlers.has_property = bcmath_number_has_property;
    // bcmath_number_obj_handlers.read_property = bcmath_number_read_property;
    // bcmath_number_obj_handlers.get_properties_for = bcmath_number_get_properties_for;
    // bcmath_number_obj_handlers.cast_object = bcmath_number_cast_object;

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(firebird)
{
#ifndef PHP_WIN32
    /**
     * When the Interbase client API library libgds.so is first loaded, it registers a call to
     * gds__cleanup() with atexit(), in order to clean up after itself when the process exits.
     * This means that the library is called at process shutdown, and cannot be unloaded beforehand.
     * PHP tries to unload modules after every request [dl()'ed modules], and right before the
     * process shuts down [modules loaded from php.ini]. This results in a segfault for this module.
     * By NULLing the dlopen() handle in the module entry, Zend omits the call to dlclose(),
     * ensuring that the module will remain present until the process exits. However, the functions
     * and classes exported by the module will not be available until the module is 'reloaded'.
     * When reloaded, dlopen() will return the handle of the already loaded module. The module will
     * be unloaded automatically when the process exits.
     */
    zend_module_entry *firebird_entry;
    if ((firebird_entry = zend_hash_str_find_ptr(&module_registry, firebird_module_entry.name,
            strlen(firebird_module_entry.name))) != NULL) {
        firebird_entry->handle = 0;
    }
#endif
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(firebird)
{
    // IBG(num_links) = IBG(num_persistent);
    // IBG(default_link)= NULL;

    // RESET_ERRMSG;

    return SUCCESS;
}

PHP_MINFO_FUNCTION(firebird)
{
    char tmp[64], *s;

    php_info_print_table_start();
    php_info_print_table_row(2, "Firebird/InterBase Support",
#ifdef COMPILE_DL_FIREBIRD
        "dynamic");
#else
        "static");
#endif

    php_info_print_table_row(2, "Interbase extension version", PHP_FIREBIRD_VERSION);

#ifdef FB_API_VER
    snprintf( (s = tmp), sizeof(tmp), "Firebird API version %d", FB_API_VER);
#elif (SQLDA_CURRENT_VERSION > 1)
    s =  "Interbase 7.0 and up";
#endif
    php_info_print_table_row(2, "Compile-time Client Library Version", s);

#if defined(__GNUC__) || defined(PHP_WIN32)
    do {
        info_func_t info_func = NULL;
#ifdef __GNUC__
        info_func = (info_func_t)dlsym(RTLD_DEFAULT, "isc_get_client_version");
#else
        HMODULE l = GetModuleHandle("fbclient");

        if (!l && !(l = GetModuleHandle("gds32"))) {
            break;
        }
        info_func = (info_func_t)GetProcAddress(l, "isc_get_client_version");
#endif
        if (info_func) {
            info_func(s = tmp);
        }
        php_info_print_table_row(2, "Run-time Client Library Version", s);
    } while (0);
#endif
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();

}
/* }}} */

// static void _php_firebird_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent) /* {{{ */
// {
// 	char *c, hash[16], *args[] = { NULL, NULL, NULL, NULL, NULL };
// 	int i;
// 	size_t len[] = { 0, 0, 0, 0, 0 };
// 	zend_long largs[] = { 0, 0, 0 };
// 	PHP_MD5_CTX hash_context;
// 	zend_resource new_index_ptr, *le;
// 	isc_db_handle db_handle = 0;
// 	firebird_db_link *ib_link;

// 	RESET_ERRMSG;

// 	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "|ssssllsl",
// 			&args[DB], &len[DB], &args[USER], &len[USER], &args[PASS], &len[PASS],
// 			&args[CSET], &len[CSET], &largs[BUF], &largs[DLECT], &args[ROLE], &len[ROLE],
// 			&largs[SYNC])) {
// 		RETURN_FALSE;
// 	}

// 	/* restrict to the server/db in the .ini if in safe mode */
// 	if (!len[DB] && (c = INI_STR("ibase.default_db"))) {
// 		args[DB] = c;
// 		len[DB] = strlen(c);
// 	}
// 	if (!len[USER] && (c = INI_STR("ibase.default_user"))) {
// 		args[USER] = c;
// 		len[USER] = strlen(c);
// 	}
// 	if (!len[PASS] && (c = INI_STR("ibase.default_password"))) {
// 		args[PASS] = c;
// 		len[PASS] = strlen(c);
// 	}
// 	if (!len[CSET] && (c = INI_STR("ibase.default_charset"))) {
// 		args[CSET] = c;
// 		len[CSET] = strlen(c);
// 	}

// 	/* don't want usernames and passwords floating around */
// 	PHP_MD5Init(&hash_context);
// 	for (i = 0; i < sizeof(args)/sizeof(char*); ++i) {
// 		PHP_MD5Update(&hash_context,args[i],len[i]);
// 	}
// 	for (i = 0; i < sizeof(largs)/sizeof(zend_long); ++i) {
// 		PHP_MD5Update(&hash_context,(char*)&largs[i],sizeof(zend_long));
// 	}
// 	PHP_MD5Final((unsigned char*)hash, &hash_context);

// 	/* try to reuse a connection */
// 	if ((le = zend_hash_str_find_ptr(&EG(regular_list), hash, sizeof(hash)-1)) != NULL) {
// 		zend_resource *xlink;

// 		if (le->type != le_index_ptr) {
// 			RETURN_FALSE;
// 		}

// 		xlink = (zend_resource*) le->ptr;
// 		if ((!persistent && xlink->type == le_link) || xlink->type == le_plink) {
// 			if (IBG(default_link) != xlink) {
// 				GC_ADDREF(xlink);
// 				if (IBG(default_link)) {
// 					zend_list_delete(IBG(default_link));
// 				}
// 				IBG(default_link) = xlink;
// 			}
// 			GC_ADDREF(xlink);
// 			RETURN_RES(xlink);
// 		} else {
// 			zend_hash_str_del(&EG(regular_list), hash, sizeof(hash)-1);
// 		}
// 	}

// 	/* ... or a persistent one */
// 	do {
// 		zend_long l;
// 		static char info[] = { isc_info_base_level, isc_info_end };
// 		char result[8];
// 		ISC_STATUS status[20];

// 		if ((le = zend_hash_str_find_ptr(&EG(persistent_list), hash, sizeof(hash)-1)) != NULL) {
// 			if (le->type != le_plink) {
// 				RETURN_FALSE;
// 			}
// 			/* check if connection has timed out */
// 			ib_link = (firebird_db_link *) le->ptr;
// 			if (!isc_database_info(status, &ib_link->handle, sizeof(info), info, sizeof(result), result)) {
// 				RETVAL_RES(zend_register_resource(ib_link, le_plink));
// 				break;
// 			}
// 			zend_hash_str_del(&EG(persistent_list), hash, sizeof(hash)-1);
// 		}

// 		/* no link found, so we have to open one */

// 		if ((l = INI_INT("ibase.max_links")) != -1 && IBG(num_links) >= l) {
// 			_php_firebird_module_error("Too many open links (%ld)", IBG(num_links));
// 			RETURN_FALSE;
// 		}

// 		/* create the ib_link */
// 		if (FAILURE == _php_firebird_attach_db(args, len, largs, &db_handle)) {
// 			RETURN_FALSE;
// 		}

// 		/* use non-persistent if allowed number of persistent links is exceeded */
// 		if (!persistent || ((l = INI_INT("ibase.max_persistent") != -1) && IBG(num_persistent) >= l)) {
// 			ib_link = (firebird_db_link *) emalloc(sizeof(firebird_db_link));
// 			RETVAL_RES(zend_register_resource(ib_link, le_link));
// 		} else {
// 			ib_link = (firebird_db_link *) malloc(sizeof(firebird_db_link));
// 			if (!ib_link) {
// 				RETURN_FALSE;
// 			}

// 			/* hash it up */
// 			if (zend_register_persistent_resource(hash, sizeof(hash)-1, ib_link, le_plink) == NULL) {
// 				free(ib_link);
// 				RETURN_FALSE;
// 			}
// 			RETVAL_RES(zend_register_resource(ib_link, le_plink));
// 			++IBG(num_persistent);
// 		}
// 		ib_link->handle = db_handle;
// 		ib_link->dialect = largs[DLECT] ? (unsigned short)largs[DLECT] : SQL_DIALECT_CURRENT;
// 		ib_link->tr_list = NULL;
// 		ib_link->event_head = NULL;

// 		++IBG(num_links);
// 	} while (0);

// 	/* add it to the hash */
// 	new_index_ptr.ptr = (void *) Z_RES_P(return_value);
// 	new_index_ptr.type = le_index_ptr;
// 	zend_hash_str_update_mem(&EG(regular_list), hash, sizeof(hash)-1,
// 			(void *) &new_index_ptr, sizeof(zend_resource));
// 	if (IBG(default_link)) {
// 		zend_list_delete(IBG(default_link));
// 	}
// 	IBG(default_link) = Z_RES_P(return_value);
// 	Z_TRY_ADDREF_P(return_value);
// 	Z_TRY_ADDREF_P(return_value);
// }
/* }}} */

/* {{{ proto resource firebird_connect([string database [, string username [, string password [, string charset [, int buffers [, int dialect [, string role]]]]]]])
   Open a connection to an InterBase database */
// PHP_FUNCTION(firebird_connect)
// {
// 	_php_firebird_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
// }
/* }}} */

/* {{{ proto resource firebird_pconnect([string database [, string username [, string password [, string charset [, int buffers [, int dialect [, string role]]]]]]])
   Open a persistent connection to an InterBase database */
// PHP_FUNCTION(firebird_pconnect)
// {
// 	_php_firebird_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, INI_INT("ibase.allow_persistent"));
// }
/* }}} */

/* {{{ proto bool firebird_close([resource link_identifier])
   Close an InterBase connection */
// PHP_FUNCTION(firebird_close)
// {
// 	zval *link_arg = NULL;
// 	zend_resource *link_res;

// 	RESET_ERRMSG;

// 	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|r", &link_arg) == FAILURE) {
// 		return;
// 	}

// 	if (ZEND_NUM_ARGS() == 0) {
// 		link_res = IBG(default_link);
// 		CHECK_LINK(link_res);
// 		IBG(default_link) = NULL;
// 	} else {
// 		link_res = Z_RES_P(link_arg);
// 	}

// 	/* we have at least 3 additional references to this resource ??? */
// 	if (GC_REFCOUNT(link_res) < 4) {
// 		zend_list_close(link_res);
// 	} else {
// 		zend_list_delete(link_res);
// 	}
// 	RETURN_TRUE;
// }
/* }}} */

/* {{{ proto bool firebird_drop_db([resource link_identifier])
   Drop an InterBase database */
// PHP_FUNCTION(firebird_drop_db)
// {
// 	zval *link_arg = NULL;
// 	firebird_db_link *ib_link;
// 	firebird_tr_list *l;
// 	zend_resource *link_res;

// 	RESET_ERRMSG;

// 	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|r", &link_arg) == FAILURE) {
// 		return;
// 	}

// 	if (ZEND_NUM_ARGS() == 0) {
// 		link_res = IBG(default_link);
// 		CHECK_LINK(link_res);
// 		IBG(default_link) = NULL;
// 	} else {
// 		link_res = Z_RES_P(link_arg);
// 	}

// 	ib_link = (firebird_db_link *)zend_fetch_resource2(link_res, LE_LINK, le_link, le_plink);

// 	if (!ib_link) {
// 		RETURN_FALSE;
// 	}

// 	if (isc_drop_database(IB_STATUS, &ib_link->handle)) {
// 		_php_firebird_error();
// 		RETURN_FALSE;
// 	}

// 	/* isc_drop_database() doesn't invalidate the transaction handles */
// 	for (l = ib_link->tr_list; l != NULL; l = l->next) {
// 		if (l->trans != NULL) l->trans->handle = 0;
// 	}

// 	zend_list_delete(link_res);

// 	RETURN_TRUE;
// }
/* }}} */

/* {{{ proto resource firebird_trans([int trans_args [, resource link_identifier [, ... ], int trans_args [, resource link_identifier [, ... ]] [, ...]]])
   Start a transaction over one or several databases */

#define TPB_MAX_SIZE (8*sizeof(char))

void _php_firebird_populate_trans(zend_long trans_argl, zend_long trans_timeout, char *last_tpb, unsigned short *len) /* {{{ */
{
    unsigned short tpb_len = 0;
    if (trans_argl != PHP_FIREBIRD_DEFAULT) {
        last_tpb[tpb_len++] = isc_tpb_version3;

        /* access mode */
        if (PHP_FIREBIRD_READ == (trans_argl & PHP_FIREBIRD_READ)) {
            last_tpb[tpb_len++] = isc_tpb_read;
        } else if (PHP_FIREBIRD_WRITE == (trans_argl & PHP_FIREBIRD_WRITE)) {
            last_tpb[tpb_len++] = isc_tpb_write;
        }

        /* isolation level */
        if (PHP_FIREBIRD_COMMITTED == (trans_argl & PHP_FIREBIRD_COMMITTED)) {
            last_tpb[tpb_len++] = isc_tpb_read_committed;
            if (PHP_FIREBIRD_REC_VERSION == (trans_argl & PHP_FIREBIRD_REC_VERSION)) {
                last_tpb[tpb_len++] = isc_tpb_rec_version;
            } else if (PHP_FIREBIRD_REC_NO_VERSION == (trans_argl & PHP_FIREBIRD_REC_NO_VERSION)) {
                last_tpb[tpb_len++] = isc_tpb_no_rec_version;
            }
        } else if (PHP_FIREBIRD_CONSISTENCY == (trans_argl & PHP_FIREBIRD_CONSISTENCY)) {
            last_tpb[tpb_len++] = isc_tpb_consistency;
        } else if (PHP_FIREBIRD_CONCURRENCY == (trans_argl & PHP_FIREBIRD_CONCURRENCY)) {
            last_tpb[tpb_len++] = isc_tpb_concurrency;
        }

        /* lock resolution */
        if (PHP_FIREBIRD_NOWAIT == (trans_argl & PHP_FIREBIRD_NOWAIT)) {
            last_tpb[tpb_len++] = isc_tpb_nowait;
        } else if (PHP_FIREBIRD_WAIT == (trans_argl & PHP_FIREBIRD_WAIT)) {
            last_tpb[tpb_len++] = isc_tpb_wait;
            if (PHP_FIREBIRD_LOCK_TIMEOUT == (trans_argl & PHP_FIREBIRD_LOCK_TIMEOUT)) {
                if (trans_timeout <= 0 || trans_timeout > 0x7FFF) {
                    php_error_docref(NULL, E_WARNING, "Invalid timeout parameter");
                } else {
                    last_tpb[tpb_len++] = isc_tpb_lock_timeout;
                    last_tpb[tpb_len++] = sizeof(ISC_SHORT);
                    last_tpb[tpb_len] = (ISC_SHORT)trans_timeout;
                    tpb_len += sizeof(ISC_SHORT);
                }
            }
        }
    }
    *len = tpb_len;
}
/* }}} */

// PHP_FUNCTION(firebird_trans)
// {
// 	unsigned short i, link_cnt = 0, tpb_len = 0;
// 	int argn = ZEND_NUM_ARGS();
// 	char last_tpb[TPB_MAX_SIZE];
// 	firebird_db_link **ib_link = NULL;
// 	firebird_trans *ib_trans;
// 	isc_tr_handle tr_handle = 0;
// 	ISC_STATUS result;

// 	RESET_ERRMSG;

// 	/* (1+argn) is an upper bound for the number of links this trans connects to */
// 	ib_link = (firebird_db_link **) safe_emalloc(sizeof(firebird_db_link *),1+argn,0);

// 	if (argn > 0) {
// 		zend_long trans_argl = 0;
// 		zend_long trans_timeout = 0;
// 		char *tpb;
// 		ISC_TEB *teb;
// 		zval *args = NULL;

// 		if (zend_parse_parameters(argn, "+", &args, &argn) == FAILURE) {
// 			efree(ib_link);
// 			RETURN_FALSE;
// 		}

// 		teb = (ISC_TEB *) safe_emalloc(sizeof(ISC_TEB),argn,0);
// 		tpb = (char *) safe_emalloc(TPB_MAX_SIZE,argn,0);

// 		/* enumerate all the arguments: assume every non-resource argument
// 		   specifies modifiers for the link ids that follow it */
// 		for (i = 0; i < argn; ++i) {

// 			if (Z_TYPE(args[i]) == IS_RESOURCE) {

// 				if ((ib_link[link_cnt] = (firebird_db_link *)zend_fetch_resource2_ex(&args[i], LE_LINK, le_link, le_plink)) == NULL) {
// 					efree(teb);
// 					efree(tpb);
// 					efree(ib_link);
// 					RETURN_FALSE;
// 				}

// 				/* copy the most recent modifier string into tbp[] */
// 				memcpy(&tpb[TPB_MAX_SIZE * link_cnt], last_tpb, TPB_MAX_SIZE);

// 				/* add a database handle to the TEB with the most recently specified set of modifiers */
// 				teb[link_cnt].db_ptr = &ib_link[link_cnt]->handle;
// 				teb[link_cnt].tpb_len = tpb_len;
// 				teb[link_cnt].tpb_ptr = &tpb[TPB_MAX_SIZE * link_cnt];

// 				++link_cnt;

// 			} else {

// 				tpb_len = 0;

// 				convert_to_long_ex(&args[i]);
// 				trans_argl = Z_LVAL(args[i]);
// 				if (trans_argl != PHP_FIREBIRD_DEFAULT) {
// 					// Skip conflicting parameters
// 					if (PHP_FIREBIRD_NOWAIT != (trans_argl & PHP_FIREBIRD_NOWAIT) && PHP_FIREBIRD_WAIT == (trans_argl & PHP_FIREBIRD_WAIT)) {
// 						if (PHP_FIREBIRD_LOCK_TIMEOUT == (trans_argl & PHP_FIREBIRD_LOCK_TIMEOUT)) {
// 							if((i + 1 < argn) && (Z_TYPE(args[i + 1]) == IS_LONG)){
// 								i++;
// 								convert_to_long_ex(&args[i]);
// 								trans_timeout = Z_LVAL(args[i]);
// 							} else {
// 								php_error_docref(NULL, E_WARNING, "FIREBIRD_LOCK_TIMEOUT expects next argument to be timeout value");
// 							}
// 						}
// 					}
// 					_php_firebird_populate_trans(trans_argl, trans_timeout, last_tpb, &tpb_len);
// 				}
// 			}
// 		}

// 		if (link_cnt > 0) {
// 			result = isc_start_multiple(IB_STATUS, &tr_handle, link_cnt, teb);
// 		}

// 		efree(tpb);
// 		efree(teb);
// 	}

// 	if (link_cnt == 0) {
// 		link_cnt = 1;
// 		if ((ib_link[0] = (firebird_db_link *)zend_fetch_resource2(IBG(default_link), LE_LINK, le_link, le_plink)) == NULL) {
// 			efree(ib_link);
// 			RETURN_FALSE;
// 		}
// 		result = isc_start_transaction(IB_STATUS, &tr_handle, 1, &ib_link[0]->handle, tpb_len, last_tpb);
// 	}

// 	/* start the transaction */
// 	if (result) {
// 		_php_firebird_error();
// 		efree(ib_link);
// 		RETURN_FALSE;
// 	}

// 	/* register the transaction in our own data structures */
// 	ib_trans = (firebird_trans *) safe_emalloc(link_cnt-1, sizeof(firebird_db_link *), sizeof(firebird_trans));
// 	ib_trans->handle = tr_handle;
// 	ib_trans->link_cnt = link_cnt;
// 	ib_trans->affected_rows = 0;
// 	for (i = 0; i < link_cnt; ++i) {
// 		firebird_tr_list **l;
// 		ib_trans->db_link[i] = ib_link[i];

// 		/* the first item in the connection-transaction list is reserved for the default transaction */
// 		if (ib_link[i]->tr_list == NULL) {
// 			ib_link[i]->tr_list = (firebird_tr_list *) emalloc(sizeof(firebird_tr_list));
// 			ib_link[i]->tr_list->trans = NULL;
// 			ib_link[i]->tr_list->next = NULL;
// 		}

// 		/* link the transaction into the connection-transaction list */
// 		for (l = &ib_link[i]->tr_list; *l != NULL; l = &(*l)->next);
// 		*l = (firebird_tr_list *) emalloc(sizeof(firebird_tr_list));
// 		(*l)->trans = ib_trans;
// 		(*l)->next = NULL;
// 	}
// 	efree(ib_link);
// 	RETVAL_RES(zend_register_resource(ib_trans, le_trans));
// 	Z_TRY_ADDREF_P(return_value);
// }
/* }}} */

// int _php_firebird_def_trans(firebird_db_link *ib_link, firebird_trans **trans) /* {{{ */
// {
// 	if (ib_link == NULL) {
// 		php_error_docref(NULL, E_WARNING, "Invalid database link");
// 		return FAILURE;
// 	}

// 	/* the first item in the connection-transaction list is reserved for the default transaction */
// 	if (ib_link->tr_list == NULL) {
// 		ib_link->tr_list = (firebird_tr_list *) emalloc(sizeof(firebird_tr_list));
// 		ib_link->tr_list->trans = NULL;
// 		ib_link->tr_list->next = NULL;
// 	}

// 	if (*trans == NULL) {
// 		firebird_trans *tr = ib_link->tr_list->trans;

// 		if (tr == NULL) {
// 			tr = (firebird_trans *) emalloc(sizeof(firebird_trans));
// 			tr->handle = 0;
// 			tr->link_cnt = 1;
// 			tr->affected_rows = 0;
// 			tr->db_link[0] = ib_link;
// 			ib_link->tr_list->trans = tr;
// 		}
// 		if (tr->handle == 0) {
// 			ISC_STATUS result;
// 			zend_long trans_argl = IBG(default_trans_params);

// 			if(trans_argl == PHP_FIREBIRD_DEFAULT){
// 				result = isc_start_transaction(IB_STATUS, &tr->handle, 1, &ib_link->handle, 0, NULL);
// 			} else {
// 				zend_long trans_timeout = IBG(default_lock_timeout);
// 				char last_tpb[TPB_MAX_SIZE];
// 				unsigned short tpb_len = 0;
// 				_php_firebird_populate_trans(trans_argl, trans_timeout, last_tpb, &tpb_len);
// 				result = isc_start_transaction(IB_STATUS, &tr->handle, 1, &ib_link->handle, tpb_len, last_tpb);
// 			}

// 			if (result) {
// 				_php_firebird_error();
// 				return FAILURE;
// 			}
// 		}
// 		*trans = tr;
// 	}
// 	return SUCCESS;
// }
/* }}} */

// static void _php_firebird_trans_end(INTERNAL_FUNCTION_PARAMETERS, int commit) /* {{{ */
// {
// 	firebird_trans *trans = NULL;
// 	int res_id = 0;
// 	ISC_STATUS result;
// 	firebird_db_link *ib_link;
// 	zval *arg = NULL;

// 	RESET_ERRMSG;

// 	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|r", &arg) == FAILURE) {
// 		return;
// 	}

// 	if (ZEND_NUM_ARGS() == 0) {
// 		ib_link = (firebird_db_link *)zend_fetch_resource2(IBG(default_link), LE_LINK, le_link, le_plink);
// 		if (ib_link->tr_list == NULL || ib_link->tr_list->trans == NULL) {
// 			/* this link doesn't have a default transaction */
// 			_php_firebird_module_error("Default link has no default transaction");
// 			RETURN_FALSE;
// 		}
// 		trans = ib_link->tr_list->trans;
// 	} else {
// 		/* one id was passed, could be db or trans id */
// 		if (Z_RES_P(arg)->type == le_trans) {
// 			trans = (firebird_trans *)zend_fetch_resource_ex(arg, LE_TRANS, le_trans);
// 			res_id = Z_RES_P(arg)->handle;
// 		} else {
// 			ib_link = (firebird_db_link *)zend_fetch_resource2_ex(arg, LE_LINK, le_link, le_plink);

// 			if (ib_link->tr_list == NULL || ib_link->tr_list->trans == NULL) {
// 				/* this link doesn't have a default transaction */
// 				_php_firebird_module_error("Link has no default transaction");
// 				RETURN_FALSE;
// 			}
// 			trans = ib_link->tr_list->trans;
// 		}
// 	}

// 	switch (commit) {
// 		default: /* == case ROLLBACK: */
// 			result = isc_rollback_transaction(IB_STATUS, &trans->handle);
// 			break;
// 		case COMMIT:
// 			result = isc_commit_transaction(IB_STATUS, &trans->handle);
// 			break;
// 		case (ROLLBACK | RETAIN):
// 			result = isc_rollback_retaining(IB_STATUS, &trans->handle);
// 			break;
// 		case (COMMIT | RETAIN):
// 			result = isc_commit_retaining(IB_STATUS, &trans->handle);
// 			break;
// 	}

// 	if (result) {
// 		_php_firebird_error();
// 		RETURN_FALSE;
// 	}

// 	/* Don't try to destroy implicitly opened transaction from list... */
// 	if ((commit & RETAIN) == 0 && res_id != 0) {
// 		zend_list_delete(Z_RES_P(arg));
// 	}
// 	RETURN_TRUE;
// }
/* }}} */

/* {{{ proto bool firebird_commit( resource link_identifier )
   Commit transaction */
// PHP_FUNCTION(firebird_commit)
// {
// 	_php_firebird_trans_end(INTERNAL_FUNCTION_PARAM_PASSTHRU, COMMIT);
// }
/* }}} */

/* {{{ proto bool firebird_rollback( resource link_identifier )
   Rollback transaction */
// PHP_FUNCTION(firebird_rollback)
// {
// 	_php_firebird_trans_end(INTERNAL_FUNCTION_PARAM_PASSTHRU, ROLLBACK);
// }
/* }}} */

/* {{{ proto bool firebird_commit_ret( resource link_identifier )
   Commit transaction and retain the transaction context */
// PHP_FUNCTION(firebird_commit_ret)
// {
// 	_php_firebird_trans_end(INTERNAL_FUNCTION_PARAM_PASSTHRU, COMMIT | RETAIN);
// }
/* }}} */

/* {{{ proto bool firebird_rollback_ret( resource link_identifier )
   Rollback transaction and retain the transaction context */
// PHP_FUNCTION(firebird_rollback_ret)
// {
// 	_php_firebird_trans_end(INTERNAL_FUNCTION_PARAM_PASSTHRU, ROLLBACK | RETAIN);
// }
/* }}} */

/* {{{ proto int firebird_gen_id(string generator [, int increment [, resource link_identifier ]])
   Increments the named generator and returns its new value */
// PHP_FUNCTION(firebird_gen_id)
// {
// 	zval *link = NULL;
// 	char query[128], *generator;
// 	size_t gen_len;
// 	zend_long inc = 1;
// 	firebird_db_link *ib_link;
// 	firebird_trans *trans = NULL;
// 	XSQLDA out_sqlda;
// 	ISC_INT64 result;

// 	RESET_ERRMSG;

// 	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "s|lr", &generator, &gen_len,
// 			&inc, &link)) {
// 		RETURN_FALSE;
// 	}

// 	if (gen_len > 31) {
// 		php_error_docref(NULL, E_WARNING, "Invalid generator name");
// 		RETURN_FALSE;
// 	}

// 	PHP_FIREBIRD_LINK_TRANS(link, ib_link, trans);

// 	snprintf(query, sizeof(query), "SELECT GEN_ID(%s,%ld) FROM rdb$database", generator, inc);

// 	/* allocate a minimal descriptor area */
// 	out_sqlda.sqln = out_sqlda.sqld = 1;
// 	out_sqlda.version = SQLDA_CURRENT_VERSION;

// 	/* allocate the field for the result */
// 	out_sqlda.sqlvar[0].sqltype = SQL_INT64;
// 	out_sqlda.sqlvar[0].sqlscale = 0;
// 	out_sqlda.sqlvar[0].sqllen = sizeof(result);
// 	out_sqlda.sqlvar[0].sqldata = (void*) &result;

// 	/* execute the query */
// 	if (isc_dsql_exec_immed2(IB_STATUS, &ib_link->handle, &trans->handle, 0, query,
// 			SQL_DIALECT_CURRENT, NULL, &out_sqlda)) {
// 		_php_firebird_error();
// 		RETURN_FALSE;
// 	}

// 	/* don't return the generator value as a string unless it doesn't fit in a long */
// #if SIZEOF_ZEND_LONG < 8
// 	if (result < ZEND_LONG_MIN || result > ZEND_LONG_MAX) {
// 		char *res;
// 		int l;

// 		l = spprintf(&res, 0, "%" LL_MASK "d", result);
// 		RETURN_STRINGL(res, l);
// 	}
// #endif
// 	RETURN_LONG((zend_long)result);
// }

/* }}} */

#endif /* HAVE_FIREBIRD */
