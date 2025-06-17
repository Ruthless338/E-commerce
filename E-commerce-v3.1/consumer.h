#ifndef CONSUMER_H
#define CONSUMER_H
#include "user.h"

class Consumer : public User
{
    Q_OBJECT
public:
    explicit Consumer(const QString& uname, const QString& pwd, const double& bal):
        User(uname,pwd,bal) {}
    explicit Consumer(const User& user) : User(user) {}
    QString getUserType() const override;
};

#endif // CONSUMER_H
