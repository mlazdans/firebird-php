#pragma once

#include <cstddef>
#include <memory>
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

// Owns an IXpbBuilder, disposing it on scope exit (including when a Firebird
// call throws mid-way).
struct XpbBuilderDeleter {
    void operator()(IXpbBuilder *p) const noexcept { if (p) p->dispose(); }
};
using XpbBuilderPtr = std::unique_ptr<IXpbBuilder, XpbBuilderDeleter>;

// A standalone status for code outside Base-derived classes. Disposed on scope exit.
class ScopedStatus: public ThrowStatusWrapper
{
    ScopedStatus(const ScopedStatus&) = delete;
    ScopedStatus& operator=(const ScopedStatus&) = delete;
public:
    ScopedStatus(): ThrowStatusWrapper(fb_get_master_interface()->getStatus()) {}
    ~ScopedStatus() { dispose(); }
};

class Base
{
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
protected:
    IMaster* master;
    ThrowStatusWrapper st;
public:
    Base(): master(fb_get_master_interface()), st(master->getStatus())
    {
    }
    ~Base()
    {
        st.dispose();
    }
};

} // namespace
