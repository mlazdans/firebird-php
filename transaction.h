#ifndef TRANSACTION_H
#define TRANSACTION_H

extern zend_class_entry *FireBird_Transaction_ce;
extern void register_FireBird_Transaction_ce();

void FireBird_Transaction___construct(zval *Tr, zval *Database, zval *Builder);
int FireBird_Transaction_prepare(zval *Tr, zval *return_Stmt, const ISC_SCHAR* sql);

#endif /* TRANSACTION_H */
