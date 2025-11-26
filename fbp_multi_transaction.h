#ifndef FBP_MULTI_TRANSACTION_H
#define FBP_MULTI_TRANSACTION_H

#include <ibase.h>
#include "php.h"

typedef struct firebird_multi_trans {
    isc_tr_handle tr_handle;
    ISC_UINT64 tr_id;
    zval Databases;
    zval Builders;
    zval Transactions;
    unsigned short is_prepared_2pc;

    zend_object std;
} firebird_multi_trans;

fbp_declare_object_accessor(firebird_multi_trans);

int fbp_multi_transaction_start(firebird_multi_trans *mtr);

#endif /* FBP_MULTI_TRANSACTION_H */
