#ifndef FIREBIRD_PHP_H
#define FIREBIRD_PHP_H

#include "fbp/database.hpp"

using namespace FBP;

extern "C" {

#include "php.h"

extern zend_module_entry firebird_module_entry;
#define phpext_firebird_ptr &firebird_module_entry

// TODO: move here
// ZEND_BEGIN_MODULE_GLOBALS(firebird)
//     ISC_STATUS_ARRAY status;
//     bool debug;
// ZEND_END_MODULE_GLOBALS(firebird)

// ZEND_EXTERN_MODULE_GLOBALS(firebird)

// #define FBG(v) ZEND_MODULE_GLOBALS_ACCESSOR(firebird, v)

// #if defined(ZTS) && defined(COMPILE_DL_FIREBIRD)
// ZEND_TSRMLS_CACHE_EXTERN()
// #endif

PHP_MINIT_FUNCTION(firebird);
PHP_RINIT_FUNCTION(firebird);
PHP_MSHUTDOWN_FUNCTION(firebird);
PHP_RSHUTDOWN_FUNCTION(firebird);
PHP_MINFO_FUNCTION(firebird);

ZEND_BEGIN_MODULE_GLOBALS(firebird)
    // ISC_STATUS_ARRAY status;
    bool debug;
    std::vector<DatabasePtr> *db_list;
    // int has_error_handler;
    zend_fcall_info error_fci;
    zend_fcall_info_cache error_fcc;
ZEND_END_MODULE_GLOBALS(firebird)

ZEND_EXTERN_MODULE_GLOBALS(firebird)

#define FBG(v) ZEND_MODULE_GLOBALS_ACCESSOR(firebird, v)

#if defined(ZTS) && defined(COMPILE_DL_FIREBIRD)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#define update_err_props(status, _nd1, _nd2)

#ifdef PHP_DEBUG
#define FBDEBUG(format, ...) if(FBG(debug))php_printf(format " (%s:%d)\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__);
#define FBDEBUG_NOFL(format, ...) if(FBG(debug))php_printf(format "\n" __VA_OPT__(,) __VA_ARGS__);
#else
#define FBDEBUG(...)
#define FBDEBUG_NOFL(...)
#endif

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

}

#endif // FIREBIRD_PHP_H
