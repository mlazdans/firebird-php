#ifndef TRANSACTION_H
#define TRANSACTION_H

#define FBP_TR_ROLLBACK 1
#define FBP_TR_COMMIT   2
#define FBP_TR_RETAIN   4

#include <ibase.h>
#include "php.h"
#include "fbp_database.h"

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

typedef struct firebird_trans {
    void *trptr;
    firebird_db *db;
    ISC_INT64 id;

    zend_object std;
} firebird_trans;

typedef struct firebird_teb {
    isc_db_handle *db_handle;
    int tpb_len;
    char *tpb;
} firebird_teb;

fbp_declare_object_accessor(firebird_tbuilder);
fbp_declare_object_accessor(firebird_trans);

void FireBird_Transaction___construct(zval *self, zval *database);
int FireBird_Transaction_start(zval *self, zval *builder);
// int FireBird_Transaction_prepare(zval *Tr, zval *return_Stmt, const ISC_SCHAR* sql);
void register_FireBird_Transaction_object_handlers();
void register_FireBird_TBuilder_object_handlers();

#endif /* TRANSACTION_H */
