#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include "user.h"

class AuthManager : public QObject {
    Q_OBJECT
public:
    explicit AuthManager(QObject *parent = nullptr):QObject(parent) {}

    Q_INVOKABLE static bool verifyLogin(const QString &username, const QString &password) {
        return User::verifyLogin(username, password);
    }
    Q_INVOKABLE static QVariantMap registerUser(const QString &username, const QString &pwd, const QString &type, double balance) {
        return User::registerUser(username, pwd, type, balance);
    }
    Q_INVOKABLE static bool changePassword(const QString &username,
                                           const QString &oldPwd,
                                           const QString &newPwd);
    Q_INVOKABLE static bool verifyPassword(const QString &username,
                                           const QString &pwd);
};

#endif // AUTHMANAGER_H
