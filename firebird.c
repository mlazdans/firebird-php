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

#include <firebird/fb_c_api.h>
#include "php.h"
#include "blob.h"

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

firebird_xpb_zmap database_create_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_dpb_user_name, isc_dpb_password, isc_dpb_set_db_charset, isc_dpb_sweep_interval,
        isc_dpb_set_page_buffers, isc_dpb_page_size, isc_dpb_force_write, isc_dpb_overwrite,
        isc_dpb_connect_timeout
    }),
    ((const char *[]){
        "user_name", "password", "set_db_charset", "sweep_interval",
        "set_page_buffers", "page_size", "force_write", "overwrite",
        "timeout"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_BOOL, MAY_BE_BOOL,
        MAY_BE_LONG
    })
);

firebird_xpb_zmap database_connect_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_dpb_user_name, isc_dpb_password, isc_dpb_lc_ctype, isc_dpb_sql_role_name, isc_dpb_num_buffers, isc_dpb_connect_timeout
    }),
    ((const char *[]){
        "user_name", "password", "charset", "role_name", "num_buffers", "timeout"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_LONG, MAY_BE_LONG
    })
);

firebird_xpb_zmap service_connect_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_spb_user_name, isc_spb_password
    }),
    ((const char *[]){
        "user_name", "password"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING
    })
);

firebird_xpb_zmap database_info_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_info_reads, isc_info_writes, isc_info_fetches, isc_info_marks,
        isc_info_page_size, isc_info_num_buffers, isc_info_current_memory, isc_info_max_memory,

        isc_info_allocation, isc_info_attachment_id, isc_info_read_seq_count, isc_info_read_idx_count, isc_info_insert_count,
        isc_info_update_count, isc_info_delete_count, isc_info_backout_count, isc_info_purge_count, isc_info_expunge_count,

        isc_info_isc_version, isc_info_firebird_version, isc_info_limbo,
        isc_info_sweep_interval, isc_info_ods_version, isc_info_ods_minor_version, isc_info_no_reserve,

        isc_info_forced_writes, isc_info_set_page_buffers, isc_info_db_sql_dialect, isc_info_db_read_only, isc_info_db_size_in_pages,
        isc_info_oldest_transaction, isc_info_oldest_active, isc_info_oldest_snapshot, isc_info_next_transaction,

        isc_info_creation_date, isc_info_db_file_size, fb_info_pages_used, fb_info_pages_free,

        fb_info_ses_idle_timeout_db, fb_info_ses_idle_timeout_att, fb_info_ses_idle_timeout_run, fb_info_conn_flags,
        fb_info_crypt_key, fb_info_crypt_state, fb_info_statement_timeout_db, fb_info_statement_timeout_att,
        fb_info_protocol_version, fb_info_crypt_plugin, fb_info_wire_crypt,

        fb_info_next_attachment, fb_info_next_statement, fb_info_db_guid, fb_info_db_file_id,
        fb_info_replica_mode, fb_info_username, fb_info_sqlrole, fb_info_parallel_workers
    }),
    ((const char *[]){
        "reads", "writes", "fetches", "marks",
        "page_size", "num_buffers", "current_memory", "max_memory",

        "allocation", "attachment_id", "read_seq_count", "read_idx_count", "insert_count",
        "update_count", "delete_count", "backout_count", "purge_count", "expunge_count",

        "isc_version", "firebird_version", "limbo",
        "sweep_interval", "ods_version", "ods_minor_version", "no_reserve",

        "forced_writes", "set_page_buffers", "db_sql_dialect", "db_read_only", "db_size_in_pages",
        "oldest_transaction", "oldest_active", "oldest_snapshot", "next_transaction",

        "creation_date", "db_file_size", "pages_used", "pages_free",

        "ses_idle_timeout_db", "ses_idle_timeout_att", "ses_idle_timeout_run", "conn_flags",
        "crypt_key", "crypt_state", "statement_timeout_db", "statement_timeout_att", "protocol_version", "crypt_plugin", "wire_crypt",

        "next_attachment", "next_statement", "db_guid", "db_file_id",
        "replica_mode", "username", "sqlrole", "parallel_workers"

    }),
    ((uint32_t []) {
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_ARRAY, MAY_BE_ARRAY, MAY_BE_ARRAY,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_STRING, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_STRING, MAY_BE_STRING,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_LONG,
    })
);

firebird_xpb_zmap server_info_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_info_svc_server_version, isc_info_svc_implementation, isc_info_svc_get_env, isc_info_svc_get_env_lock, isc_info_svc_get_env_msg,
        isc_info_svc_user_dbpath
    }),
    ((const char *[]){
        "server_version", "implementation", "get_env", "get_env_lock", "get_env_msg",
        "user_dbpath"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING,
        MAY_BE_STRING
    })
);

firebird_xpb_zmap user_info_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_spb_sec_username, isc_spb_sec_password, isc_spb_sec_firstname, isc_spb_sec_middlename, isc_spb_sec_lastname, isc_spb_sec_admin
    }),
    ((const char *[]){
        "username", "password", "firstname", "middlename", "lastname", "admin"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_TRUE | MAY_BE_FALSE,
    })
);

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

ZEND_INI_MH(OnUpdateDebug)
{
    int *p;
    zend_long tmp;

    tmp = zend_ini_parse_quantity_warn(new_value, entry->name);
    if (tmp < 0 || tmp > INT_MAX) {
        return FAILURE;
    }

    p = (int *) ZEND_INI_GET_ADDR();
    *p = (int) tmp;

    return SUCCESS;
}

/* {{{ startup, shutdown and info functions */
PHP_INI_BEGIN()
    // PHP_INI_ENTRY_EX("ibase.allow_persistent", "1", PHP_INI_SYSTEM, NULL, zend_ini_boolean_displayer_cb)
    // PHP_INI_ENTRY_EX("ibase.max_persistent", "-1", PHP_INI_SYSTEM, NULL, display_link_numbers)
    // PHP_INI_ENTRY_EX("ibase.max_links", "-1", PHP_INI_SYSTEM, NULL, display_link_numbers)
    // PHP_INI_ENTRY("ibase.default_db", NULL, PHP_INI_SYSTEM, NULL)
    // PHP_INI_ENTRY("ibase.default_user", NULL, PHP_INI_ALL, NULL)
    // PHP_INI_ENTRY_EX("ibase.default_password", NULL, PHP_INI_ALL, NULL, php_firebird_password_displayer_cb)

    // ZEND_INI_ENTRY(name, default_value, modifiable, on_modify)
    STD_PHP_INI_ENTRY("firebird.debug", "0", PHP_INI_ALL, OnUpdateDebug, debug, zend_firebird_globals, firebird_globals)
    PHP_INI_ENTRY("firebird.default_charset", NULL, PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("firebird.timestampformat", FIREBIRD_DATE_FMT " " FIREBIRD_TIME_FMT, PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("firebird.dateformat", FIREBIRD_DATE_FMT, PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("firebird.timeformat", FIREBIRD_TIME_FMT, PHP_INI_ALL, NULL)
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

#ifdef ZEND_SIGNALS
    // firebird replaces some signals at runtime, suppress warnings.
    SIGG(check) = 0;
#endif

    register_FireBird_Connect_Args_ce();
    register_FireBird_Create_Args_ce();
    register_FireBird_IError_ce();
    register_FireBird_Error_ce();
    register_FireBird_Database_ce();
    register_FireBird_Transaction_ce();
    register_FireBird_Statement_ce();
    register_FireBird_Blob_ce();
    register_FireBird_Blob_Id_ce();
    register_FireBird_Var_Info_ce();
    register_FireBird_Db_Info_ce();
    register_FireBird_Event_ce();
    register_FireBird_Service_ce();
    register_FireBird_Service_Connect_Args_ce();
    register_FireBird_Server_Info_ce();
    register_FireBird_Server_Db_Info_ce();
    register_FireBird_Server_User_Info_ce();
    register_FireBird_TBuilder_ce();

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

int status_err_msg(const ISC_STATUS *status, char *msg, unsigned short msg_size)
{
    char *s = msg;
    const ISC_STATUS* pstatus = status;

    while ((s - msg) < msg_size && fb_interpret(s, msg_size - (s - msg), &pstatus)) {
        s = msg + strlen(msg);
        *s++ = '\n';
    }

    // strip last newline
    if (s - msg > 0) {
        *--s = '\0';
        return s - msg;
    } else {
        return 0;
    }
}

// Use this when object is beeing destroyed but some clean-up errors happen
void status_fbp_error_ex(const ISC_STATUS *status, const char *file_name, size_t line_num)
{
    char msg[1024] = { 0 };
    status_err_msg(status, msg, sizeof(msg));
#ifdef PHP_DEBUG
    _php_firebird_module_error("firebird-php error: %s (%s:%d)\n", msg, file_name, line_num);
#else
    _php_firebird_module_error("firebird-php error: %s", msg);
#endif
}

#define UPD_CODES() \
    error_code = isc_sqlcode(&pstatus[0]); \
    error_code_long = isc_portable_integer((const ISC_UCHAR*)(&pstatus[1]), 4)

// Returns last error_code_long from the status array
ISC_INT64 update_err_props_ex(ISC_STATUS_ARRAY status, zend_class_entry *ce, zval *obj, const char *file_name, size_t line_num)
{
    if (!(status[0] == 1 && status[1])){
        return 0;
    }

    char sqlstate[6];  // SQLSTATE codes are 5 characters + 1 for null terminator
    char msg[1024] = { 0 };
    char *s = msg;

    zval rv, errors, hash_entry, ferror_o;
    HashTable *ht_errors;
    int errors_count = 0;

    const ISC_STATUS* pstatus = status;
    ISC_LONG error_code = 0, s_len = 0;
    ISC_INT64 error_code_long = 0;

    array_init_size(&errors, 20);
    ht_errors = Z_ARRVAL(errors);

    UPD_CODES();

    do {
        if (!(s - msg < sizeof(msg) && (s_len = fb_interpret(s, sizeof(msg) - (s - msg), &pstatus)))) {
            break;
        }

        if((&pstatus[0])[0]) {
            UPD_CODES();
        }

        object_init_ex(&ferror_o, FireBird_Error_ce);
        update_ferr_props(FireBird_Error_ce, Z_OBJ(ferror_o), s, s_len, error_code, error_code_long);
        zend_hash_next_index_insert(ht_errors, &ferror_o);
        errors_count++;

        s += s_len;
        *s++ = '\n';
    } while(1);

    if(errors_count <= 0) {
        zval_ptr_dtor(&errors);
        return error_code_long;
    }

    update_ferr_props(ce, Z_OBJ_P(obj), msg, s - msg - 1, error_code, error_code_long);

    zend_update_property_string(ce, Z_OBJ_P(obj), "error_file", sizeof("error_file") - 1, zend_get_executed_filename());
    zend_update_property_long(ce, Z_OBJ_P(obj), "error_lineno", sizeof("error_lineno") - 1, zend_get_executed_lineno());

    zend_update_property(ce, Z_OBJ_P(obj), "errors", sizeof("errors") - 1, &errors);
    zval_ptr_dtor(&errors);

#ifdef PHP_DEBUG
    char *line;
    size_t line_len = spprintf(&line, 0, "%s:%d", file_name, line_num);
    zend_update_property_stringl(ce, Z_OBJ_P(obj), "ext_error_line", sizeof("ext_error_line") - 1, line, line_len);
    efree(line);
#endif

    fb_sqlstate(sqlstate, status);
    zend_update_property_string(ce, Z_OBJ_P(obj), "sqlstate", sizeof("sqlstate") - 1, sqlstate);

    return error_code_long;
}

void _php_firebird_module_error(char *msg, ...)
{
    va_list ap;
    char buf[1024] = { 0 };

    va_start(ap, msg);

    /* vsnprintf NUL terminates the buf and writes at most n-1 chars+NUL */
    vsnprintf(buf, sizeof(buf), msg, ap);
    va_end(ap);

    // IBG(sql_code) = -999; /* no SQL error */

    php_error_docref(NULL, E_WARNING, "%s", buf);
}

void _php_firebird_module_fatal(char *msg, ...)
{
    va_list ap;
    char buf[1024] = {0};

    va_start(ap, msg);

    /* vsnprintf NUL terminates the buf and writes at most n-1 chars+NUL */
    vsnprintf(buf, sizeof(buf), msg, ap);
    va_end(ap);

    // IBG(sql_code) = -999; /* no SQL error */

    php_error_docref(NULL, E_ERROR, "%s", buf);
}

void declare_props_zmap(zend_class_entry *ce, const firebird_xpb_zmap *xpb_zmap)
{
    for (int i = 0; i < xpb_zmap->count; i++) {
        zval prop_def_val;
        ZVAL_UNDEF(&prop_def_val);
        zend_string *prop_name = zend_string_init(xpb_zmap->names[i], strlen(xpb_zmap->names[i]), 1);
        zend_declare_typed_property(ce, prop_name, &prop_def_val,
            ZEND_ACC_PUBLIC, NULL,
            (zend_type) ZEND_TYPE_INIT_MASK(xpb_zmap->ztypes[i]));
        zend_string_release(prop_name);
    }
}

void xpb_insert_zmap(zend_class_entry *ce, zval *args, const firebird_xpb_zmap *xpb_zmap, struct IXpbBuilder* xpb, struct IStatus* st)
{
    zend_string *prop_name = NULL;
    zend_property_info *prop_info = NULL;
    zval rv, *val, *checkval;
    int i;

    for (int i = 0; i < xpb_zmap->count; i++) {
        prop_name = zend_string_init(xpb_zmap->names[i], strlen(xpb_zmap->names[i]), 1);

#ifdef PHP_DEBUG
        if (!zend_hash_exists(&ce->properties_info, prop_name)) {
            _php_firebird_module_fatal("BUG! Property %s does not exist for %s::%s. Verify xpb_zmap",
                xpb_zmap->names[i], ZSTR_VAL(ce->name), xpb_zmap->names[i]);
            zend_string_release(prop_name);
            continue;
        }
#endif

        prop_info = zend_get_property_info(ce, prop_name, 0);
        checkval = OBJ_PROP(Z_OBJ_P(args), prop_info->offset);
        if (Z_ISUNDEF_P(checkval)) {
            FBDEBUG("property: %s is uninitialized", xpb_zmap->names[i]);
            zend_string_release(prop_name);
            continue;
        }

        val = zend_read_property_ex(ce, Z_OBJ_P(args), prop_name, 0, &rv);
        zend_string_release(prop_name);

        switch (Z_TYPE_P(val)) {
            case IS_STRING:
                FBDEBUG("property: %s is string: `%s`", xpb_zmap->names[i], Z_STRVAL_P(val));
                xpb_insert_string(xpb_zmap->tags[i], Z_STRVAL_P(val));
                break;
            case IS_LONG:
                FBDEBUG("property: %s is long: `%u`", xpb_zmap->names[i], Z_LVAL_P(val));
                xpb_insert_int(xpb_zmap->tags[i], (int)Z_LVAL_P(val));
                break;
            case IS_TRUE:
                FBDEBUG("property: %s is true", xpb_zmap->names[i]);
                xpb_insert_true(xpb_zmap->tags[i]);
                break;
            case IS_FALSE:
                FBDEBUG("property: %s is false", xpb_zmap->names[i]);
                xpb_insert_false(xpb_zmap->tags[i]);
                break;
            case IS_NULL:
                FBDEBUG("property: %s is null", xpb_zmap->names[i]);
                break;
            default:
                _php_firebird_module_fatal("BUG! Unhandled: type %s for property %s::%s",
                    zend_get_type_by_const(Z_TYPE_P(val)), ZSTR_VAL(ce->name), xpb_zmap->names[i]);
                break;
        }
    }
}

void store_portable_integer(unsigned char *buffer, ISC_UINT64 value, int length)
{
    for (int i = 0; i < length; i++) {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }
}

#define fbp_object_accessor(strct)                                               \
    zend_always_inline strct *get_ ##strct## _from_obj(const zend_object *obj) { \
        return (strct*)((char*)(obj) - XtOffsetOf(strct, std));                  \
    }                                                                            \
    zend_always_inline strct *get_ ##strct## _from_zval(const zval *zv) {        \
        return get_ ##strct## _from_obj(Z_OBJ_P(zv));                            \
    }

fbp_object_accessor(firebird_trans)
fbp_object_accessor(firebird_stmt)
fbp_object_accessor(firebird_db);
fbp_object_accessor(firebird_blob);
fbp_object_accessor(firebird_blob_id);
// fbp_object_accessor(zend_fiber);
fbp_object_accessor(firebird_event);
fbp_object_accessor(firebird_service);
fbp_object_accessor(firebird_tbuilder);

#endif /* HAVE_FIREBIRD */
