#ifndef DATABASE_H
#define DATABASE_H

#include <ibase.h>
#include "php.h"
#include "php_firebird_includes.h"

extern zend_class_entry *FireBird_Database_ce;
extern zend_class_entry *FireBird_Db_Info_ce;
extern zend_class_entry *FireBird_Connect_Args_ce;
extern zend_class_entry *FireBird_Create_Args_ce;

void register_FireBird_Database_ce();
void register_FireBird_Db_Info_ce();
void register_FireBird_Connect_Args_ce();
void register_FireBird_Create_Args_ce();

void FireBird_Database_reconnect_transaction(zval *Db, zval *return_value, zend_long id);

#endif /* DATABASE_H */
