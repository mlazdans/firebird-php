#include <firebird/fb_c_api.h>

#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"
#include "zend_exceptions.h"

zend_class_entry *FireBird_Service_ce;
static zend_object_handlers FireBird_Service_object_handlers;

int service_build_dpb(zend_class_entry *ce, zval *args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written)
{
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_DPB, NULL, 0);

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

const zend_function_entry FireBird_Service_methods[] = {
    PHP_ME(Service, connect, arginfo_FireBird_Service_connect, ZEND_ACC_PUBLIC)
    PHP_ME(Service, disconnect, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
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
