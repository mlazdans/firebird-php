#ifndef FBP_TRANSACTION_H
#define FBP_TRANSACTION_H

typedef enum {
    FBP_TR_ROLLBACK = 1,
    FBP_TR_COMMIT   = 2,
    FBP_TR_RETAIN   = 4,
} firebird_tr_fin_flag;

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
    // I want to be able to set this pointer to, for example, to tr_handle from
    // multi transaction, so it could be shared between databases. In standard
    // transactions this should point to real_tr_handle
    isc_tr_handle *tr_handle;
    isc_db_handle *db_handle;
    firebird_tbuilder *builder;
    ISC_UINT64 tr_id;
    unsigned short is_prepared_2pc;
    isc_tr_handle real_tr_handle;

    zend_object std;
} firebird_trans;

typedef struct firebird_teb {
    isc_db_handle *db_handle;
    int tpb_len;
    char *tpb;
} firebird_teb;

fbp_declare_object_accessor(firebird_tbuilder);
fbp_declare_object_accessor(firebird_trans);

void fbp_transaction_ctor(firebird_trans *tr, firebird_db *db, firebird_tbuilder *builder);
int fbp_transaction_start(firebird_trans *tr);
int fbp_transaction_get_info(firebird_trans *tr);
int fbp_transaction_finalize(isc_tr_handle *tr_handle, firebird_tr_fin_flag mode);
int fbp_transaction_reconnect(firebird_trans *tr, ISC_ULONG id);
void fbp_transaction_build_tpb(firebird_tbuilder *builder, char *tpb, int *tpb_len);

#endif /* FBP_TRANSACTION_H */
