#pragma once

#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/transaction.hpp"

using namespace Firebird;

namespace FBP {

class Blob: Base
{
private:
    Transaction *tra;
    IBlob *blob = nullptr;
    // ISC_QUAD id = {0};
    firebird_blob_info info = {0};

    void set_info();
public:
    Blob(Transaction *tra);
    ~Blob() noexcept;
    ISC_QUAD create();
    void open(ISC_QUAD id);
    zend_string *get_contents(ISC_LONG max_len);
    void put_contents(unsigned int buf_size, const char *buf);
    firebird_blob_info *get_info();
    void close();
    void cancel();
    void seek(int mode, int offset, int *new_offset);
};

}
