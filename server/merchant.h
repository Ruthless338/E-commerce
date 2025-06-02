#ifndef MERCHANT_H
#define MERCHANT_H
#include "user.h"

class Merchant : public User
{
public:
    Merchant(const QString& uname, const QString& pwd, const double& bal):User(uname,pwd,bal) {}

    QString getUserType() const override;
};

#endif // MERCHANT_H
