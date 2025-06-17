#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QJsonObject>
#include "globalstate.h" // 需要包含 GlobalState 来更新它

class AuthManager : public QObject { // QObject 基类是为了使用信号槽机制（如果 sendRequestAndWait 内部需要）
    Q_OBJECT // 即使是静态方法为主的类，为了配合 QEventLoop，有时会临时创建实例或使用全局实例

public:
    // 保持与单机版完全一致的静态方法签名
    Q_INVOKABLE static bool verifyLogin(const QString &username, const QString &password);
    Q_INVOKABLE static QVariantMap registerUser(const QString &username, const QString &pwd, const QString &type); // 单机版可能没有balance参数
    Q_INVOKABLE static bool changePassword(const QString &username, const QString &oldPwd, const QString &newPwd);
    Q_INVOKABLE static bool recharge(const QString& username, double amount);
    Q_INVOKABLE static double getBalance(const QString& username); // 假设单机版 User 类有此方法

    // 单机版中 User 类的方法，现在移到 AuthManager 并通过网络实现
    Q_INVOKABLE static bool deductBalance(const QString& username, double amount);
    Q_INVOKABLE static bool addBalance(const QString& username, double amount);
    Q_INVOKABLE static QString getUserType(const QString& username); // 假设 User 类有此方法

    // 辅助函数，发送请求并等待响应
    // 这个函数现在需要一个机制来确保它只处理它发出的那个请求的响应
    static QJsonObject sendRequestAndWait(const QJsonObject& requestData, int timeoutMs = 5000);
};

#endif // AUTHMANAGER_H
