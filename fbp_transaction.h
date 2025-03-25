#ifndef FBP_TRANSACTION_H
#define FBP_TRANSACTION_H

enum firebird_tr_fin_flag {
    FBP_TR_ROLLBACK = 1,
    FBP_TR_COMMIT   = 2,
    FBP_TR_RETAIN   = 4,
};

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
    isc_tr_handle tr_handle;
    isc_db_handle *db_handle;
    firebird_tbuilder *builder;
    ISC_UINT64 tr_id;
    unsigned short is_prepared_2pc;

    zend_object std;
} firebird_trans;

void fbp_transaction_ctor(firebird_trans *tr, firebird_db *db, firebird_tbuilder *builder);
int fbp_transaction_start(firebird_trans *tr);
int fbp_transaction_get_info(firebird_trans *tr);
int fbp_transaction_finalize(firebird_trans *tr, int mode);
int fbp_transaction_reconnect(firebird_trans *tr, ISC_ULONG id);
void fbp_transaction_build_tpb(firebird_tbuilder *builder, char *tpb, unsigned short *tpb_len);

#endif /* FBP_TRANSACTION_H */
