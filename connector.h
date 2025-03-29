#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "php.h"

extern zend_class_entry *FireBird_Connector_ce;

void register_FireBird_Connector_ce();

int FireBird_Connector_connect(zval *Conn, zval *Db, zval *Connect_Args);
int FireBird_Connector_create(zval *Conn, zval *Db, zval *Create_Args);

#endif /* CONNECTOR_H */
