#ifndef FBP_SERVICE_H
#define FBP_SERVICE_H

typedef struct firebird_service {
    isc_svc_handle svc_handle;
    // zval args;
    // char *hostname;
    // char *username;
    // zend_resource *res;

    // zval instance;
    zend_object std;
} firebird_service;

fbp_declare_object_accessor(firebird_service);

extern firebird_xpb_zmap fbp_service_connect_zmap;
extern firebird_xpb_zmap fbp_server_info_zmap;
extern firebird_xpb_zmap fbp_user_info_zmap;

int fbp_service_build_dpb(zend_class_entry *ce, zval *Args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written);
int fbp_service_connect(firebird_service *svc, zval *Service_Connect_Args);
int fbp_service_get_server_info(firebird_service *svc, zval *Server_Info,
    size_t req_size, char *req_buff,
    size_t resp_size, char *resp_buff);
int fbp_service_addmod_user(firebird_service *svc, zval *User_Info, const ISC_UCHAR tag);
int fbp_service_delete_user(firebird_service *svc, const char *username, ISC_USHORT username_len);

#endif /* FBP_SERVICE_H */
