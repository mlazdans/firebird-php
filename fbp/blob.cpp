#include <algorithm>
#include <firebird/Interface.h>

#include "./base.hpp"
#include "./database.hpp"
#include "./blob.hpp"
#include "../firebird_php.hpp"
#include "../firebird_utils.h"

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

    try
    {
        if (blob) blob->close(&st);
    }
    catch (...)
    {
        fbu_report_exception_as_warning(__FILE__, __LINE__);
    }

    if (blob) {
        blob->release();
        blob = nullptr;
    }

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

// Segment lengths are 16-bit on the wire: anything longer is silently cut modulo 64K,
// so reads and writes go in chunks of at most this many bytes.
static constexpr unsigned BLOB_CHUNK = 0xFFFF;

void Blob::get_contents(int64_t max_len, zend_string **buf)
{
    if (info.is_writable) {
        throw Php_Firebird_Exception(zend_ce_error, "Attempted to read from a writable BLOB");
    }

    int64_t remaining_len = info.total_length - info.position;
    if (remaining_len < 0) remaining_len = 0;

    size_t want = (max_len <= 0 || max_len > remaining_len) ? static_cast<size_t>(remaining_len) : static_cast<size_t>(max_len);

    zend_string *str = zend_string_alloc(want, 0);
    size_t got = 0;

    try {
        while (got < want) {
            unsigned chunk = static_cast<unsigned>(std::min<size_t>(want - got, BLOB_CHUNK));
            unsigned len = 0;
            int res = get()->getSegment(&st, chunk, ZSTR_VAL(str) + got, &len);
            got += len;
            if (res == IStatus::RESULT_NO_DATA) break;
        }
    } catch (...) {
        zend_string_efree(str);
        throw;
    }

    ZSTR_LEN(str) = got;
    ZSTR_VAL(str)[got] = '\0';
    *buf = str;

    FBDEBUG("Blob::get_contents(position=%ld, remaining_len=%ld, max_len=%ld, read=%zu)",
        (long)info.position, (long)remaining_len, (long)max_len, got);

    info.position += got;
}

void Blob::put_contents(size_t buf_size, const char *buf)
{
    if (!info.is_writable) {
        throw Php_Firebird_Exception(zend_ce_error, "Attempted to write to a read-only BLOB");
    }

    for (size_t done = 0; done < buf_size; ) {
        unsigned chunk = static_cast<unsigned>(std::min<size_t>(buf_size - done, BLOB_CHUNK));
        get()->putSegment(&st, chunk, buf + done);
        done += chunk;
        info.position += chunk;
        info.total_length += chunk;
    }
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

    XpbBuilderPtr b(util->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, sizeof(resp)));

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

    assert(info.position <= info.total_length);
}

const firebird_blob_info *Blob::get_info()
{
    return &info;
}

void Blob::close()
{
    get()->close(&st);
    blob->release();
    blob = nullptr;
}

void Blob::cancel()
{
    get()->cancel(&st);
    blob->release();
    blob = nullptr;
}

void Blob::seek(int mode, int offset, int *new_offset)
{
    *new_offset = get()->seek(&st, mode, offset);
    info.position = *new_offset;
}

bool Blob::has_blob()
{
    return blob != nullptr;
}

IBlob *Blob::get()
{
    FBDEBUG("Blob::get()=%p", blob);
    if (blob) {
        return blob;
    }
    throw Php_Firebird_Exception(zend_ce_error, "Invalid blob pointer");
}

} // namespace
