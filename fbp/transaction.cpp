#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/blob.hpp"
#include "firebird_php.hpp"

extern "C" {
#include "firebird_utils.h"
#include "zend_exceptions.h"
}

using namespace Firebird;

namespace FBP {

Transaction::Transaction(Database &dba): dba{dba}
{
    FBDEBUG("new Transaction(dba=%p)=%p", PTR(dba), PTR(*this));
}

void Transaction::start(const firebird_tbuilder *builder)
{
    // TODO: check if already started
    if (tra) {
        throw Php_Firebird_Exception(zend_ce_error, "Transaction already started");
    }

    auto tpb = build_tpb(builder);

    if (tpb) {
        tra = dba.get()->startTransaction(&st, tpb->getBufferLength(&st), tpb->getBuffer(&st));
        tpb->dispose();
    } else {
        tra = dba.get()->startTransaction(&st, 0, NULL);
    }
    query_info();
}

void Transaction::query_info()
{
    if (tra) {
        auto util = master->getUtilInterface();

        unsigned char req[] = { isc_info_tra_id };
        unsigned char resp[16];

        get()->getInfo(&st, sizeof(req), req, sizeof(resp), resp);

        auto dpb = util->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, sizeof(resp));
        for (dpb->rewind(&st); !dpb->isEof(&st); dpb->moveNext(&st)) {
            auto tag = dpb->getTag(&st);
            if (tag == isc_info_tra_id) {
                info.id = dpb->getBigInt(&st);
            }
        }
    } else {
        info.id = 0;
    }
}

const firebird_trans_info *Transaction::get_info()
{
    return &info;
}

IXpbBuilder *Transaction::build_tpb(const firebird_tbuilder *builder)
{
    FBDEBUG("Creating transaction start buffer");

    if (!builder) return nullptr;

    auto util = master->getUtilInterface();
    auto tpb = util->getXpbBuilder(&st, IXpbBuilder::TPB, NULL, 0);

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

    return tpb;
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

    if (err) fbu_handle_exception(__FILE__, __LINE__);
}

void Transaction::commit()
{
    get()->commit(&st);
    tra->release();
    tra = nullptr;
}

void Transaction::commit_ret()
{
    get()->commitRetaining(&st);
}

void Transaction::rollback()
{
    get()->rollback(&st);
    tra->release();
    tra = nullptr;
}

void Transaction::rollback_ret()
{
    get()->rollbackRetaining(&st);
}

zend_string* Transaction::get_blob_contents(ISC_QUAD *blob_id)
{
    auto blob = new Blob(*this);
    blob->open(blob_id);
    zend_string *ret;
    blob->get_contents(0, &ret);

    delete blob;

    return ret;
}

ISC_QUAD Transaction::create_blob_from_string(zend_string *data)
{
    auto blob = new Blob(*this);
    auto id = blob->create();
    blob->put_contents(ZSTR_LEN(data), ZSTR_VAL(data));
    blob->close();

    delete blob;

    return id;
}

ITransaction *Transaction::execute(unsigned len_sql, const char *sql)
{
    // FBDEBUG("Transaction::execute(dba=%p)", dba);

    // TODO: release old?
    auto new_tra = dba.get()->execute(&st, tra, len_sql, sql, SQL_DIALECT_CURRENT, NULL, NULL, NULL, NULL);

    if (new_tra != tra) {
        tra = new_tra;
        query_info();
    }

    return tra;
}

IBlob *Transaction::open_blob(ISC_QUAD *blob_id)
{
    const unsigned char bpb[] = {
        isc_bpb_version1,
        isc_bpb_type, 1, isc_bpb_type_stream
    };

    return dba.get()->openBlob(&st, tra, blob_id, sizeof(bpb), bpb);
}

IBlob *Transaction::create_blob(ISC_QUAD *blob_id)
{
    const unsigned char bpb[] = {
        isc_bpb_version1,
        isc_bpb_type, 1, isc_bpb_type_stream
    };

    return dba.get()->createBlob(&st, tra, blob_id, sizeof(bpb), bpb);
}

void Transaction::execute_statement(IStatement *statement,
    IMessageMetadata* input_metadata, void* in_buffer,
    IMessageMetadata* output_metadata, void* out_buffer)
{
    auto new_tra = statement->execute(&st, tra, input_metadata, in_buffer, output_metadata, out_buffer);

    if (new_tra != tra) {
        tra = new_tra;
        query_info();
    }
}

IResultSet *Transaction::open_cursor(IStatement *statement,
    IMessageMetadata* input_metadata, void* in_buffer,
    IMessageMetadata* output_metadata)
{
    return statement->openCursor(&st, tra, input_metadata, in_buffer, output_metadata, 0);
}

IStatement* Transaction::prepare_statement(unsigned int len_sql, const char *sql)
{
    return dba.get()->prepare(&st, tra, len_sql, sql, SQL_DIALECT_CURRENT,
        IStatement::PREPARE_PREFETCH_METADATA);
}

ITransaction *Transaction::get()
{
    FBDEBUG("Transaction::get()=%p", tra);
    if (tra) {
        return tra;
    }
    throw Php_Firebird_Exception(zend_ce_error, "Invalid transaction pointer");
}

void Transaction::set(ITransaction *new_tra)
{
    if (tra) {
        throw Php_Firebird_Exception(zend_ce_error, "Transaction already started");
    }

    tra = new_tra;
}

void Transaction::finalize(int mode)
{
    if (mode == FBP_TR_COMMIT) {
        commit();
    } else if (mode == (FBP_TR_ROLLBACK | FBP_TR_RETAIN)) {
        rollback_ret();
    } else if (mode == (FBP_TR_COMMIT | FBP_TR_RETAIN)) {
        commit_ret();
    } else {
        rollback();
    }
}

void Transaction::prepare()
{
    tra->prepare(&st, 0, nullptr);
}

}// namespace
