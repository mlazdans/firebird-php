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

// ZEND_BEGIN_MODULE_GLOBALS(firebird)
//     ISC_STATUS_ARRAY status;
//     bool debug;
//     // int has_error_handler;
//     zend_fcall_info error_fci;
//     zend_fcall_info_cache error_fcc;
// ZEND_END_MODULE_GLOBALS(firebird)

// ZEND_EXTERN_MODULE_GLOBALS(firebird)


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
ISC_INT64 fbp_update_err_props_ex(ISC_STATUS_ARRAY status, zend_class_entry *ce, zval *obj, const char *file_name, size_t line_num);
ISC_INT64 fbp_call_error_handler(ISC_STATUS_ARRAY status, const char *file_name, size_t line_num);
void fbp_declare_props_from_zmap(zend_class_entry *ce, const firebird_xpb_zmap *xpb_zmap);
// void fbp_insert_xpb_from_zmap(zend_class_entry *ce, zval *args, const firebird_xpb_zmap *xpb_zmap, struct IXpbBuilder* xpb, struct IStatus* st);
void event_ast_routine(void *_ev, ISC_USHORT length, const ISC_UCHAR *result_buffer);

// fbp_declare_object_accessor(zend_fiber);
fbp_declare_object_accessor(firebird_event);

#define fbp_status_error(status) fbp_status_error_ex(status, __FILE__, __LINE__)

// #define update_err_props(status, ce, obj) fbp_update_err_props_ex(status, ce, obj, __FILE__, __LINE__)
// #define update_err_props(status, _nd1, _nd2) fbp_call_error_handler(status, __FILE__, __LINE__)


#ifdef __cplusplus
}
#endif

#endif /* PHP_FIREBIRD_INCLUDES_H */
