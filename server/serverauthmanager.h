#ifndef SERVERAUTHMANAGER_H
#define SERVERAUTHMANAGER_H
#include <QObject>
#include <QVariantMap>
// No User.h needed here if all logic is self-contained or passed in

class ServerAuthManager : public QObject {
    Q_OBJECT
public:
    explicit ServerAuthManager(QObject *parent = nullptr);
    QVariantMap verifyLogin(const QString &username, const QString &password);
    QVariantMap registerUser(const QString &username, const QString &pwd, const QString &type, double balance);
    bool changePassword(const QString &username, const QString &oldPwd, const QString &newPwd);
    // ... other methods from your AuthManager ...
    bool recharge(const QString& username, double amount);
    double getBalance(const QString& username);
    bool deductBalance(const QString& username, double amount);
    bool addBalance(const QString& username, double amount);
    QString getUserType(const QString& username);
};
#endif
