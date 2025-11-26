#ifndef SERVICE_H
#define SERVICE_H

#include <ibase.h>
#include "php.h"

extern zend_class_entry *FireBird_Service_ce;
extern zend_class_entry *FireBird_Service_Connect_Args_ce;
extern zend_class_entry *FireBird_Server_Info_ce;
extern zend_class_entry *FireBird_Server_Db_Info_ce;
extern zend_class_entry *FireBird_Server_User_Info_ce;

extern void register_FireBird_Service_ce();
extern void register_FireBird_Service_Connect_Args_ce();
extern void register_FireBird_Server_Info_ce();
extern void register_FireBird_Server_Db_Info_ce();
extern void register_FireBird_Server_User_Info_ce();

#endif /* SERVICE_H */