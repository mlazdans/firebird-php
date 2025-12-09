#include <vector>
#include <memory>
#include <firebird/Interface.h>
#include <stdint.h>

#include "firebird_utils.h"

#include "fbp/base.hpp"
#include "fbp/service.hpp"

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

#define READ_SI_STRING(name)                                \
    case isc_info_svc_##name: {                             \
        zend_update_property_string(                        \
            FireBird_Server_Info_ce, Z_OBJ_P(server_info),  \
            #name, sizeof(#name) - 1, spb->getString(&st)); \
    } break

void Service::get_info(zval *server_info)
{
    // TODO: separate only SYSDBA accessible information
    const unsigned char info_req[] = {
        isc_info_svc_server_version,
        isc_info_svc_implementation,
        isc_info_svc_get_env,
        isc_info_svc_get_env_lock,
        isc_info_svc_get_env_msg,
        isc_info_svc_user_dbpath,
    };

    unsigned char info_resp[1024] = { 0 };

    // static char spb[] = { isc_info_svc_timeout, 10, 0, 0, 0 };
    // static char action[] = { isc_action_svc_display_user_adm };

    // query(StatusType* status, unsigned sendLength, const unsigned char* sendItems,
    // unsigned receiveLength, const unsigned char* receiveItems, unsigned bufferLength, unsigned char* buffer)
    srv->query(&st, 0, NULL, sizeof(info_req), info_req, sizeof(info_resp), info_resp);

    auto spb =  master->getUtilInterface()->getXpbBuilder(
        &st, IXpbBuilder::SPB_RESPONSE, info_resp, sizeof(info_resp));

    for (spb->rewind(&st); !spb->isEof(&st); spb->moveNext(&st)) {
        auto tag = spb->getTag(&st);

        if (tag == isc_info_end) {
            break;
        }

        switch(tag)
        {
            READ_SI_STRING(server_version);
            READ_SI_STRING(implementation);
            READ_SI_STRING(get_env);
            READ_SI_STRING(get_env_lock);
            READ_SI_STRING(get_env_msg);
            READ_SI_STRING(user_dbpath);
            default:
                fbp_fatal("BUG! Unhandled isc_info_svc tag: %d", tag);
        }
    }
}

#define READ_UI_STRING(name)                          \
    isc_spb_sec_##name: {                             \
        len = isc_portable_integer(p, 2); p += 2;     \
        zend_update_property_stringl(                 \
            FireBird_User_Info_ce, Z_OBJ(user_info),  \
             #name, sizeof(#name) - 1, (char *)p, len \
        );                                            \
        p += len;                                     \
    } break

#define READ_UI_LONG(name)                           \
    isc_spb_sec_##name: {                            \
        len = 4;                                     \
        zend_update_property_long(                   \
            FireBird_User_Info_ce, Z_OBJ(user_info), \
            #name, sizeof(#name) - 1,                \
            isc_portable_integer(p, len)             \
        );                                           \
        p += len;                                    \
    } break

void Service::get_users(zval *users)
{
    const unsigned char req[] = { isc_info_svc_get_users };
    unsigned char resp[1024] = { 0 };

    const unsigned char send[] = { isc_info_svc_timeout, 10, 0, 0, 0 };
    const unsigned char action[] = { isc_action_svc_display_user_adm };

    srv->start(&st, sizeof(action), action);
    srv->query(&st, sizeof(send), send, sizeof(req), req, sizeof(resp), resp);

    auto p = resp;
    auto end = p + sizeof(resp);

    if (*p++ != isc_info_svc_get_users) {
        fbp_fatal("BUG! Expected isc_info_svc_get_users but got: %d", *p);
    }
    auto resp_len = isc_portable_integer(p, 2); p += 2;

    zval user_info;
    ISC_INT64 len, total_len = 0;
    unsigned char tag;

    for (;;) {
        tag = *p++;

        // Assuming isc_spb_sec_username is always first tag in user info
        if (tag == isc_spb_sec_username || tag == isc_info_end) {
            if (Z_TYPE(user_info) == IS_OBJECT) {
                add_next_index_zval(users, &user_info);
            }
        }

        if (tag == isc_spb_sec_username) {
            object_init_ex(&user_info, FireBird_User_Info_ce);
        }

        if (tag == isc_info_end) {
            break;
        }

        // TODO: isc_info_truncated
        switch(tag)
        {
            case READ_UI_STRING(username);
            case READ_UI_STRING(firstname);
            case READ_UI_STRING(middlename);
            case READ_UI_STRING(lastname);
            case READ_UI_LONG(admin);
            case isc_spb_sec_userid:
            case isc_spb_sec_groupid:
                p += 4;
                break;
            default:
                fbp_fatal("BUG! Unhandled isc_info_svc tag: %d", tag);
        }
    }

    return;
}

void Service::add_user(zval *user_info)
{
    add_or_modify_user(user_info, isc_action_svc_add_user);
}

void Service::modify_user(zval *user_info)
{
    add_or_modify_user(user_info, isc_action_svc_modify_user);
}

void Service::add_or_modify_user(zval *user_info, const ISC_UCHAR tag)
{
    auto xpb = master->getUtilInterface()->getXpbBuilder(&st, IXpbBuilder::SPB_START, NULL, 0);

    xpb->insertTag(&st, tag);
    fbu_xpb_insert_object(xpb, user_info, FireBird_User_Info_ce, &fbp_user_info_zmap);
    srv->start(&st, xpb->getBufferLength(&st), xpb->getBuffer(&st));
}

void Service::delete_user(const char *username, size_t username_len)
{
    auto xpb = master->getUtilInterface()->getXpbBuilder(&st, IXpbBuilder::SPB_START, NULL, 0);

    xpb->insertTag(&st, isc_action_svc_delete_user);
    xpb->insertBytes(&st, isc_spb_sec_username, username, username_len);
    srv->start(&st, xpb->getBufferLength(&st), xpb->getBuffer(&st));
}

void Service::get_db_info(zval *server_db_info)
{
    // TODO: separate only SYSDBA accessible information
    const unsigned char info_req[] = { isc_info_svc_svr_db_info };
    unsigned char info_resp[1024] = { 0 };

    srv->query(&st, 0, NULL, sizeof(info_req), info_req, sizeof(info_resp), info_resp);

    auto spb =  master->getUtilInterface()->getXpbBuilder(&st, IXpbBuilder::SPB_RESPONSE, info_resp, sizeof(info_resp));

    zval databases;
    array_init(&databases);
    for (spb->rewind(&st); !spb->isEof(&st); spb->moveNext(&st)) {
        auto tag = spb->getTag(&st);

        if (tag == isc_info_end) {
            break;
        }

        switch(tag)
        {
            case isc_info_flag_end:
            case isc_info_svc_svr_db_info:
                break;
            case isc_spb_num_att:
                PROP_SET_LONG(FireBird_Server_Db_Info_ce, server_db_info, "num_att", spb->getBigInt(&st));
                break;
            case isc_spb_num_db:
                PROP_SET_LONG(FireBird_Server_Db_Info_ce, server_db_info, "num_db", spb->getBigInt(&st));
                break;
            case isc_spb_dbname:
                add_next_index_stringl(&databases, (char *)spb->getBytes(&st), spb->getLength(&st));
                break;
            default:
                fbp_fatal("BUG! Unhandled isc_info_svc tag: %d", tag);
        }
    }

    zend_update_property(FireBird_Server_Db_Info_ce, Z_OBJ_P(server_db_info), "databases", sizeof("databases") - 1, &databases);
    zval_ptr_dtor(&databases);

    return;
}


} // namespace
