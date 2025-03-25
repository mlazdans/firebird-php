#ifndef DATABASE_H
#define DATABASE_H

#include <ibase.h>
#include "php.h"
#include "php_firebird_includes.h"

extern zend_class_entry *FireBird_Database_ce;
extern zend_class_entry *FireBird_Db_Info_ce;

void register_FireBird_Database_ce();
void register_FireBird_Db_Info_ce();

int _FireBird_Database_build_dpb(zend_class_entry *ce, zval *Args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written);

#endif /* DATABASE_H */