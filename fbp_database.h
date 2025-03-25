#ifndef FBP_DATABASE_H
#define FBP_DATABASE_H

#include <ibase.h>
#include "php.h"

extern firebird_xpb_zmap fbp_database_create_zmap;
extern firebird_xpb_zmap fbp_database_connect_zmap;
extern firebird_xpb_zmap fbp_database_info_zmap;

typedef struct firebird_db {
    isc_db_handle db_handle;

    zend_object std;
} firebird_db;

fbp_declare_object_accessor(firebird_db);

int fbp_database_build_dpb(zend_class_entry *ce, zval *Args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written);
int fbp_database_create(firebird_db *db, zval *Create_Args);
int fbp_database_connect(firebird_db *db, zval *Connect_Args);
int fbp_database_get_info(firebird_db *db, zval *Db_Info,
    size_t info_req_size, char *info_req, size_t info_resp_size, char *info_resp, size_t max_limbo_count);

#endif /* FBP_DATABASE_H */
