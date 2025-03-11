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

#include <ibase.h>

#define PHP_FIREBIRD_VERSION "0.0.1-alpha"

#ifndef SQLDA_CURRENT_VERSION
#define SQLDA_CURRENT_VERSION SQLDA_VERSION1
#endif

#ifndef METADATALENGTH
#define METADATALENGTH 68
#endif

#define TPB_MAX_SIZE (8*sizeof(char))

// #define RESET_ERRMSG do { IBG(errmsg)[0] = '\0'; IBG(sql_code) = 0; } while (0)

// #define IB_STATUS (IBG(status))

#ifdef FIREBIRD_DEBUG
#define IBDEBUG(a) php_printf("::: %s (%s:%d)\n", a, __FILE__, __LINE__);
#endif

#ifndef IBDEBUG
#define IBDEBUG(a)
#endif

// #define IBASE_MSGSIZE 512
// #define MAX_ERRMSG (IBASE_MSGSIZE*2)

#define FIREBIRD_DATE_FMT "%Y-%m-%d"
#define FIREBIRD_TIME_FMT "%H:%M:%S"

/* this value should never be > USHRT_MAX */
#define IBASE_BLOB_SEG 4096

ZEND_BEGIN_MODULE_GLOBALS(firebird)
    // ISC_STATUS status[20];
    // zend_resource *default_link;
    // zend_long num_links, num_persistent;
    // char errmsg[MAX_ERRMSG];
    // zend_long sql_code;
    zend_long default_trans_params;
    zend_long default_lock_timeout; // only used togetger with trans_param IBASE_LOCK_TIMEOUT
ZEND_END_MODULE_GLOBALS(firebird)

ZEND_EXTERN_MODULE_GLOBALS(firebird)

typedef struct {
    ISC_ARRAY_DESC ar_desc;
    ISC_LONG ar_size; /* size of entire array in bytes */
    unsigned short el_type, el_size;
} firebird_array;

typedef struct firebird_db {
    zend_object std;
} firebird_db;

typedef struct {
    isc_db_handle db_handle;
    // struct tr_list *tr_list;
    // unsigned short dialect;
    // struct event *event_head;
    zend_object std;
} firebird_connection;

typedef struct firebird_trans {
    isc_db_handle db_handle;
    isc_tr_handle tr_handle;
    zend_long trans_args;
    zend_long trans_timeout;
    zend_object std;
} firebird_trans;

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

typedef struct firebird_stmt {
    isc_db_handle db_handle;
    isc_tr_handle tr_handle;
    isc_stmt_handle stmt_handle;
    XSQLDA *in_sqlda, *out_sqlda;
    unsigned char statement_type, has_more_rows;
    firebird_array *in_array, *out_array;
    unsigned short in_array_cnt, out_array_cnt;
    firebird_bind_buf *bind_buf;
    // unsigned short dialect;
    // char *query;
    zend_object std;
} firebird_stmt;

typedef struct firebird_vary {
    unsigned short vary_length;
    char vary_string[1];
} firebird_vary;

// typedef struct tr_list {
//     firebird_trans *trans;
//     struct tr_list *next;
// } firebird_tr_list;

typedef struct firebird_blob {
    isc_blob_handle bl_handle;
    unsigned short type;
    ISC_QUAD bl_qd;
} firebird_blob;

typedef struct firebird_blobinfo {
    ISC_LONG  max_segment;  // Length of longest segment
    ISC_LONG  num_segments; // Total number of segments
    ISC_LONG  total_length; // Total length of blob
    int       bl_stream;    // blob is stream ?
} firebird_blobinfo;

// typedef struct event {
//     firebird_connection *link;
//     zend_resource* link_res;
//     ISC_LONG event_id;
//     unsigned short event_count;
//     char **events;
//     unsigned char *event_buffer, *result_buffer;
//     zval callback;
//     void *thread_ctx;
//     struct event *event_next;
//     enum event_state { NEW, ACTIVE, DEAD } state;
// } firebird_event;

enum php_firebird_option {
    PHP_FIREBIRD_DEFAULT            = 0,
    PHP_FIREBIRD_CREATE             = 0,

    /* fetch flags */
    PHP_FIREBIRD_FETCH_BLOBS        = 1,
    PHP_FIREBIRD_FETCH_ARRAYS       = 2,
    PHP_FIREBIRD_UNIXTIME           = 4,

    /* transaction access mode */
    PHP_FIREBIRD_WRITE              = 1,
    PHP_FIREBIRD_READ               = 2,

    /* transaction isolation level */
    PHP_FIREBIRD_CONCURRENCY        = 4,
    PHP_FIREBIRD_COMMITTED          = 8,
        PHP_FIREBIRD_REC_NO_VERSION = 32,
        PHP_FIREBIRD_REC_VERSION    = 64,
    PHP_FIREBIRD_CONSISTENCY        = 16,

    /* transaction lock resolution */
    PHP_FIREBIRD_WAIT               = 128,
    PHP_FIREBIRD_NOWAIT             = 256,
        PHP_FIREBIRD_LOCK_TIMEOUT   = 512,
};

#define IBG(v) ZEND_MODULE_GLOBALS_ACCESSOR(firebird, v)

#if defined(ZTS) && defined(COMPILE_DL_FIREBIRD)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#define BLOB_ID_LEN     18
#define BLOB_ID_MASK    "0x%" LL_MASK "x"

#define BLOB_INPUT      1
#define BLOB_OUTPUT     2

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

/* determine if a resource is a link or transaction handle */
#define PHP_FIREBIRD_LINK_TRANS(zv, lh, th)                                                    \
        do {                                                                                \
            if (!zv) {                                                                      \
                lh = (firebird_connection *)zend_fetch_resource2(                                 \
                    IBG(default_link), "InterBase link", le_link, le_plink);                \
            } else {                                                                        \
                _php_firebird_get_link_trans(INTERNAL_FUNCTION_PARAM_PASSTHRU, zv, &lh, &th);  \
            }                                                                               \
            if (SUCCESS != _php_firebird_def_trans(lh, &th)) { RETURN_FALSE; }                 \
        } while (0)

int _php_firebird_def_trans(firebird_connection *ib_link, firebird_trans **trans);
void _php_firebird_get_link_trans(INTERNAL_FUNCTION_PARAMETERS, zval *link_id,
    firebird_connection **ib_link, firebird_trans **trans);

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define O_SET(o, name) Z_OBJ_P(o), #name, sizeof(#name) - 1, name
#define THIS_SET(name) O_SET(ZEND_THIS, name)

#define O_GET(o, name) Z_OBJ_P(o), #name, sizeof(#name) - 1
#define THIS_GET(name) O_GET(ZEND_THIS, name)

#define DECLARE_PROP_OBJ(class_ce, name, obj_name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_CLASS(zend_string_init(#obj_name, sizeof(#obj_name)-1, 1), 0, 0), visibilty)
#define DECLARE_PROP_INT(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_MASK(MAY_BE_LONG), visibilty)
#define DECLARE_PROP_STRING(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, ZEND_TYPE_INIT_MASK(MAY_BE_STRING), visibilty)
#define DECLARE_PROP(class_ce, name, type, visibilty) do {                            \
    zval prop_##name##_def_val;                                                       \
    ZVAL_UNDEF(&prop_##name##_def_val);                                               \
    zend_string *prop_##name##_name = zend_string_init(#name, sizeof(#name) - 1, 1);  \
    zend_declare_typed_property(class_ce, prop_##name##_name, &prop_##name##_def_val, \
        visibilty, NULL,                                                              \
        (zend_type) type);                                       \
} while (0)

#define Z_CONNECTION_P(zv) \
    ((firebird_connection*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(firebird_connection, std)))

#define Z_CONNECTION_O(obj) \
    ((firebird_connection*)((char*)(obj) - XtOffsetOf(firebird_connection, std)))

#define Z_TRANSACTION_P(zv) \
    ((firebird_trans*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(firebird_trans, std)))

#define Z_TRANSACTION_O(obj) \
    ((firebird_trans*)((char*)(obj) - XtOffsetOf(firebird_trans, std)))

#define Z_STMT_P(zv) \
    ((firebird_stmt*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(firebird_stmt, std)))

#define Z_STMT_O(obj) \
    ((firebird_stmt*)((char*)(obj) - XtOffsetOf(firebird_stmt, std)))

#define Z_DB_P(zv) \
    ((firebird_db*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(firebird_db, std)))

#define Z_DB_O(obj) \
    ((firebird_db*)((char*)(obj) - XtOffsetOf(firebird_db, std)))

// General argument types
ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_none_return_bool, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

// Database argument types
ZEND_BEGIN_ARG_INFO_EX(arginfo_FireBird_Database_construct, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, database, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, username, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, password, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, charset, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, buffers, IS_LONG, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, dialect, IS_LONG, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, role, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Database_connect, 0, 0, FireBird\\Connection, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

// Connection argument types
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Connection_start_transaction, 0, 0, FireBird\\Transaction, MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, trans_args, IS_LONG, 1, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, lock_timeout, IS_LONG, 1, 0)
ZEND_END_ARG_INFO()

// Transaction argument types
ZEND_BEGIN_ARG_INFO_EX(arginfo_FireBird_Transaction_construct, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, connection, FireBird\\Connection, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, trans_args, IS_LONG, 1, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, lock_timeout, IS_LONG, 1, 0)
ZEND_END_ARG_INFO()

// ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(arginfo_FireBird_Transaction_query, 0, 1, FireBird\\Statement, MAY_BE_FALSE)
//     ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
//     ZEND_ARG_VARIADIC_INFO(0, bind_args)
// ZEND_END_ARG_INFO()

// Statement argument types
ZEND_BEGIN_ARG_INFO_EX(arginfo_FireBird_Statement_construct, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, connection, FireBird\\Transaction, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Statement_prepare, 0, 0, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_Statement_fetch_row, 0, 0, MAY_BE_ARRAY|MAY_BE_FALSE|MAY_BE_NULL)
    ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_FireBird_Statement_fetch_object, 0, 0, MAY_BE_OBJECT|MAY_BE_FALSE|MAY_BE_NULL)
    ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Statement_execute, 0, 0, _IS_BOOL, 0)
    ZEND_ARG_VARIADIC_INFO(0, bind_args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_Statement_query, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
    ZEND_ARG_VARIADIC_INFO(0, bind_args)
ZEND_END_ARG_INFO()

extern zend_class_entry *FireBird_Database_ce;
extern zend_class_entry *FireBird_Connection_ce;
extern zend_class_entry *FireBird_Transaction_ce;
extern zend_class_entry *FireBird_Statement_ce;
extern void register_FireBird_Database_ce();
extern void register_FireBird_Connection_ce();
extern void register_FireBird_Transaction_ce();
extern void register_FireBird_Statement_ce();

#ifdef PHP_DEBUG
#define ADD_ERR_PROPS(class_ce)                                              \
    do {                                                                     \
        DECLARE_PROP_STRING(class_ce, error_msg, ZEND_ACC_PROTECTED_SET);    \
        DECLARE_PROP_INT(class_ce, error_code, ZEND_ACC_PROTECTED_SET);      \
        DECLARE_PROP_INT(class_ce, error_code_long, ZEND_ACC_PROTECTED_SET); \
        DECLARE_PROP_STRING(class_ce, error_line, ZEND_ACC_PROTECTED_SET);   \
    } while(0)
#else
#define ADD_ERR_PROPS(class_ce)                                              \
    do {                                                                     \
        DECLARE_PROP_STRING(class_ce, error_msg, ZEND_ACC_PROTECTED_SET);    \
        DECLARE_PROP_INT(class_ce, error_code, ZEND_ACC_PROTECTED_SET);      \
        DECLARE_PROP_INT(class_ce, error_code_long, ZEND_ACC_PROTECTED_SET); \
    } while(0)
#endif

void dump_buffer(const unsigned char *buffer, int len);
void update_err_props_ex(ISC_STATUS_ARRAY status, zend_class_entry *class_ce, zend_object *obj, const char *file_name, size_t line_num);
int _php_firebird_string_to_quad(char const *id, ISC_QUAD *qd);
int _php_firebird_blob_add(ISC_STATUS_ARRAY status, zval *string_arg, firebird_blob *ib_blob);
int _php_firebird_blob_get(ISC_STATUS_ARRAY status, zval *return_value, firebird_blob *ib_blob, zend_ulong max_len);
zend_string *_php_firebird_quad_to_string(ISC_QUAD const qd);
void transaction_ctor(zval *this, zval *connection, zend_long trans_args, zend_long lock_timeout);
int transaction_start(ISC_STATUS_ARRAY status, zval *tr_o);

#define update_err_props(status, class_ce, obj) update_err_props_ex(status, class_ce, obj, __FILE__, __LINE__)

// C++ experiments
// #ifdef __cplusplus
// extern "C" {
// #endif
// int fb_connect(zend_class_entry *class_ce, zend_object *obj);
// #ifdef __cplusplus
// }
// #endif

#endif /* PHP_FIREBIRD_INCLUDES_H */
