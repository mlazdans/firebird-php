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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#if HAVE_FIREBIRD

#include <time.h>
#include <firebird/fb_c_api.h>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/php_standard.h"
// #include "ext/standard/md5.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"
#include "SAPI.h"
#include "zend_interfaces.h"

#include "blob.h"
#include "database.h"
#include "transaction.h"
#include "statement.h"
#include "service.h"

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

    REGISTER_NS_LONG_CONSTANT("FireBird", "FETCH_BLOBS", FBP_FETCH_BLOBS, CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT("FireBird", "FETCH_UNIXTIME", FBP_FETCH_UNIXTIME, CONST_PERSISTENT);

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

void fbp_dump_buffer(int len, const unsigned char *buffer){
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

int fbp_status_err_msg(const ISC_STATUS *status, char *msg, unsigned short msg_size)
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
void fbp_status_error_ex(const ISC_STATUS *status, const char *file_name, size_t line_num)
{
    char msg[1024] = { 0 };
    fbp_status_err_msg(status, msg, sizeof(msg));
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
ISC_INT64 fbp_update_err_props_ex(ISC_STATUS_ARRAY status, zend_class_entry *ce, zval *obj, const char *file_name, size_t line_num)
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

void fbp_declare_props_from_zmap(zend_class_entry *ce, const firebird_xpb_zmap *xpb_zmap)
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

void fbp_insert_xpb_from_zmap(zend_class_entry *ce, zval *args, const firebird_xpb_zmap *xpb_zmap, struct IXpbBuilder* xpb, struct IStatus* st)
{
    zend_string *prop_name = NULL;
    zend_property_info *prop_info = NULL;
    zval rv, *val, *checkval;
    int i;

    for (int i = 0; i < xpb_zmap->count; i++) {
        prop_name = zend_string_init(xpb_zmap->names[i], strlen(xpb_zmap->names[i]), 1);

#ifdef PHP_DEBUG
        if (!zend_hash_exists(&ce->properties_info, prop_name)) {
            fbp_fatal("BUG! Property %s does not exist for %s::%s. Verify xpb_zmap",
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
                fbp_fatal("BUG! Unhandled: type %s for property %s::%s",
                    zend_get_type_by_const(Z_TYPE_P(val)), ZSTR_VAL(ce->name), xpb_zmap->names[i]);
                break;
        }
    }
}

void fbp_store_portable_integer(unsigned char *buffer, ISC_UINT64 value, int length)
{
    for (int i = 0; i < length; i++) {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }
}

// fbp_object_accessor(zend_fiber);
fbp_object_accessor(firebird_event);

#endif /* HAVE_FIREBIRD */
