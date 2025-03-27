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
int fbp_service_shutdown_db(firebird_service *svc, size_t dbname_len, char *dbname, ISC_UCHAR mode, ISC_ULONG timeout);
int fbp_service_db_online(firebird_service *svc, size_t dbname_len, char *dbname, ISC_UCHAR mode);
int fbp_service_set_page_buffers(firebird_service *svc, size_t dbname_len, char *dbname, ISC_ULONG buffers);
int fbp_service_set_sweep_interval(firebird_service *svc, size_t dbname_len, char *dbname, ISC_ULONG interval);
int fbp_service_deny_new_attachments(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_deny_new_transactions(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_set_write_mode_async(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_set_write_mode_sync(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_set_access_mode_readonly(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_set_access_mode_readwrite(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_enable_reserve_space(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_disable_reserve_space(firebird_service *svc, size_t dbname_len, char *dbname);
int fbp_service_set_sql_dialect(firebird_service *svc, size_t dbname_len, char *dbname, ISC_ULONG dialect);
int fbp_service_backup(firebird_service *svc, size_t dbname_len, char *dbname, size_t bkpname_len, char *bkpname);
int fbp_service_restore(firebird_service *svc, size_t bkpname_len, char *bkpname, size_t dbname_len, char *dbname);

#endif /* FBP_SERVICE_H */
