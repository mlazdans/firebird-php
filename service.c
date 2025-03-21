#include <firebird/fb_c_api.h>

#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"
#include "zend_exceptions.h"

zend_class_entry *FireBird_Service_ce, *FireBird_Server_Info_ce;
static zend_object_handlers FireBird_Service_object_handlers;

int service_build_dpb(zend_class_entry *ce, zval *args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written)
{
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_SPB_ATTACH, NULL, 0); // TODO: SPB?

    xpb_insert_tag(isc_spb_version3);
    if (FAILURE == xpb_insert_zmap(ce, args, xpb_zmap, xpb, st)) {
        return FAILURE;
    }

    *num_dpb_written = IXpbBuilder_getBufferLength(xpb, st);
    *dpb_buf = IXpbBuilder_getBuffer(xpb, st);

    return SUCCESS;
}

int service_connect(ISC_STATUS_ARRAY status, firebird_service *svc)
{
    zval rv, *service_name;
    const char *dpb_buffer;
    short num_dpb_written;

    service_name = zend_read_property(FireBird_Service_Connect_Args_ce, O_GET(&svc->args, service_name), 0, &rv);

    if ((Z_TYPE_P(service_name) != IS_STRING) || !Z_STRLEN_P(service_name)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "service_name argument is not set");
        return FAILURE;
    }

    if (FAILURE == service_build_dpb(FireBird_Service_Connect_Args_ce, &svc->args, &service_connect_zmap, &dpb_buffer, &num_dpb_written)) {
        return FAILURE;
    }

    FBDEBUG("Service::connect: %s", Z_STRVAL_P(service_name));
    // if (isc_service_attach(IB_STATUS, 0, loc, &handle, (unsigned short)spb_len, buf)) {
    if (isc_service_attach(status, (short)Z_STRLEN_P(service_name), Z_STRVAL_P(service_name), &svc->svc_handle, num_dpb_written, dpb_buffer)) {
        return FAILURE;
    }
    FBDEBUG_NOFL("  connected, handle: %d", svc->svc_handle);

    return SUCCESS;
}

PHP_METHOD(Service, connect)
{
    zval *args = NULL;
    ISC_STATUS_ARRAY status = {0};
    firebird_service *svc = Z_SERVICE_P(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(args, FireBird_Service_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    ZVAL_COPY_VALUE(&svc->args, args);

    zend_update_property(FireBird_Service_ce, THIS_SET(args));

    if (FAILURE == service_connect(status, svc)) {
        update_err_props(status, FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Service, disconnect) {
    ZEND_PARSE_PARAMETERS_NONE();

    ISC_STATUS_ARRAY status;
    firebird_service *svc = Z_SERVICE_P(ZEND_THIS);

    FBDEBUG("Service::disconnect()");

    if(svc->svc_handle) {
        FBDEBUG_NOFL("  Closing handle: %d", svc->svc_handle);
        if(isc_service_detach(status, &svc->svc_handle)){
            update_err_props(status, FireBird_Service_ce, ZEND_THIS);
        } else {
            svc->svc_handle = 0;
            RETURN_TRUE;
        }
    }

    RETURN_FALSE;
}

int service_get_server_info(ISC_STATUS_ARRAY status, zval *service, zval *server_info,
    size_t info_req_size, char *info_req,
    size_t info_resp_size, char *info_resp)
{
    firebird_service *svc = Z_SERVICE_P(service);
    static char spb[] = { isc_info_svc_timeout, 10, 0, 0, 0 };

    if (isc_service_query(status, &svc->svc_handle, NULL, sizeof(spb), spb,
        info_req_size, info_req,
        info_resp_size, info_resp)) {
            return FAILURE;
    }

    // TODO: IXpbBuilder code duplication all over the place.
    // Maybe using firebird_xpb_zmap?
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* dpb;

    dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_INFO_RESPONSE, info_resp, info_resp_size);

    ISC_INT64 len, total_len = 0;

#define READ_SVC_STRING(name) \
    case isc_info_svc_##name: zend_update_property_stringl(FireBird_Server_Info_ce, Z_OBJ_P(server_info), #name, sizeof(#name) - 1, b, len); break

    FBDEBUG("Parsing server info buffer");

    dump_buffer(IXpbBuilder_getBuffer(dpb, st), 50);

    for (IXpbBuilder_rewind(dpb, st); !IXpbBuilder_isEof(dpb, st); IXpbBuilder_moveNext(dpb, st)) {
        unsigned char tag = IXpbBuilder_getTag(dpb, st); total_len++;
        const ISC_UCHAR* b = IXpbBuilder_getBytes(dpb, st);
        len = IXpbBuilder_getLength(dpb, st); total_len += 2;
        total_len += len;

        FBDEBUG_NOFL("  tag: %d, len: %d", tag, len);

        switch(tag) {
            READ_SVC_STRING(server_version);
            READ_SVC_STRING(implementation);
            READ_SVC_STRING(get_env);
            READ_SVC_STRING(get_env_lock);
            READ_SVC_STRING(get_env_msg);
            READ_SVC_STRING(user_dbpath);

            case isc_info_end: break;
            case isc_info_truncated: {
                _php_firebird_module_error("Server info buffer error: truncated");
            } return FAILURE;
            case isc_info_error: {
                _php_firebird_module_error("Server info buffer error");
            } return FAILURE;

            default: {
                _php_firebird_module_fatal("BUG! Unhandled service info tag: %d", tag);
            } break;
        }
    }

    return SUCCESS;
}

PHP_METHOD(Service, get_server_info)
{
    ZEND_PARSE_PARAMETERS_NONE();

    char info_resp[1024] = { 0 };
    ISC_STATUS_ARRAY status = { 0 };

    static char info_req[] = {
        isc_info_svc_server_version,
        isc_info_svc_implementation,
        isc_info_svc_get_env,
        isc_info_svc_get_env_lock,
        isc_info_svc_get_env_msg,
        isc_info_svc_user_dbpath
    };

    object_init_ex(return_value, FireBird_Server_Info_ce);

    if(service_get_server_info(status, ZEND_THIS, return_value, sizeof(info_req), info_req, sizeof(info_resp), info_resp)) {
        update_err_props(status, FireBird_Service_ce, ZEND_THIS);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

const zend_function_entry FireBird_Service_methods[] = {
    PHP_ME(Service, connect, arginfo_FireBird_Service_connect, ZEND_ACC_PUBLIC)
    PHP_ME(Service, disconnect, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Service, get_server_info, arginfo_FireBird_Service_get_server_info, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Service_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Service_create()");

    firebird_service *s = zend_object_alloc(sizeof(firebird_service), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Service_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Service_free_obj");

    firebird_service *svc = Z_SERVICE_O(obj);

    if(svc->svc_handle) {
        ISC_STATUS_ARRAY status;
        if(isc_service_detach(status, &svc->svc_handle)) {
            status_fbp_error(status);
        } else {
            svc->svc_handle = 0;
        }
    }

    zend_object_std_dtor(&svc->std);
}

void register_FireBird_Service_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Service", FireBird_Service_methods);
    FireBird_Service_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Service_ce, args, FireBird\\Service_Connect_Args, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Service_ce);

    zend_class_implements(FireBird_Service_ce, 1, FireBird_IError_ce);

    FireBird_Service_ce->create_object = FireBird_Service_create;
    FireBird_Service_ce->default_object_handlers = &FireBird_Service_object_handlers;

    memcpy(&FireBird_Service_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Service_object_handlers.offset = XtOffsetOf(firebird_service, std);
    FireBird_Service_object_handlers.free_obj = FireBird_Service_free_obj;
}

void register_FireBird_Server_Info_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Server_Info", NULL);
    FireBird_Server_Info_ce = zend_register_internal_class(&tmp_ce);

    declare_props_zmap(FireBird_Server_Info_ce, &server_info_zmap);
}
