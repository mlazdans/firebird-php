#pragma once

#include <cstddef>
#include <stdexcept>
#include <firebird/Interface.h>

extern "C" {
#include "php.h"
#include "php_firebird_includes.h"
}

#define PTR(x) static_cast<void*>(&(x))

using namespace Firebird;

void fbu_handle_exception(const char *file, size_t line);
void fbu_xpb_insert_object(IXpbBuilder* xpb, zval *obj, zend_class_entry *ce,
    const firebird_xpb_zmap *xpb_zmap);

class Php_Firebird_Exception : public std::runtime_error {
public:
    zend_class_entry *ce;
    explicit Php_Firebird_Exception(zend_class_entry *ce, const std::string& msg)
        : std::runtime_error(msg),
        ce(ce)
        {}
};

namespace FBP
{

class Base
{
protected:
    IMaster* master;
    ThrowStatusWrapper st;
public:
    Base(): master(fb_get_master_interface()), st(master->getStatus())
    {
    }
};

} // namespace
