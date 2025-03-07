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

// #define RESET_ERRMSG do { IBG(errmsg)[0] = '\0'; IBG(sql_code) = 0; } while (0)

// #define IB_STATUS (IBG(status))

#ifdef FIREBIRD_DEBUG
#define IBDEBUG(a) php_printf("::: %s (%s:%d)\n", a, __FILE__, __LINE__);
#endif

#ifndef IBDEBUG
#define IBDEBUG(a)
#endif

// extern int le_link, le_plink, le_trans;

// #define LE_LINK "Firebird/InterBase link"
// #define LE_PLINK "Firebird/InterBase persistent link"
// #define LE_TRANS "Firebird/InterBase transaction"

// #define IBASE_MSGSIZE 512
// #define MAX_ERRMSG (IBASE_MSGSIZE*2)

#define IB_DEF_DATE_FMT "%Y-%m-%d"
#define IB_DEF_TIME_FMT "%H:%M:%S"

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
    isc_db_handle handle;
    // struct tr_list *tr_list;
    // unsigned short dialect;
    // struct event *event_head;
    zend_object std;
} firebird_db_link;

typedef struct {
    isc_tr_handle handle;
    unsigned short link_cnt;
    unsigned long affected_rows;
    firebird_db_link *db_link[1]; /* last member */
} firebird_trans;

typedef struct tr_list {
    firebird_trans *trans;
    struct tr_list *next;
} firebird_tr_list;

typedef struct {
    isc_blob_handle bl_handle;
    unsigned short type;
    ISC_QUAD bl_qd;
} firebird_blob;

typedef struct event {
    firebird_db_link *link;
    zend_resource* link_res;
    ISC_LONG event_id;
    unsigned short event_count;
    char **events;
    unsigned char *event_buffer, *result_buffer;
    zval callback;
    void *thread_ctx;
    struct event *event_next;
    enum event_state { NEW, ACTIVE, DEAD } state;
} firebird_event;

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

void _php_firebird_error(void);
void _php_firebird_module_error(char *, ...)
    PHP_ATTRIBUTE_FORMAT(printf,1,2);

/* determine if a resource is a link or transaction handle */
#define PHP_FIREBIRD_LINK_TRANS(zv, lh, th)                                                    \
        do {                                                                                \
            if (!zv) {                                                                      \
                lh = (firebird_db_link *)zend_fetch_resource2(                                 \
                    IBG(default_link), "InterBase link", le_link, le_plink);                \
            } else {                                                                        \
                _php_firebird_get_link_trans(INTERNAL_FUNCTION_PARAM_PASSTHRU, zv, &lh, &th);  \
            }                                                                               \
            if (SUCCESS != _php_firebird_def_trans(lh, &th)) { RETURN_FALSE; }                 \
        } while (0)

int _php_firebird_def_trans(firebird_db_link *ib_link, firebird_trans **trans);
void _php_firebird_get_link_trans(INTERNAL_FUNCTION_PARAMETERS, zval *link_id,
    firebird_db_link **ib_link, firebird_trans **trans);

/* provided by firebird_query.c */
void php_firebird_query_minit(INIT_FUNC_ARGS);

/* provided by firebird_blobs.c */
void php_firebird_blobs_minit(INIT_FUNC_ARGS);
int _php_firebird_string_to_quad(char const *id, ISC_QUAD *qd);
zend_string *_php_firebird_quad_to_string(ISC_QUAD const qd);
int _php_firebird_blob_get(zval *return_value, firebird_blob *ib_blob, zend_ulong max_len);
int _php_firebird_blob_add(zval *string_arg, firebird_blob *ib_blob);

/* provided by firebird_events.c */
void php_firebird_events_minit(INIT_FUNC_ARGS);
void _php_firebird_free_event(firebird_event *event);

/* provided by firebird_service.c */
void php_firebird_service_minit(INIT_FUNC_ARGS);

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define DECLARE_PROP_INT(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, MAY_BE_LONG, visibilty)
#define DECLARE_PROP_STRING(class_ce, name, visibilty) DECLARE_PROP(class_ce, name, MAY_BE_STRING, visibilty)
#define DECLARE_PROP(class_ce, name, type, visibilty) do {                            \
    zval prop_##name##_def_val;                                                       \
    ZVAL_UNDEF(&prop_##name##_def_val);                                               \
    zend_string *prop_##name##_name = zend_string_init(#name, sizeof(#name) - 1, 1);  \
    zend_declare_typed_property(class_ce, prop_##name##_name, &prop_##name##_def_val, \
        visibilty, NULL,                                                              \
        (zend_type) ZEND_TYPE_INIT_MASK(type));                                       \
} while (0)

#define Z_CONNECTION_P(zv) \
    ((firebird_db_link*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(firebird_db_link, std)))

#define Z_CONNECTION_O(obj) \
    ((firebird_db_link*)((char*)(obj) - XtOffsetOf(firebird_db_link, std)))

ZEND_BEGIN_ARG_INFO_EX(arginfo_firebird_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_firebird_bool, 0, 0, _IS_BOOL, 0)
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

extern zend_class_entry *firebird_connection_ce;
extern void register_connection_ce();

#endif /* PHP_FIREBIRD_INCLUDES_H */
