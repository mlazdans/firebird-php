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

#include <ibase.h>
#include "php_version.h"
#define PHP_FIREBIRD_VERSION "0.0.1-alpha"

PHP_MINIT_FUNCTION(firebird);
PHP_RINIT_FUNCTION(firebird);
PHP_MSHUTDOWN_FUNCTION(firebird);
PHP_RSHUTDOWN_FUNCTION(firebird);
PHP_MINFO_FUNCTION(firebird);

// PHP_FUNCTION(firebird_connect);
// PHP_FUNCTION(firebird_pconnect);
// PHP_FUNCTION(firebird_close);
// PHP_FUNCTION(firebird_drop_db);
// PHP_FUNCTION(firebird_query);
// PHP_FUNCTION(firebird_fetch_row);
// PHP_FUNCTION(firebird_fetch_assoc);
// PHP_FUNCTION(firebird_fetch_object);
// PHP_FUNCTION(firebird_free_result);
// PHP_FUNCTION(firebird_name_result);
// PHP_FUNCTION(firebird_prepare);
// PHP_FUNCTION(firebird_execute);
// PHP_FUNCTION(firebird_free_query);

// PHP_FUNCTION(firebird_timefmt);

// PHP_FUNCTION(firebird_gen_id);
// PHP_FUNCTION(firebird_num_fields);
// PHP_FUNCTION(firebird_num_params);
// #if abies_0
// PHP_FUNCTION(firebird_num_rows);
// #endif
// PHP_FUNCTION(firebird_affected_rows);
// PHP_FUNCTION(firebird_field_info);
// PHP_FUNCTION(firebird_param_info);

// PHP_FUNCTION(firebird_trans);
// PHP_FUNCTION(firebird_commit);
// PHP_FUNCTION(firebird_rollback);
// PHP_FUNCTION(firebird_commit_ret);
// PHP_FUNCTION(firebird_rollback_ret);

// PHP_FUNCTION(firebird_blob_create);
// PHP_FUNCTION(firebird_blob_add);
// PHP_FUNCTION(firebird_blob_cancel);
// PHP_FUNCTION(firebird_blob_open);
// PHP_FUNCTION(firebird_blob_get);
// PHP_FUNCTION(firebird_blob_close);
// PHP_FUNCTION(firebird_blob_echo);
// PHP_FUNCTION(firebird_blob_info);
// PHP_FUNCTION(firebird_blob_import);

// PHP_FUNCTION(firebird_add_user);
// PHP_FUNCTION(firebird_modify_user);
// PHP_FUNCTION(firebird_delete_user);

// PHP_FUNCTION(firebird_service_attach);
// PHP_FUNCTION(firebird_service_detach);
// PHP_FUNCTION(firebird_backup);
// PHP_FUNCTION(firebird_restore);
// PHP_FUNCTION(firebird_maintain_db);
// PHP_FUNCTION(firebird_db_info);
// PHP_FUNCTION(firebird_server_info);

// PHP_FUNCTION(firebird_errmsg);
// PHP_FUNCTION(firebird_errcode);

// PHP_FUNCTION(firebird_wait_event);
// PHP_FUNCTION(firebird_set_event_handler);
// PHP_FUNCTION(firebird_free_event_handler);

typedef struct _firebird_connection_obj_t {
    isc_db_handle handle;
    // zend_string *value;
    // size_t scale;
    // bc_num num;
    zend_object std;
} firebird_connection_obj_t;

#else

#define phpext_firebird_ptr NULL

#endif /* PHP_FIREBIRD_H */
