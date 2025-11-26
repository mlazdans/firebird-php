// #include <firebird/fb_c_api.h>
#include "php.h"
#include "zend.h"
#include "php_firebird.h"
#include "ext/standard/php_var.h"

#include "transaction.h"
#include "fbp_multi_transaction.h"

fbp_object_accessor(firebird_multi_trans);

int fbp_multi_transaction_start(firebird_multi_trans *mtr)
{
    TODO("fbp_multi_transaction_start");
#if 0
    zend_array *db_arr = Z_ARR(mtr->Databases);
    zend_array *b_arr = Z_ARR(mtr->Builders);
    zval *Db, *Builder;

    ZEND_ASSERT(db_arr->nNumOfElements == b_arr->nNumOfElements);

    char *buffers = emalloc(db_arr->nNumOfElements * sizeof(TPB_MAX_SIZE));
    firebird_teb *tebs = emalloc(db_arr->nNumOfElements * sizeof(firebird_teb));

    for (int i = 0; i < db_arr->nNumOfElements; i++) {
        zval *Builder = zend_hash_index_find(b_arr, i);
        zval *Db = zend_hash_index_find(db_arr, i);

        firebird_db *db = get_firebird_db_from_zval(Db);
        tebs[i].db_handle = &db->db_handle;
        tebs[i].tpb_len = 0;
        tebs[i].tpb = &buffers[i];

        if (Builder && !ZVAL_IS_NULL(Builder)) {
            firebird_tbuilder *builder = get_firebird_tbuilder_from_zval(Builder);
            fbp_transaction_build_tpb(builder, tebs[i].tpb, &tebs[i].tpb_len);
        }
    }

    if (isc_start_multiple(FBG(status), &mtr->tr_handle, db_arr->nNumOfElements, tebs)) {
        return FAILURE;
    }

    efree(tebs);
    efree(buffers);

    return SUCCESS;
#endif
}
