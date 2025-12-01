#pragma once

#include <cstddef>
#include <stdexcept>
#include <firebird/Interface.h>

extern "C" {
#include "php.h"
}

#define PTR(x) static_cast<void*>(&(x))

using namespace Firebird;

class Php_Firebird_Exception : public std::runtime_error
{
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
