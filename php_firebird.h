/*
   +----------------------------------------------------------------------+
   | PHP Version 7, 8                                                       |
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
   |          Jonatan Klemets <jonatan.r.klemets@gmail.com>               |
   |          others                                                      |
   +----------------------------------------------------------------------+
   | You'll find history on Github                                        |
   | https://github.com/FirebirdSQL/php-firebird/commits/master           |
   +----------------------------------------------------------------------+
 */

#ifndef PHP_FIREBIRD_H
#define PHP_FIREBIRD_H

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

#endif /* PHP_FIREBIRD_H */
