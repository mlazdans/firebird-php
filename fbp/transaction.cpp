#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "firebird_utils.h"
#include "zend_exceptions.h"

using namespace Firebird;

namespace FBP {

static const unsigned char BLOB_PARAMS[] = {
    isc_bpb_version1,
    isc_bpb_type, 1, isc_bpb_type_stream
};

Transaction::Transaction(Database *dba): dba{dba}
{
    FBDEBUG("new Transaction(dba=%p)", dba);
}

ITransaction* Transaction::get_tra()
{
    return tra;
}

IAttachment* Transaction::get_dba()
{
    return dba->get_att();
}

void Transaction::start(const firebird_tbuilder *builder)
{
    // TODO: check if already started
    if (tra) {
        throw Php_Firebird_Exception(zend_ce_error, "Transaction already started");
    }

    if (builder) {
        auto util = master->getUtilInterface();
        auto tpb = util->getXpbBuilder(&st, IXpbBuilder::TPB, NULL, 0);
        fbu_transaction_build_tpb(tpb, builder);
        // fbp_dump_buffer(tpb->getBufferLength(&st), tpb->getBuffer(&st));
        tra = dba->get_att()->startTransaction(&st, tpb->getBufferLength(&st), tpb->getBuffer(&st));
    } else {
        tra = dba->get_att()->startTransaction(&st, 0, NULL);
    }
}

ISC_INT64 Transaction::query_transaction_id()
{
    auto util = master->getUtilInterface();

    unsigned char req[] = { isc_info_tra_id };
    unsigned char resp[16];

    tra->getInfo(&st, sizeof(req), req, sizeof(resp), resp);

    auto dpb = util->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, sizeof(resp));
    for (dpb->rewind(&st); !dpb->isEof(&st); dpb->moveNext(&st)) {
        auto tag = dpb->getTag(&st);
        if (tag == isc_info_tra_id) {
            return dpb->getBigInt(&st);
        }
    }

    return -1;
}

void Transaction::fbu_transaction_build_tpb(IXpbBuilder *tpb, const firebird_tbuilder *builder)
{
    ThrowStatusWrapper st(fb_get_master_interface()->getStatus());

    FBDEBUG("Creating transaction start buffer");

    if (!builder) return;

    tpb->insertTag(&st, builder->read_only ? isc_tpb_read : isc_tpb_write);

    if (builder->ignore_limbo) tpb->insertTag(&st, isc_tpb_ignore_limbo);
    if (builder->auto_commit) tpb->insertTag(&st, isc_tpb_autocommit);
    if (builder->no_auto_undo) tpb->insertTag(&st, isc_tpb_no_auto_undo);

    // TODO: switch
    if (builder->isolation_mode == 0) {
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_consistency");
        tpb->insertTag(&st, isc_tpb_consistency);
    } else if (builder->isolation_mode == 1) {
        tpb->insertTag(&st, isc_tpb_concurrency);
        if (builder->snapshot_at_number) {
            tpb->insertInt(&st, isc_tpb_at_snapshot_number, builder->snapshot_at_number);
            // *p++ = isc_tpb_at_snapshot_number;
            // *p++ = sizeof(builder->snapshot_at_number);
            // fbp_store_portable_integer(p, builder->snapshot_at_number, sizeof(builder->snapshot_at_number));
            // p += sizeof(builder->snapshot_at_number);
            FBDEBUG_NOFL("  isolation_mode = isc_tpb_concurrency");
            FBDEBUG_NOFL("                   isc_tpb_at_snapshot_number = %d", builder->snapshot_at_number);
        } else {
            FBDEBUG_NOFL("  isolation_mode = isc_tpb_concurrency");
        }
    } else if (builder->isolation_mode == 2) {
        tpb->insertTag(&st, isc_tpb_read_committed);
        tpb->insertTag(&st, isc_tpb_rec_version);
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_rec_version");
    } else if (builder->isolation_mode == 3) {
        tpb->insertTag(&st, isc_tpb_read_committed);
        tpb->insertTag(&st, isc_tpb_no_rec_version);
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_no_rec_version");
    } else if (builder->isolation_mode == 4) {
        tpb->insertTag(&st, isc_tpb_read_committed);
        tpb->insertTag(&st, isc_tpb_read_consistency);
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_read_consistency");
    } else {
        fbp_fatal("BUG! unknown transaction isolation_mode: %d", builder->isolation_mode);
    }

    if (builder->lock_timeout == 0) {
        tpb->insertTag(&st, isc_tpb_nowait);
        FBDEBUG_NOFL("  isc_tpb_nowait");
    } else if (builder->lock_timeout == -1) {
        tpb->insertTag(&st, isc_tpb_wait);
        FBDEBUG_NOFL("  isc_tpb_wait");
    } else if (builder->lock_timeout > 0) {
        tpb->insertTag(&st, isc_tpb_wait);
        tpb->insertInt(&st, isc_tpb_lock_timeout, builder->lock_timeout);
        FBDEBUG_NOFL("  isc_tpb_wait");
        FBDEBUG_NOFL("    isc_tpb_lock_timeout = %d", builder->lock_timeout);
    } else {
        fbp_fatal("BUG! invalid lock_timeout: %d", builder->lock_timeout);
    }
}

Transaction::~Transaction()
{
    FBDEBUG("~Transaction(this=%p)", this);

    int err = 0;
    try
    {
        if (tra) tra->rollback(&st);
    }
    catch (...)
    {
        err = 1;
    }

    if (tra) {
        tra->release();
        tra = nullptr;
    }

    if (err) fbu_handle_exception2();
}

void Transaction::commit()
{
    tra->commit(&st);
    tra->release();
    tra = nullptr;
}

void Transaction::commit_ret()
{
    tra->commitRetaining(&st);
}

void Transaction::rollback()
{
    tra->rollback(&st);
    tra->release();
    tra = nullptr;
}

void Transaction::rollback_ret()
{
    tra->rollbackRetaining(&st);
}

// TODO: move to blob.cpp
int Transaction::blob_set_info(IBlob *blo, firebird_blob *blob)
{
    auto util = master->getUtilInterface();

    const unsigned char req[] = {
        isc_info_blob_num_segments,
        isc_info_blob_max_segment,
        isc_info_blob_total_length,
        isc_info_blob_type
    };

    unsigned char resp[32] = {0};

    blo->getInfo(&st, sizeof(req), req, sizeof(resp), resp);

    IXpbBuilder* b = util->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, sizeof(resp));

    for (b->rewind(&st); !b->isEof(&st); b->moveNext(&st)) {
        unsigned char tag = b->getTag(&st);
        int val = b->getInt(&st);

        // FBDEBUG_NOFL(" tag: %d, val: %d", tag, val);
        switch(tag) {
            case isc_info_blob_num_segments:
                blob->num_segments = val;
                break;
            case isc_info_blob_max_segment:
                blob->max_segment = val;
                break;
            case isc_info_blob_total_length:
                blob->total_length = val;
                break;
            case isc_info_blob_type:
                blob->type = val;
                break;
        }
    }

    return SUCCESS;
}

zend_string* Transaction::fetch_blob_data(ISC_QUAD id)
{
    firebird_blob blob;
    unsigned len = 0;

    auto blo = dba->get_att()->openBlob(&st, tra, &id, sizeof(BLOB_PARAMS), BLOB_PARAMS);
    blob_set_info(blo, &blob);

    zend_string *buf = zend_string_alloc(blob.total_length, 0);
    blo->getSegment(&st, ZSTR_LEN(buf), &ZSTR_VAL(buf), &len);
    ZSTR_VAL(buf)[len] = '\0';

    return buf;
}

ITransaction *Transaction::execute(unsigned len_sql, const char *sql)
{
    FBDEBUG("Transaction::execute(dba=%p)", dba);
    return tra = dba->get_att()->execute(&st, tra, len_sql, sql, SQL_DIALECT_CURRENT, NULL, NULL, NULL, NULL);

    // if (tra2) {
    //     if (!tra) {
    //         return tra = tra2;
    //     } else {
    //         assert(tra == tra2);
    //     }
    // } else {
    //     return tra = tra2;
    // }

    return nullptr;
}

} // namespace
