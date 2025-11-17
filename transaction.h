#ifndef TRANSACTION_H
#define TRANSACTION_H

void FireBird_Transaction___construct(zval *Tr, zval *Database, zval *Builder);
int FireBird_Transaction_prepare(zval *Tr, zval *return_Stmt, const ISC_SCHAR* sql);
void register_FireBird_Transaction_object_handlers();

#endif /* TRANSACTION_H */
