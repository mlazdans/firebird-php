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

#ifndef PHP_FIREBIRD_INCLUDES_H
#define PHP_FIREBIRD_INCLUDES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ibase.h>

#define PHP_FIREBIRD_VERSION "0.0.1-alpha"

#ifndef METADATALENGTH
#define METADATALENGTH 63*4
#endif

// #define TPB_MAX_SIZE (32)

// For limbo buffers. Need roughly 7 bytes per transaction id
#define TRANS_ID_SIZE 8
#define TRANS_MAX_STACK_COUNT 128

#define FIREBIRD_DATE_FMT "%Y-%m-%d"
#define FIREBIRD_TIME_FMT "%H:%M:%S"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// ZEND_BEGIN_MODULE_GLOBALS(firebird)
//     ISC_STATUS_ARRAY status;
//     bool debug;
//     // int has_error_handler;
//     zend_fcall_info error_fci;
//     zend_fcall_info_cache error_fcc;
// ZEND_END_MODULE_GLOBALS(firebird)

// ZEND_EXTERN_MODULE_GLOBALS(firebird)

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

typedef struct firebird_event {
    ISC_LONG event_id;
    isc_db_handle *db_handle;
    ISC_UCHAR *event_buffer, *result_buffer;
    ISC_USHORT buff_len;
    ISC_LONG posted_count;

    const char *name;
    // zend_fiber *fiber;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;

    zval retval;
    enum firebird_event_state { NEW, ACTIVE, DEAD } state;

    // zval instance;
    struct firebird_event *next;

    zend_object std;
} firebird_event;

typedef struct firebird_events {
    firebird_event *events;
    size_t count;
} firebird_events;

#define BLOB_ID_LEN     18
#define BLOB_ID_MASK    "0x%" LL_MASK "x"

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

#define DECLARE_PROP_OBJ(class_ce, name, obj_name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_CLASS(zend_string_init(#obj_name, sizeof(#obj_name)-1, 1), 0, 0), visibilty)
#define DECLARE_PROP_LONG(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_MASK(MAY_BE_LONG), visibilty)
#define DECLARE_PROP_BOOL(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_MASK(MAY_BE_BOOL), visibilty)
#define DECLARE_PROP_STRING(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_MASK(MAY_BE_STRING), visibilty)
#define DECLARE_PROP_ARRAY(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY), visibilty)
#define DECLARE_PROP(class_ce, name, type, visibilty) do {                            \
    zval prop_##name##_def_val;                                                       \
    ZVAL_UNDEF(&prop_##name##_def_val);                                               \
    zend_string *prop_##name##_name = zend_string_init(#name, sizeof(#name) - 1, 1);  \
    zend_declare_typed_property(class_ce, prop_##name##_name, &prop_##name##_def_val, \
        visibilty, NULL,                                                              \
        (zend_type) type);                                                            \
    zend_string_release(prop_##name##_name);                                          \
} while (0)

#define DECLARE_FERR_PROPS(ce)                                  \
    DECLARE_PROP_STRING(ce, error_msg, ZEND_ACC_PROTECTED_SET); \
    DECLARE_PROP_LONG(ce, error_code, ZEND_ACC_PROTECTED_SET);  \
    DECLARE_PROP_LONG(ce, error_code_long, ZEND_ACC_PROTECTED_SET)

#define DECLARE_IERR_PROPS(ce)                                   \
    DECLARE_PROP_STRING(ce, error_file, ZEND_ACC_PROTECTED_SET); \
    DECLARE_PROP_LONG(ce, error_lineno, ZEND_ACC_PROTECTED_SET); \
    DECLARE_PROP_ARRAY(ce, errors, ZEND_ACC_PROTECTED_SET);      \
    DECLARE_PROP_STRING(ce, sqlstate, ZEND_ACC_PROTECTED_SET);   \

#ifdef PHP_DEBUG
#define DECLARE_ERR_PROPS(ce)                                            \
    do {                                                                 \
        DECLARE_FERR_PROPS(ce);                                          \
        DECLARE_IERR_PROPS(ce);                                          \
        DECLARE_PROP_STRING(ce, ext_error_line, ZEND_ACC_PROTECTED_SET); \
    } while(0)
#else
#define DECLARE_ERR_PROPS(ce)   \
    do {                        \
        DECLARE_FERR_PROPS(ce); \
        DECLARE_IERR_PROPS(ce); \
    } while(0)
#endif

void fbp_store_portable_integer(unsigned char *buffer, ISC_UINT64 value, int length);
int fbp_get_status_err_msg(const ISC_STATUS *status, char *msg, unsigned short msg_size);
void fbp_status_error_ex(const ISC_STATUS *status, const char *file_name, size_t line_num);
void fbp_dump_buffer(int len, const unsigned char *buffer);
void fbp_dump_buffer_raw(int len, const unsigned char *buffer);
ISC_INT64 fbp_update_err_props_ex(ISC_STATUS_ARRAY status, zend_class_entry *ce, zval *obj, const char *file_name, size_t line_num);
ISC_INT64 fbp_call_error_handler(ISC_STATUS_ARRAY status, const char *file_name, size_t line_num);
void fbp_declare_props_from_zmap(zend_class_entry *ce, const firebird_xpb_zmap *xpb_zmap);
// void fbp_insert_xpb_from_zmap(zend_class_entry *ce, zval *args, const firebird_xpb_zmap *xpb_zmap, struct IXpbBuilder* xpb, struct IStatus* st);
void event_ast_routine(void *_ev, ISC_USHORT length, const ISC_UCHAR *result_buffer);

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

// fbp_declare_object_accessor(zend_fiber);
fbp_declare_object_accessor(firebird_event);

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

#define fbp_status_error(status) fbp_status_error_ex(status, __FILE__, __LINE__)

// #define update_err_props(status, ce, obj) fbp_update_err_props_ex(status, ce, obj, __FILE__, __LINE__)
// #define update_err_props(status, _nd1, _nd2) fbp_call_error_handler(status, __FILE__, __LINE__)

#define update_ferr_props(ce, obj, error_msg, error_msg_len, error_code, error_code_long)                      \
    do {                                                                                                       \
        zend_update_property_stringl(ce, obj, "error_msg", sizeof("error_msg") - 1, error_msg, error_msg_len); \
        zend_update_property_long(ce, obj, "error_code", sizeof("error_code") - 1, error_code);                \
        zend_update_property_long(ce, obj, "error_code_long", sizeof("error_code_long") - 1, error_code_long); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* PHP_FIREBIRD_INCLUDES_H */
