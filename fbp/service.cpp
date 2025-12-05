#include <vector>
#include <memory>
#include <firebird/Interface.h>
#include <stdint.h>

#include "firebird_utils.h"

#include "fbp/base.hpp"
#include "fbp/service.hpp"

extern "C" {
// #include "zend_exceptions.h"
}

using namespace Firebird;

namespace FBP {

Service::Service()
{
}

void Service::connect(zval *args)
{
    auto util = master->getUtilInterface();
    auto prov = master->getDispatcher();

    // TODO: create normal C struct
    if (!args || Z_TYPE_P(args) != IS_OBJECT || Z_OBJCE_P(args) != FireBird_Service_Connect_Args_ce) {
        throw Php_Firebird_Exception(zend_ce_value_error, "Expected instanceof Service_Connect_Args");
    }

    zval rv;
    zval *service_name = PROP_GET(FireBird_Service_Connect_Args_ce, args, "service_name");
    IXpbBuilder* dpb = nullptr;

    // TODO: process other props
    dpb = util->getXpbBuilder(&st, IXpbBuilder::SPB_ATTACH, NULL, 0);
    fbu_xpb_insert_object(dpb, args, FireBird_Service_Connect_Args_ce, &fbp_service_connect_zmap);

    // dpb->insertInt(&status, isc_dpb_page_size, 4 * 1024);
    // dpb->insertString(&st, isc_dpb_user_name, "sysdba");
    // dpb->insertString(&st, isc_dpb_password, "masterkey");

    // attachServiceManager(const char* service, unsigned spbLength, const unsigned char* spb)
    srv = prov->attachServiceManager(&st, Z_STRVAL_P(service_name),
        dpb->getBufferLength(&st), dpb->getBuffer(&st));
}

void Service::disconnect()
{
    srv->detach(&st);
    srv->release();
    srv = nullptr;
}

Service::~Service()
{
    FBDEBUG("~Service(this=%p)", this);

    int err = 0;
    try
    {
        // trans_list.clear();
        if (srv) disconnect();
    }
    catch (...)
    {
        err = 1;
    }

    if (srv) {
        srv->release();
        srv = nullptr;
    }

    if (err) fbu_handle_exception(__FILE__, __LINE__);
}

} // namespace
