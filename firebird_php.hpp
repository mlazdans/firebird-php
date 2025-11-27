#ifndef FIREBIRD_PHP_H
#define FIREBIRD_PHP_H

#include <vector>
#include <memory>

#define PHP_FIREBIRD_VERSION "0.0.1-alpha"
#define METADATALENGTH 63*4
// #define TPB_MAX_SIZE (32)

#define FIREBIRD_DATE_FMT "%Y-%m-%d"
#define FIREBIRD_TIME_FMT "%H:%M:%S"

namespace FBP {
class Database;
}

extern "C" {

#include "php.h"

extern zend_module_entry firebird_module_entry;
#define phpext_firebird_ptr &firebird_module_entry


#ifdef PHP_WIN32
// Case switch, because of troubles on Windows and PHP 8.0
#if PHP_VERSION_ID < 80000
   #define LL_MASK "I64"
#else
   #define LL_MASK "ll"
#endif
#define LL_LIT(lit) lit ## I64
typedef void (__stdcall *info_func_t)(char*);
#else
#define LL_MASK "ll"
#define LL_LIT(lit) lit ## ll
typedef void (*info_func_t)(char*);
#endif

PHP_MINIT_FUNCTION(firebird);
PHP_RINIT_FUNCTION(firebird);
PHP_MSHUTDOWN_FUNCTION(firebird);
PHP_RSHUTDOWN_FUNCTION(firebird);
PHP_MINFO_FUNCTION(firebird);

ZEND_BEGIN_MODULE_GLOBALS(firebird)
    // ISC_STATUS_ARRAY status;
    bool debug;
    std::vector<std::unique_ptr<FBP::Database>> db_list;
    // int has_error_handler;
    zend_fcall_info error_fci;
    zend_fcall_info_cache error_fcc;
ZEND_END_MODULE_GLOBALS(firebird)

ZEND_EXTERN_MODULE_GLOBALS(firebird)

#define FBG(v) ZEND_MODULE_GLOBALS_ACCESSOR(firebird, v)

#if defined(ZTS) && defined(COMPILE_DL_FIREBIRD)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define update_err_props(status, _nd1, _nd2)

#ifdef PHP_DEBUG
#define FBDEBUG(format, ...) if(FBG(debug))php_printf(format " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__);
#define FBDEBUG_NOFL(format, ...) if(FBG(debug))php_printf(format "\n" __VA_OPT__(,) __VA_ARGS__);
#else
#define FBDEBUG(...)
#define FBDEBUG_NOFL(...)
#endif

#define fbp_declare_object_accessor(strct)                    \
    strct *get_ ##strct## _from_obj(const zend_object *obj);  \
    strct *get_ ##strct## _from_zval(const zval *zv)

#define fbp_object_accessor(strct)                            \
    strct *get_ ##strct## _from_obj(const zend_object *obj) { \
        return obj ?                                          \
            (strct*)((char*)(obj) - XtOffsetOf(strct, std)) : \
            NULL;                                             \
    }                                                         \
    strct *get_ ##strct## _from_zval(const zval *zv) {        \
        return zv ?                                           \
            get_ ##strct## _from_obj(Z_OBJ_P(zv)) : NULL;     \
    }

// #define THIS_SET(prop, val) \
//     zend_update_property(Z_OBJCE_P(ZEND_THIS), Z_OBJ_P(ZEND_THIS), prop, sizeof(prop) - 1, val)
// #define THIS_GET(prop) \
//     zend_read_property(Z_OBJCE_P(ZEND_THIS), Z_OBJ_P(ZEND_THIS), prop, sizeof(prop) - 1, 0, &rv)
#define PROP_GET(ce, zval_obj, prop) \
    zend_read_property(ce, Z_OBJ_P(zval_obj), prop, sizeof(prop) - 1, 0, &rv)
#define PROP_SET(ce, zval_obj, prop, val) \
    zend_update_property(ce, Z_OBJ_P(zval_obj), prop, sizeof(prop) - 1, val)
#define PROP_SET_LONG(ce, zval_obj, prop, val) \
    zend_update_property_long(ce, Z_OBJ_P(zval_obj), prop, sizeof(prop) - 1, val)

void fbp_error_ex(long level, const char *, ...)
    PHP_ATTRIBUTE_FORMAT(printf,2,3);

#ifdef PHP_WIN32
#define fbp_fatal(msg, ...)   fbp_error_ex(E_ERROR,   msg " (%s:%d)\n", ## __VA_ARGS__, __FILE__, __LINE__)
#define fbp_warning(msg, ...) fbp_error_ex(E_WARNING, msg " (%s:%d)\n", ## __VA_ARGS__, __FILE__, __LINE__)
#define fbp_notice(msg, ...)  fbp_error_ex(E_NOTICE,  msg " (%s:%d)\n", ## __VA_ARGS__, __FILE__, __LINE__)
#else
#define fbp_fatal(msg, ...)   fbp_error_ex(E_ERROR,   msg " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__)
#define fbp_warning(msg, ...) fbp_error_ex(E_WARNING, msg " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__)
#define fbp_notice(msg, ...)  fbp_error_ex(E_NOTICE,  msg " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__)
#endif

#define TODO(msg) fbp_fatal("TODO: " msg)

typedef struct firebird_xpb_zmap {
    const uint8_t *tags;
    const char **names;
    uint32_t *ztypes;
    const short count;
} firebird_xpb_zmap;

#define _XPB_ZMAP_INIT(t, n, z) { \
    .tags = t,                    \
    .names = n,                   \
    .ztypes = z,                  \
    .count = ARRAY_SIZE(t)        \
}

#ifndef _MSC_VER
#define XPB_ZMAP_INIT(t, n, z) _XPB_ZMAP_INIT(t, n, z); \
static_assert(ARRAY_SIZE(t) == ARRAY_SIZE(n) && ARRAY_SIZE(n) == ARRAY_SIZE(z), "Array sizes do not match")
#else
#define XPB_ZMAP_INIT(t, n, z) _XPB_ZMAP_INIT(t, n, z)
#endif

#define update_ferr_props(ce, obj, error_msg, error_msg_len, error_code, error_code_long)                      \
    do {                                                                                                       \
        zend_update_property_stringl(ce, obj, "error_msg", sizeof("error_msg") - 1, error_msg, error_msg_len); \
        zend_update_property_long(ce, obj, "error_code", sizeof("error_code") - 1, error_code);                \
        zend_update_property_long(ce, obj, "error_code_long", sizeof("error_code_long") - 1, error_code_long); \
    } while(0)

// extern firebird_events fb_events;

// extern zend_class_entry *FireBird_IError_ce;
extern zend_class_entry *FireBird_Fb_Error_ce;
extern zend_class_entry *FireBird_Var_Info_ce;
extern zend_class_entry *FireBird_Event_ce;
extern zend_class_entry *FireBird_TBuilder_ce;
extern zend_class_entry *FireBird_Transaction_ce;
extern zend_class_entry *FireBird_Database_ce;
extern zend_class_entry *FireBird_Db_Info_ce;
extern zend_class_entry *FireBird_Connect_Args_ce;
extern zend_class_entry *FireBird_Create_Args_ce;
extern zend_class_entry *FireBird_Statement_ce;
extern zend_class_entry *FireBird_Blob_ce;
extern zend_class_entry *FireBird_Blob_Id_ce;
extern zend_class_entry *FireBird_Fb_Exception_ce;

void fbp_dump_buffer(int len, const unsigned char *buffer);
void fbp_dump_buffer_raw(int len, const unsigned char *buffer);

}

#endif // FIREBIRD_PHP_H
