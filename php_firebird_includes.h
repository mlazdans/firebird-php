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

#include <firebird/fb_c_api.h>
#include <ibase.h>

#define PHP_FIREBIRD_VERSION "0.0.1-alpha"

#ifndef SQLDA_CURRENT_VERSION
#define SQLDA_CURRENT_VERSION SQLDA_VERSION1
#endif

#ifndef METADATALENGTH
#define METADATALENGTH 68
#endif

#define TPB_MAX_SIZE (32)

// For limbo buffers. Need roughly 7 bytes per transaction id
#define TRANS_ID_SIZE 8
#define TRANS_MAX_STACK_COUNT 128

#ifdef FIREBIRD_DEBUG
#define FBDEBUG(format, ...) if(FBG(debug))php_printf(format " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__);
#define FBDEBUG_NOFL(format, ...) if(FBG(debug))php_printf(format "\n" __VA_OPT__(,) __VA_ARGS__);
#else
#define FBDEBUG(args...)
#define FBDEBUG_NOFL(format, ...)
#endif

#define FIREBIRD_DATE_FMT "%Y-%m-%d"
#define FIREBIRD_TIME_FMT "%H:%M:%S"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

ZEND_BEGIN_MODULE_GLOBALS(firebird)
    bool debug;
ZEND_END_MODULE_GLOBALS(firebird)

ZEND_EXTERN_MODULE_GLOBALS(firebird)

typedef struct firebird_bind_buf {
    union {
#ifdef SQL_BOOLEAN
        FB_BOOLEAN bval;
#endif
        short sval;
        float fval;
        ISC_LONG lval;
        ISC_QUAD qval;
        ISC_TIMESTAMP tsval;
        ISC_DATE dtval;
        ISC_TIME tmval;
    } val;
    short sqlind;
} firebird_bind_buf;

typedef struct {
    ISC_ARRAY_DESC ar_desc;
    ISC_LONG ar_size; /* size of entire array in bytes */
    unsigned short el_type, el_size;
} firebird_array;

typedef struct firebird_db {
    isc_db_handle db_handle;
    zval args;

    zend_object std;
} firebird_db;

typedef struct firebird_service {
    isc_svc_handle svc_handle;
    zval args;
    // char *hostname;
    // char *username;
    // zend_resource *res;

    // zval instance;
    zend_object std;
} firebird_service;

typedef struct firebird_trans {
    isc_tr_handle tr_handle;
    isc_db_handle *db_handle;
    ISC_UINT64 tr_id;
    unsigned short is_prepared_2pc;
    zend_object std;
} firebird_trans;

typedef struct firebird_stmt {
    isc_stmt_handle stmt_handle;
    isc_db_handle *db_handle;
    isc_tr_handle *tr_handle;
    XSQLDA *in_sqlda, *out_sqlda;
    unsigned char statement_type, has_more_rows;
    firebird_array *in_array, *out_array;
    unsigned short in_array_cnt, out_array_cnt;
    firebird_bind_buf *bind_buf;
    // unsigned short dialect;
    const ISC_SCHAR *query;
    const ISC_SCHAR *name;
    ISC_ULONG insert_count, update_count, delete_count, affected_count;
    zend_object std;
} firebird_stmt;

typedef struct firebird_blob {
    isc_blob_handle bl_handle;
    isc_db_handle *db_handle;
    isc_tr_handle *tr_handle;
    ISC_QUAD bl_id;
    ISC_LONG max_segment;
    ISC_LONG num_segments;
    ISC_LONG total_length;
    ISC_LONG type; // 0 - segmented, 1 - streamed

    unsigned short is_writable;

    zend_object std;
} firebird_blob;

typedef struct firebird_blob_id {
    ISC_QUAD bl_id;
    zend_object std;
} firebird_blob_id;

typedef struct firebird_vary {
    unsigned short vary_length;
    char vary_string[1];
} firebird_vary;

typedef struct firebird_xpb_zmap {
    const char *tags, **names;
    uint32_t *ztypes;
    const short count;
} firebird_xpb_zmap;

#define XPB_ZMAP_INIT(t, n, z) (firebird_xpb_zmap) { \
    .tags = t,                \
    .names = n,               \
    .ztypes = z,              \
    .count = ARRAY_SIZE(t)    \
};                            \
_Static_assert(ARRAY_SIZE(t) == ARRAY_SIZE(n) && ARRAY_SIZE(n) == ARRAY_SIZE(z), "Array sizes do not match");

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

    zval instance;
    struct firebird_event *next;

    zend_object std;
} firebird_event;

typedef struct firebird_events {
    firebird_event *events;
    size_t count;
} firebird_events;

typedef struct firebird_tbuilder {
    char read_only, ignore_limbo, auto_commit, no_auto_undo;

    /**
     * Isolation mode (level):
     * 0 - consistency (snapshot table stability)
     * 1 - concurrency (snapshot)
     * 2 - read committed record version
     * 3 - read committed no record version
     * 4 - read committed read consistency
     */
    char isolation_mode;
    ISC_SHORT lock_timeout;
    ISC_UINT64 snapshot_at_number;

    zend_object std;
} firebird_tbuilder;

enum php_firebird_option {
    /* fetch flags */
    PHP_FIREBIRD_FETCH_BLOBS        = 1,
    PHP_FIREBIRD_FETCH_ARRAYS       = 2,
    PHP_FIREBIRD_UNIXTIME           = 4,
};

#define FBG(v) ZEND_MODULE_GLOBALS_ACCESSOR(firebird, v)

#if defined(ZTS) && defined(COMPILE_DL_FIREBIRD)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

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

void _php_firebird_module_error(char *, ...)
    PHP_ATTRIBUTE_FORMAT(printf,1,2);
void _php_firebird_module_fatal(char *, ...)
    PHP_ATTRIBUTE_FORMAT(printf,1,2);

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define OBJ_GET(ce, zobj, prop, rv) \
    zend_read_property(ce, Z_OBJ_P(zobj), prop, sizeof(prop) - 1, 0, rv)

#define OBJ_SET(ce, zobj, prop, val) zend_update_property(ce, Z_OBJ_P(zobj), prop, sizeof(prop) - 1, val)
#define OBJ_SET_LONG(ce, zobj, prop, val) zend_update_property_long(ce, Z_OBJ_P(zobj), prop, sizeof(prop) - 1, val)

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

// TODO: these macros are very bad idea because not type checking. It is easy to
// make mistakes calling these on *zend_object or *zval and vice versa
#define Z_TRANSACTION_O(zobj) \
    ((firebird_trans*)((char*)(zobj) - XtOffsetOf(firebird_trans, std)))

#define Z_STMT_O(zobj) \
    ((firebird_stmt*)((char*)(zobj) - XtOffsetOf(firebird_stmt, std)))

#define Z_DB_O(zobj) \
    ((firebird_db*)((char*)(zobj) - XtOffsetOf(firebird_db, std)))

#define Z_BLOB_O(zobj) \
    ((firebird_blob*)((char*)(zobj) - XtOffsetOf(firebird_blob, std)))

#define Z_BLOB_ID_O(zobj) \
    ((firebird_blob_id*)((char*)(zobj) - XtOffsetOf(firebird_blob_id, std)))

#define Z_FIBER_O(zobj) \
    ((zend_fiber*)((char*)(zobj) - XtOffsetOf(zend_fiber, std)))

#define Z_EVENT_O(zobj) \
    ((firebird_event*)((char*)(zobj) - XtOffsetOf(firebird_event, std)))

#define Z_SERVICE_O(zobj) \
    ((firebird_service*)((char*)(zobj) - XtOffsetOf(firebird_service, std)))

#define Z_TBUILDER_O(zobj) \
    ((firebird_tbuilder*)((char*)(zobj) - XtOffsetOf(firebird_tbuilder, std)))

#define Z_TRANSACTION_P(zv) Z_TRANSACTION_O(Z_OBJ_P(zv))
#define Z_STMT_P(zv) Z_STMT_O(Z_OBJ_P(zv))
#define Z_DB_P(zv) Z_DB_O(Z_OBJ_P(zv))
#define Z_BLOB_P(zv) Z_BLOB_O(Z_OBJ_P(zv))
#define Z_BLOB_ID_P(zv) Z_BLOB_ID_O(Z_OBJ_P(zv))
#define Z_FIBER_P(zv) Z_FIBER_O(Z_OBJ_P(zv))
#define Z_EVENT_P(zv) Z_EVENT_O(Z_OBJ_P(zv))
#define Z_SERVICE_P(zv) Z_SERVICE_O(Z_OBJ_P(zv))
#define Z_TBUILDER_P(zv) Z_TBUILDER_O(Z_OBJ_P(zv))

// TODO: similar macros for reading
#define xpb_insert(f, ...) do { \
    IXpbBuilder_insert##f(__VA_ARGS__); \
    if (IStatus_getState(st) & IStatus_STATE_ERRORS) { \
        char _st_msg[1024] = {0}; \
        status_err_msg(IStatus_getErrors(st), _st_msg, sizeof(_st_msg)); \
        _php_firebird_module_fatal(_st_msg); \
    } \
} while(0)

#define xpb_insert_true(tag) xpb_insert(Int, xpb, st, tag, (char)1)
#define xpb_insert_false(tag) xpb_insert(Int, xpb, st, tag, (char)0)
#define xpb_insert_int(tag, value) xpb_insert(Int, xpb, st, tag, value)
#define xpb_insert_string(tag, value) xpb_insert(String, xpb, st, tag, value)
#define xpb_insert_tag(tag) xpb_insert(Tag, xpb, st, tag)

// General argument types
ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_void, 0, 0, MAY_BE_VOID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_static, 0, 0, MAY_BE_STATIC)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_none_return_bool, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bool_return_none, 0, 0, 0)
    ZEND_ARG_TYPE_INFO(0, enable, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

// Database argument types
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Database_connect, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_OBJ_INFO(0, args, FireBird\\Connect_Args, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Database_create, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_OBJ_INFO(0, args, FireBird\\Create_Args, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Database_get_info, 0, 0, FireBird\\Db_Info, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Database_on_event, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    // ZEND_ARG_OBJ_INFO(0, f, Fiber, 0)
    ZEND_ARG_TYPE_INFO(0, f, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_INFO_EX(arginfo_FireBird_Database_new_transaction, 0, 0, FireBird\\Transaction, 0)
    ZEND_ARG_OBJ_INFO(0, args, FireBird\\TBuilder, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Database_reconnect_transaction, 0, 1, FireBird\\Transaction, MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, id, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_Database_get_limbo_transactions, 0, 1, MAY_BE_ARRAY|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, max_count, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

// Service argument types
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Service_connect, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_OBJ_INFO(0, args, FireBird\\Service_Connect_Args, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Service_get_server_info, 0, 0, FireBird\\Server_Info, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Service_add_user, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_OBJ_INFO(0, user_info, FireBird\\Server_User_Info, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Service_delete_user, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, username, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Service_backup, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, bkp_file, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Service_restore, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, bkp_file, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

// Transaction argument types
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Transaction_prepare, 0, 0, FireBird\\Statement, MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Transaction_query, 0, 1, FireBird\\Statement, MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
    ZEND_ARG_VARIADIC_INFO(0, bind_args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Transaction_open_blob, 0, 1, FireBird\\Blob, MAY_BE_FALSE)
    ZEND_ARG_OBJ_INFO(0, args, FireBird\\Blob_id, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Transaction_create_blob, 0, 0, FireBird\\Blob, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

// Statement argument types
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_Statement_fetch_row, 0, 0, MAY_BE_ARRAY|MAY_BE_FALSE|MAY_BE_NULL)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_Statement_fetch_object, 0, 0, MAY_BE_OBJECT|MAY_BE_FALSE|MAY_BE_NULL)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Statement_execute, 0, 0, _IS_BOOL, 0)
    ZEND_ARG_VARIADIC_INFO(0, bind_args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Statement_get_var_info_in_out, 0, 1, FireBird\\Var_Info, MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, num, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Statement_set_name, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

// Blob argument types
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Blob_info, 0, 0, FireBird\\Blob_Info, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Blob_get, 0, 0, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, max_len, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Blob_put, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

// TBuilder argument types
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_TBuilder_wait, 0, 0, MAY_BE_STATIC)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, lock_timeout, IS_LONG, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_TBuilder_isolation_snapshot, 0, 0, MAY_BE_STATIC)
    ZEND_ARG_TYPE_INFO(0, at_number, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_TBuilder_flag_return_static, 0, 0, MAY_BE_STATIC)
    ZEND_ARG_TYPE_INFO(0, enable, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

extern firebird_xpb_zmap database_create_zmap;
extern firebird_xpb_zmap database_connect_zmap;
extern firebird_xpb_zmap database_info_zmap;
extern firebird_xpb_zmap service_connect_zmap;
extern firebird_xpb_zmap server_info_zmap;
extern firebird_xpb_zmap user_info_zmap;
extern firebird_events fb_events;

extern zend_class_entry *FireBird_Connect_Args_ce;
extern zend_class_entry *FireBird_Create_Args_ce;
extern zend_class_entry *FireBird_Database_ce;
extern zend_class_entry *FireBird_Transaction_ce;
extern zend_class_entry *FireBird_Statement_ce;
extern zend_class_entry *FireBird_IError_ce;
extern zend_class_entry *FireBird_Error_ce;
extern zend_class_entry *FireBird_Blob_ce;
extern zend_class_entry *FireBird_Blob_Info_ce;
extern zend_class_entry *FireBird_Blob_Id_ce;
extern zend_class_entry *FireBird_Var_Info_ce;
extern zend_class_entry *FireBird_Db_Info_ce;
extern zend_class_entry *FireBird_Event_ce;
extern zend_class_entry *FireBird_Service_ce;
extern zend_class_entry *FireBird_Service_Connect_Args_ce;
extern zend_class_entry *FireBird_Server_Info_ce;
extern zend_class_entry *FireBird_Server_Db_Info_ce;
extern zend_class_entry *FireBird_Server_User_Info_ce;
extern zend_class_entry *FireBird_TBuilder_ce;

extern void register_FireBird_Database_ce();
extern void register_FireBird_Transaction_ce();
extern void register_FireBird_Statement_ce();
extern void register_FireBird_IError_ce();
extern void register_FireBird_Error_ce();
extern void register_FireBird_Connect_Args_ce();
extern void register_FireBird_Create_Args_ce();
extern void register_FireBird_Blob_ce();
extern void register_FireBird_Blob_Info_ce();
extern void register_FireBird_Blob_Id_ce();
extern void register_FireBird_Var_Info_ce();
extern void register_FireBird_Db_Info_ce();
extern void register_FireBird_Event_ce();
extern void register_FireBird_Service_ce();
extern void register_FireBird_Service_Connect_Args_ce();
extern void register_FireBird_Server_Info_ce();
extern void register_FireBird_Server_Db_Info_ce();
extern void register_FireBird_Server_User_Info_ce();
extern void register_FireBird_TBuilder_ce();

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

// TODO: tidy namspacing
void store_portable_integer(unsigned char *buffer, ISC_UINT64 value, int length);
void transaction_ctor(firebird_trans *tr, firebird_db *db);
void transaction__construct(zval *tr, zval *database, zval *builder);
void status_fbp_error_ex(const ISC_STATUS *status, const char *file_name, size_t line_num);
void dump_buffer(const unsigned char *buffer, int len);
ISC_INT64 update_err_props_ex(ISC_STATUS_ARRAY status, zend_class_entry *ce, zval *obj, const char *file_name, size_t line_num);
int transaction_start(ISC_STATUS_ARRAY status, zval *tr_o);
int transaction_get_info(ISC_STATUS_ARRAY status, firebird_trans *tr);
int status_err_msg(const ISC_STATUS *status, char *msg, unsigned short msg_size);
int database_build_dpb(zend_class_entry *ce, zval *args_o, const firebird_xpb_zmap *xpb_map, const char **dpb_buf, short *num_dpb_written);
void statement_ctor(zval *stmt_o, zval *transaction);
int statement_prepare(ISC_STATUS_ARRAY status, zval *stmt_o, const ISC_SCHAR *sql);
int statement_execute(zval *stmt_o, zval *bind_args, uint32_t num_bind_args, zend_class_entry *ce, zval *ce_o);
int statement_info(ISC_STATUS_ARRAY status, firebird_stmt *stmt);
void declare_props_zmap(zend_class_entry *ce, const firebird_xpb_zmap *xpb_zmap);
void xpb_insert_zmap(zend_class_entry *ce, zval *args, const firebird_xpb_zmap *xpb_zmap, struct IXpbBuilder* xpb, struct IStatus* st);
void blob_ctor(firebird_blob *blob, isc_db_handle *db_handle, isc_tr_handle *tr_handle);
void blob___construct(zval *blob_o, zval *transaction);
int blob_get_info(ISC_STATUS_ARRAY status, firebird_blob *blob);
int blob_create(ISC_STATUS_ARRAY status, firebird_blob *blob);
int blob_open(ISC_STATUS_ARRAY status, firebird_blob *blob);
int blob_get(ISC_STATUS_ARRAY status, firebird_blob *blob, zval *return_value, size_t max_len);
int blob_close(ISC_STATUS_ARRAY status, firebird_blob *blob);
int blob_put(ISC_STATUS_ARRAY status, firebird_blob *blob, const char *buf, size_t buf_size);
void blob_id___construct(zval *blob_id_o, ISC_QUAD bl_id);
int database_get_info(ISC_STATUS_ARRAY status, isc_db_handle *db_handle, zval *db_info,
    size_t info_req_size, char *info_req, size_t info_resp_size, char *info_resp, size_t max_limbo_count);
void event_ast_routine(void *_ev, ISC_USHORT length, const ISC_UCHAR *result_buffer);
void tbuilder_populate_tpb(firebird_tbuilder *builder, char *tpb, unsigned short *tpb_len);

#define fbp_error(msg, ...) _php_firebird_module_error(msg " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__)
#define fbp_fatal(msg, ...) _php_firebird_module_fatal(msg " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__)

#define status_fbp_error(status) status_fbp_error_ex(status, __FILE__, __LINE__)
#define update_err_props(status, ce, obj) update_err_props_ex(status, ce, obj, __FILE__, __LINE__)
#define update_ferr_props(ce, obj, error_msg, error_msg_len, error_code, error_code_long)                      \
    do {                                                                                                       \
        zend_update_property_stringl(ce, obj, "error_msg", sizeof("error_msg") - 1, error_msg, error_msg_len); \
        zend_update_property_long(ce, obj, "error_code", sizeof("error_code") - 1, error_code);                \
        zend_update_property_long(ce, obj, "error_code_long", sizeof("error_code_long") - 1, error_code_long); \
    } while(0)

// C++ experiments
// #ifdef __cplusplus
// extern "C" {
// #endif
// int fb_connect(zend_class_entry *class_ce, zend_object *obj);
// #ifdef __cplusplus
// }
// #endif

#endif /* PHP_FIREBIRD_INCLUDES_H */
