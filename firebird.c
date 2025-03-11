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
    PHP_INI_ENTRY("firebird.default_charset", NULL, PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("firebird.timestampformat", FIREBIRD_DATE_FMT " " FIREBIRD_TIME_FMT, PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("firebird.dateformat", FIREBIRD_DATE_FMT, PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("firebird.timeformat", FIREBIRD_TIME_FMT, PHP_INI_ALL, NULL)
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

PHP_MINIT_FUNCTION(firebird)
{
    REGISTER_INI_ENTRIES();

    REGISTER_NS_LONG_CONSTANT("FireBird", "FETCH_BLOBS", PHP_FIREBIRD_FETCH_BLOBS, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird", "FETCH_ARRAYS", PHP_FIREBIRD_FETCH_ARRAYS, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird", "FETCH_UNIXTIME", PHP_FIREBIRD_UNIXTIME, CONST_PERSISTENT);

    /* transactions */
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "WRITE", PHP_FIREBIRD_WRITE, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "READ", PHP_FIREBIRD_READ, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "COMMITTED", PHP_FIREBIRD_COMMITTED, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "CONSISTENCY", PHP_FIREBIRD_CONSISTENCY, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "CONCURRENCY", PHP_FIREBIRD_CONCURRENCY, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "REC_VERSION", PHP_FIREBIRD_REC_VERSION, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "REC_NO_VERSION", PHP_FIREBIRD_REC_NO_VERSION, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "NOWAIT", PHP_FIREBIRD_NOWAIT, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "WAIT", PHP_FIREBIRD_WAIT, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird\\Transaction", "LOCK_TIMEOUT", PHP_FIREBIRD_LOCK_TIMEOUT, CONST_PERSISTENT);
#ifdef ZEND_SIGNALS
    // firebird replaces some signals at runtime, suppress warnings.
    SIGG(check) = 0;
#endif

    register_FireBird_Database_ce();
    register_FireBird_Connection_ce();
    register_FireBird_Transaction_ce();
    register_FireBird_Statement_ce();

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
// 	firebird_connection *ib_link;
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

// 	ib_link = (firebird_connection *)zend_fetch_resource2(link_res, LE_LINK, le_link, le_plink);

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

// PHP_FUNCTION(firebird_trans)
// {
// 	unsigned short i, link_cnt = 0, tpb_len = 0;
// 	int argn = ZEND_NUM_ARGS();
// 	char last_tpb[TPB_MAX_SIZE];
// 	firebird_connection **ib_link = NULL;
// 	firebird_trans *ib_trans;
// 	isc_tr_handle tr_handle = 0;
// 	ISC_STATUS result;

// 	RESET_ERRMSG;

// 	/* (1+argn) is an upper bound for the number of links this trans connects to */
// 	ib_link = (firebird_connection **) safe_emalloc(sizeof(firebird_connection *),1+argn,0);

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

// 				if ((ib_link[link_cnt] = (firebird_connection *)zend_fetch_resource2_ex(&args[i], LE_LINK, le_link, le_plink)) == NULL) {
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
// 		if ((ib_link[0] = (firebird_connection *)zend_fetch_resource2(IBG(default_link), LE_LINK, le_link, le_plink)) == NULL) {
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
// 	ib_trans = (firebird_trans *) safe_emalloc(link_cnt-1, sizeof(firebird_connection *), sizeof(firebird_trans));
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

// int _php_firebird_def_trans(firebird_connection *ib_link, firebird_trans **trans) /* {{{ */
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
// 	firebird_connection *ib_link;
// 	zval *arg = NULL;

// 	RESET_ERRMSG;

// 	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|r", &arg) == FAILURE) {
// 		return;
// 	}

// 	if (ZEND_NUM_ARGS() == 0) {
// 		ib_link = (firebird_connection *)zend_fetch_resource2(IBG(default_link), LE_LINK, le_link, le_plink);
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
// 			ib_link = (firebird_connection *)zend_fetch_resource2_ex(arg, LE_LINK, le_link, le_plink);

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
// 	firebird_connection *ib_link;
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

void dump_buffer(const unsigned char *buffer, int len){
    int i;
    for (i = 0; i < len; i++) {
        if(buffer[i] < 31 || buffer[i] > 126)
            php_printf("0x%02x ", buffer[i]);
        else
            php_printf("%c", buffer[i]);
    }
    if (i > 0) {
        php_printf("\n");
    }
}

void update_err_props_ex(ISC_STATUS_ARRAY status, zend_class_entry *class_ce, zend_object *obj, const char *file_name, size_t line_num)
{
    if (!(status[0] == 1 && status[1])){
        return;
    }

    zval rv;
    char msg[1024] = {0};
    char *s = msg;
    const ISC_STATUS* pstatus = status;

    while ((s - msg) < sizeof(msg) && fb_interpret(s, sizeof(msg) - (s - msg), &pstatus)) {
        s = msg + strlen(msg);
        *s++ = '\n';
    }

    zend_update_property_stringl(class_ce, obj, "error_msg", sizeof("error_msg") - 1, msg, s - msg - 1);
    zend_update_property_long(class_ce, obj, "error_code", sizeof("error_code") - 1, (zend_long)isc_sqlcode(status));
    zend_update_property_long(class_ce, obj, "error_code_long", sizeof("error_code_long") - 1,
        (zend_long)isc_portable_integer((const ISC_UCHAR*)&status[1], 4));

#ifdef PHP_DEBUG
    char *line;
    size_t line_len = spprintf(&line, 0, "%s:%d", file_name, line_num);
    zend_update_property_stringl(class_ce, obj, "error_line", sizeof("error_line") - 1, line, line_len);
    efree(line);
#endif
}


#endif /* HAVE_FIREBIRD */
