#ifndef STATEMENT_H
#define STATEMENT_H

#include <ibase.h>
#include "php.h"

#define FBP_FETCH_BLOB_TEXT (1<<0)
#define FBP_FETCH_UNIXTIME  (1<<1)
#define FBP_FETCH_DATE_OBJ  (1<<2)
#define FBP_FETCH_BLOB_OBJ  (1<<3)
#define FBP_FETCH_INDEXED   (1<<4)
#define FBP_FETCH_HASHED    (1<<5)

typedef struct firebird_field {
    const char *alias;
    unsigned int type;
    unsigned int len;
    int scale;
    unsigned int offset;
    unsigned int null_offset;
} firebird_field;

typedef struct firebird_stmt_info {
    unsigned char statement_type;
    const char *sql;
    unsigned int sql_len;

    const char *name;
    ISC_ULONG insert_count, update_count, delete_count, select_count, affected_count;

    unsigned int in_vars_count, out_vars_count;
} firebird_stmt_info;

typedef struct firebird_stmt {
    void *sptr;
    firebird_stmt_info *info;

    unsigned char did_fake_fetch, is_cursor_open, is_exhausted;

    zend_object std;
} firebird_stmt;

fbp_declare_object_accessor(firebird_stmt);

typedef struct firebird_vary {
    unsigned short vary_length;
    unsigned char vary_string[1];
} firebird_vary;

void FireBird_Statement___construct(zval *self, zval *transaction);
int FireBird_Statement_prepare(zval *self, zend_string *sql);
int FireBird_Statement_execute(zval *self, zval *bind_args, uint32_t num_bind_args);
void FireBird_Statement_fetch(zval *self, int flags, zval *return_value);

void register_FireBird_Statement_object_handlers();

#endif /* STATEMENT_H */
