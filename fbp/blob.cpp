#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/blob.hpp"
#include "firebird_php.hpp"
#include "firebird_utils.h"

using namespace Firebird;

namespace FBP {

Blob::Blob(Transaction &tra)
    : tra{tra}
{
    FBDEBUG("new Blob(this=%p)", PTR(*this));
}

Blob::~Blob() noexcept
{
    FBDEBUG("~Blob(this=%p)", this);

    int err = 0;
    try
    {
        if (blob) blob->close(&st);
    }
    catch (...)
    {
        err = 1;
    }

    if (blob) {
        blob->release();
        blob = nullptr;
    }

    if (err) fbu_handle_exception(__FILE__, __LINE__);
}

ISC_QUAD Blob::create()
{
    blob = tra.create_blob(&info.id);

    query_info();
    info.is_writable = 1;

    return info.id;
}

void Blob::open(ISC_QUAD *blob_id)
{
    blob = tra.open_blob(blob_id);

    query_info();

    info.id = *blob_id;
    info.is_writable = 0;
}

void Blob::get_contents(int max_len, zend_string **buf)
{
    unsigned int len = 0;
    int64_t remaining_len = info.total_length - info.position;

    // MAYBE: some limits?
    // if (remaining_len > INT32_MAX) {
    //     throw Php_Firebird_Exception(zend_ce_error, "Buffer too large");
    // }

    if (max_len <= 0 || max_len > remaining_len) {
        max_len = remaining_len;
    }

    FBDEBUG("Blob::get_contents(position=%ld, remaining_len=%ld, max_len=%ld)",
        info.position, remaining_len, max_len);

    *buf = zend_string_alloc(max_len, 0);

    blob->getSegment(&st, max_len, &ZSTR_VAL(*buf), &len);
    ZSTR_VAL(*buf)[len] = '\0';

    info.position += len;
}

void Blob::put_contents(unsigned int buf_size, const char *buf)
{
    blob->putSegment(&st, buf_size, buf);
    info.position += buf_size;
    info.total_length += buf_size;
}

void Blob::query_info()
{
    auto util = master->getUtilInterface();

    const unsigned char req[] = {
        isc_info_blob_num_segments,
        isc_info_blob_max_segment,
        isc_info_blob_total_length,
        isc_info_blob_type
    };

    unsigned char resp[32] = {0};

    blob->getInfo(&st, sizeof(req), req, sizeof(resp), resp);

    IXpbBuilder* b = util->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, sizeof(resp));

    for (b->rewind(&st); !b->isEof(&st); b->moveNext(&st)) {
        unsigned char tag = b->getTag(&st);
        int val = b->getInt(&st);

        // FBDEBUG_NOFL(" tag: %d, val: %d", tag, val);
        switch(tag) {
            case isc_info_blob_num_segments:
                info.num_segments = val;
                break;
            case isc_info_blob_max_segment:
                info.max_segment = val;
                break;
            case isc_info_blob_total_length:
                info.total_length = val;
                break;
            case isc_info_blob_type:
                info.type = val;
                break;
        }
    }
}

const firebird_blob_info *Blob::get_info()
{
    return &info;
}

void Blob::close()
{
    if (blob) {
        blob->close(&st);
        blob->release();
        blob = nullptr;
    }
}

void Blob::cancel()
{
    if (blob) {
        blob->cancel(&st);
        blob->release();
        blob = nullptr;
    }
}

void Blob::seek(int mode, int offset, int *new_offset)
{
    *new_offset = blob->seek(&st, mode, offset);
    info.position = *new_offset;
}

bool Blob::has_blob()
{
    return blob != nullptr;
}

} // namespace
