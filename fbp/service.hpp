#pragma once

#include <firebird/Interface.h>

#include "fbp/base.hpp"

#include "firebird_php.hpp"

extern "C" {

// #include <ibase.h>

typedef struct firebird_service {
    // isc_svc_handle svc_handle;
    // zval args;
    // char *hostname;
    // char *username;
    // zend_resource *res;

    // zval instance;
    zend_object std;
} firebird_service;

// typedef enum {
//     FBP_SM_NORMAL = isc_spb_prp_sm_normal,
//     FBP_SM_MULTI  = isc_spb_prp_sm_multi,
//     FBP_SM_SINGLE = isc_spb_prp_sm_single,
//     FBP_SM_FULL   = isc_spb_prp_sm_full,
// } firebird_shutdown_mode;

fbp_declare_object_accessor(firebird_service);

extern firebird_xpb_zmap fbp_service_connect_zmap;
extern firebird_xpb_zmap fbp_server_info_zmap;
extern firebird_xpb_zmap fbp_user_info_zmap;

// extern void register_FireBird_Service_ce();
// extern void register_FireBird_Service_Connect_Args_ce();
// extern void register_FireBird_Server_Info_ce();
// extern void register_FireBird_Server_Db_Info_ce();
// extern void register_FireBird_Server_User_Info_ce();

}

using namespace Firebird;

namespace FBP {

class Service: Base
{
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
    Service(Service&&) = default;
    Service& operator=(Service&&) = default;
private:
    IService *srv = nullptr;
public:
    Service();
    ~Service();
    IService *get();
    void connect(zval *args);
    void disconnect();
};

}
