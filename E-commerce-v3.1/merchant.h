#ifndef MERCHANT_H
#define MERCHANT_H
#include "user.h"

class Merchant : public User
{
public:
    explicit Merchant(const QString& uname, const QString& pwd, const double& bal):User(uname,pwd,bal) {}
    explicit Merchant(const User& user) : User(user) {}
    QString getUserType() const override;
};

#endif // MERCHANT_H
