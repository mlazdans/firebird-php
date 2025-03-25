#ifndef FBP_DATABASE_H
#define FBP_DATABASE_H

#include <ibase.h>
#include "php.h"

typedef struct firebird_db {
    isc_db_handle db_handle;
    zval args;

    zend_object std;
} firebird_db;

#endif /* FBP_DATABASE_H */
