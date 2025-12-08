#pragma once

#include <firebird/Interface.h>

#include "fbp/base.hpp"

#include "firebird_php.hpp"

extern "C" {

// #include <ibase.h>

typedef struct firebird_service {
    size_t svh;

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

void register_FireBird_Service_object_handlers();

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
    // IService *get();
    void connect(zval *args);
    void disconnect();
    void get_info(zval *server_info);
    void get_users(zval *users);
    void add_user(zval *user_info);
    void modify_user(zval *user_info);
    void add_or_modify_user(zval *user_info, const ISC_UCHAR tag);
    void delete_user(const char *username, size_t username_len);
    void get_db_info(zval *server_db_info);
};

}
