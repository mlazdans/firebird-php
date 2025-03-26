#include <firebird/fb_c_api.h>
#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"

#include "fbp_transaction.h"

fbp_object_accessor(firebird_trans);
fbp_object_accessor(firebird_tbuilder);

void fbp_transaction_ctor(firebird_trans *tr, firebird_db *db, firebird_tbuilder *builder)
{
    tr->tr_handle = 0;
    tr->tr_id = 0;
    tr->db_handle = &db->db_handle;
    tr->is_prepared_2pc = 0;
    tr->builder = builder;
}

int fbp_transaction_start(firebird_trans *tr)
{
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;

    if (tr->builder != NULL) {
        fbp_transaction_build_tpb(tr->builder, tpb, &tpb_len);
        assert(tpb_len <= sizeof(tpb));
    }

    // TODO: isc_start_multiple, isc_prepare_transaction, isc_prepare_transaction2
    // isc_start_multiple()       - Begins a new transaction against multiple databases.
    // isc_prepare_transaction()  - Executes the first phase of a two-phase commit against multiple databases.
    // isc_prepare_transaction2() - Performs the first phase of a two-phase commit for multi-database transactions.
    if(isc_start_transaction(FBG(status), &tr->tr_handle, 1, tr->db_handle, tpb_len, tpb)) {
        return FAILURE;
    }

    if(fbp_transaction_get_info(tr)) {
        return FAILURE;
    }

    return SUCCESS;
}

int fbp_transaction_get_info(firebird_trans *tr)
{
    static char info_req[] = { isc_info_tra_id };
    char info_resp[(sizeof(ISC_INT64) * 2)] = { 0 };

    if (isc_transaction_info(FBG(status), &tr->tr_handle, sizeof(info_req), info_req, sizeof(info_resp), info_resp)) {
        return FAILURE;
    }

    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* dpb;

    dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_INFO_RESPONSE, info_resp, sizeof(info_resp));

    ISC_INT64 val, len, total_len = 0;
    const char *str;
    unsigned char tag;

    FBDEBUG("Parsing Transaction info buffer");
    for (IXpbBuilder_rewind(dpb, st); !IXpbBuilder_isEof(dpb, st); IXpbBuilder_moveNext(dpb, st)) {
        tag = IXpbBuilder_getTag(dpb, st); total_len++;
        len = IXpbBuilder_getLength(dpb, st); total_len += 2;
        total_len += len;

        switch(tag) {
            case isc_info_end: break;

            case isc_info_tra_id: {
                val = IXpbBuilder_getBigInt(dpb, st);
                FBDEBUG_NOFL("  tag: %d len: %d val: %d", tag, len, val);
                tr->tr_id = (ISC_UINT64)val;
            } break;

            case fb_info_tra_dbpath: {
                str = IXpbBuilder_getString(dpb, st);
                FBDEBUG_NOFL("  tag: %d len: %d val: %s", tag, len, str);
            } break;

            case isc_info_truncated: {
                fbp_warning("Transaction info buffer error: truncated");
            } return FAILURE;

            case isc_info_error: {
                fbp_warning("Transaction info buffer error");
            } return FAILURE;

            default: {
                fbp_fatal("BUG! Unhandled Transaction info tag: %d", tag);
            } break;
        }
    }

    return SUCCESS;
}

int fbp_transaction_finalize(firebird_trans *tr, int mode)
{
    ISC_STATUS result;

    if (mode == FBP_TR_COMMIT) {
        result = isc_commit_transaction(FBG(status), &tr->tr_handle);
    } else if (mode == (FBP_TR_ROLLBACK | FBP_TR_RETAIN)) {
        result = isc_rollback_retaining(FBG(status), &tr->tr_handle);
    } else if (mode == (FBP_TR_COMMIT | FBP_TR_RETAIN)) {
        result = isc_commit_retaining(FBG(status), &tr->tr_handle);
    } else {
        result = isc_rollback_transaction(FBG(status), &tr->tr_handle);
    }

    return result;
}

void fbp_transaction_build_tpb(firebird_tbuilder *builder, char *tpb, unsigned short *tpb_len)
{
    char *p = tpb;

    FBDEBUG("Creating transaction start buffer");

    *p++ = isc_tpb_version3;

    *p++ = builder->read_only ? isc_tpb_read : isc_tpb_write;
    if (builder->ignore_limbo) *p++ = isc_tpb_ignore_limbo;
    if (builder->auto_commit) *p++ = isc_tpb_autocommit;
    if (builder->no_auto_undo) *p++ = isc_tpb_no_auto_undo;

    if (builder->isolation_mode == 0) {
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_consistency");
        *p++ = isc_tpb_consistency;
    } else if (builder->isolation_mode == 1) {
        *p++ = isc_tpb_concurrency;
        if (builder->snapshot_at_number) {
            *p++ = isc_tpb_at_snapshot_number;
            *p++ = sizeof(builder->snapshot_at_number);
            fbp_store_portable_integer(p, builder->snapshot_at_number, sizeof(builder->snapshot_at_number));
            p += sizeof(builder->snapshot_at_number);
            FBDEBUG_NOFL("  isolation_mode = isc_tpb_concurrency");
            FBDEBUG_NOFL("                   isc_tpb_at_snapshot_number = %d", builder->snapshot_at_number);
        } else {
            FBDEBUG_NOFL("  isolation_mode = isc_tpb_concurrency");
        }
    } else if (builder->isolation_mode == 2) {
        *p++ = isc_tpb_read_committed;
        *p++ = isc_tpb_rec_version;
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_rec_version");
    } else if (builder->isolation_mode == 3) {
        *p++ = isc_tpb_read_committed;
        *p++ = isc_tpb_no_rec_version;
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_no_rec_version");
    } else if (builder->isolation_mode == 4) {
        *p++ = isc_tpb_read_committed;
        *p++ = isc_tpb_read_consistency;
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_read_consistency");
    } else {
        fbp_fatal("BUG! unknown transaction isolation_mode: %d", builder->isolation_mode);
    }

    if (builder->lock_timeout == 0) {
        *p++ = isc_tpb_nowait;
        FBDEBUG_NOFL("  isc_tpb_nowait");
    } else if (builder->lock_timeout == -1) {
        *p++ = isc_tpb_wait;
        FBDEBUG_NOFL("  isc_tpb_wait");
    } else if (builder->lock_timeout > 0) {
        *p++ = isc_tpb_wait;
        *p++ = isc_tpb_lock_timeout;
        *p++ = sizeof(builder->lock_timeout);
        fbp_store_portable_integer(p, builder->lock_timeout, sizeof(builder->lock_timeout));
        p += sizeof(builder->lock_timeout);
        FBDEBUG_NOFL("  isc_tpb_wait");
        FBDEBUG_NOFL("    isc_tpb_lock_timeout = %d", builder->lock_timeout);
    } else {
        fbp_fatal("BUG! invalid lock_timeout: %d", builder->lock_timeout);
    }

    *tpb_len = p - tpb;
}

int fbp_transaction_reconnect(firebird_trans *tr, ISC_ULONG id)
{
    if (isc_reconnect_transaction(FBG(status), tr->db_handle, &tr->tr_handle, sizeof(id), (const ISC_SCHAR *)&id)) {
        return FAILURE;
    }

    if (fbp_transaction_get_info(tr)) {
        return FAILURE;
    }

    return SUCCESS;
}
