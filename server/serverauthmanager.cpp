#include "serverauthmanager.h"
#include "filemanager.h"
#include "user.h"
#include "consumer.h"
#include "merchant.h"
#include <QDebug>

ServerAuthManager::ServerAuthManager(QObject *parent) : QObject(parent) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    qInfo() << "ServerAuthManager: Loaded " << users.count() << "users.";
    qDeleteAll(users); // Clean up if only for test
}

QVariantMap ServerAuthManager::verifyLogin(const QString &username, const QString &password) {
    QMap<QString, User*> users = FileManager::loadAllUsers(); // 每次都从文件加载，保证最新
    QVariantMap result;

    if (!users.contains(username)) {
        result["success"] = false;
        result["error"] = "User not found.";
        qDeleteAll(users); // 清理内存
        return result;
    }

    User* user = users[username];
    if (user->verifyPassword(password)) { // User::verifyPassword 内部会哈希输入密码
        result["success"] = true;
        QVariantMap userData;
        userData["username"] = user->getUsername();
        userData["type"] = user->getUserType();
        userData["balance"] = user->getBalance(); // 发送余额给客户端
        result["userData"] = userData;
        qInfo() << "ServerAuthManager: User" << username << "logged in successfully.";
    } else {
        result["success"] = false;
        result["error"] = "Incorrect password.";
        qInfo() << "ServerAuthManager: User" << username << "login failed: incorrect password.";
    }

    qDeleteAll(users); // 清理从 loadAllUsers 获取的 User 对象
    return result;
}

QVariantMap ServerAuthManager::registerUser(const QString &username, const QString &pwd, const QString &type, double balance) {
    // balance 参数通常由服务器设定为0，客户端传来的会被忽略，除非有特殊业务逻辑
    QMap<QString, User*> users = FileManager::loadAllUsers();
    QVariantMap result;

    if (username.isEmpty() || pwd.isEmpty() || type.isEmpty()) {
        result["success"] = false;
        result["error"] = "Username, password, and type cannot be empty.";
        qDeleteAll(users);
        return result;
    }
    if (pwd.length() < 6) { // 与客户端一致的密码长度校验
        result["success"] = false;
        result["error"] = "Password must be at least 6 characters long.";
        qDeleteAll(users);
        return result;
    }


    if (users.contains(username)) {
        result["success"] = false;
        result["error"] = "Username already exists.";
    } else {
        User* newUser = nullptr;
        QString hashedPwd = User::hashPassword(pwd); // 使用 User 类的静态哈希方法
        double initialBalance = 0.0; // 新用户默认余额为0

        if (type == "Consumer") {
            newUser = new Consumer(username, hashedPwd, initialBalance);
        } else if (type == "Merchant") {
            newUser = new Merchant(username, hashedPwd, initialBalance);
        } else {
            result["success"] = false;
            result["error"] = "Invalid user type specified.";
            qDeleteAll(users); // 清理已加载的用户
            return result;
        }

        if (FileManager::saveUser(newUser)) { // FileManager::saveUser 会处理替换和写入
            result["success"] = true;
            qInfo() << "ServerAuthManager: User" << username << "registered successfully as" << type;
            delete newUser;
        } else {
            result["success"] = false;
            result["error"] = "Failed to save new user data.";
            delete newUser; // 保存失败，删除创建的对象
        }
    }
    qDeleteAll(users); // 清理从 loadAllUsers 获取的 User 对象
    return result;
}

bool ServerAuthManager::changePassword(const QString &username, const QString &oldPwd, const QString &newPwd) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if (!users.contains(username)) {
        qWarning() << "ServerAuthManager: Attempt to change password for non-existent user" << username;
        qDeleteAll(users);
        return false;
    }
    if (newPwd.length() < 6) {
        qWarning() << "ServerAuthManager: New password too short for user" << username;
        qDeleteAll(users);
        return false;
    }

    User* user = users[username];
    if (!user->verifyPassword(oldPwd)) {
        qWarning() << "ServerAuthManager: Old password incorrect for user" << username;
        qDeleteAll(users);
        return false;
    }

    user->changePassword(User::hashPassword(newPwd)); // User::changePassword 应该只更新内存中的密码
        // User::hashPassword 在这里再次哈希
    bool success = FileManager::saveUser(user); // FileManager::saveUser 会保存这个修改后的 user
    if (success) {
        qInfo() << "ServerAuthManager: Password changed successfully for user" << username;
    } else {
        qWarning() << "ServerAuthManager: Failed to save password change for user" << username;
    }
    qDeleteAll(users);
    return success;
}

bool ServerAuthManager::recharge(const QString& username, double amount) {
    if (amount <= 0) {
        qWarning() << "ServerAuthManager: Recharge amount must be positive for user" << username;
        return false;
    }
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if (!users.contains(username)) {
        qWarning() << "ServerAuthManager: Cannot recharge, user" << username << "not found.";
        qDeleteAll(users);
        return false;
    }

    User* user = users[username];
    user->updateBalance(amount);
    bool success = FileManager::saveUser(user);
    if (success) {
        qInfo() << "ServerAuthManager: User" << username << "recharged by" << amount << ". New balance:" << user->getBalance();
    } else {
        qWarning() << "ServerAuthManager: Failed to save recharge for user" << username;
        // Potentially roll back in-memory balance update if save failed, though less critical here
        user->updateBalance(-amount); // Rollback in-memory change
    }
    qDeleteAll(users);
    return success;
}

double ServerAuthManager::getBalance(const QString& username) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    double balance = 0.0;
    if (users.contains(username)) {
        balance = users[username]->getBalance();
    } else {
        qWarning() << "ServerAuthManager: Requested balance for non-existent user" << username;
    }
    qDeleteAll(users);
    return balance;
}

bool ServerAuthManager::deductBalance(const QString& username, double amount) {
    if (amount <= 0) {
        qWarning() << "ServerAuthManager: Deduct amount must be positive for user" << username;
        return false; // Or handle amount == 0 as success no-op
    }
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if (!users.contains(username)) {
        qWarning() << "ServerAuthManager: Cannot deduct balance, user" << username << "not found.";
        qDeleteAll(users);
        return false;
    }

    User* user = users[username];
    if (user->getBalance() < amount) {
        qWarning() << "ServerAuthManager: Insufficient balance for user" << username << "to deduct" << amount;
        qDeleteAll(users);
        return false;
    }
    user->updateBalance(-amount); // User::updateBalance handles +/-
    bool success = FileManager::saveUser(user);
    if (success) {
        qInfo() << "ServerAuthManager: Deducted" << amount << "from user" << username << ". New balance:" << user->getBalance();
    } else {
        qWarning() << "ServerAuthManager: Failed to save balance deduction for user" << username;
        user->updateBalance(amount); // Rollback in-memory change
    }
    qDeleteAll(users);
    return success;
}

bool ServerAuthManager::addBalance(const QString& username, double amount) {
    if (amount <= 0) {
        qWarning() << "ServerAuthManager: Add amount must be positive for user" << username;
        return false;
    }
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if (!users.contains(username)) {
        qWarning() << "ServerAuthManager: Cannot add balance, user" << username << "not found.";
        qDeleteAll(users);
        return false;
    }

    User* user = users[username];
    user->updateBalance(amount);
    bool success = FileManager::saveUser(user);
    if (success) {
        qInfo() << "ServerAuthManager: Added" << amount << "to user" << username << ". New balance:" << user->getBalance();
    } else {
        qWarning() << "ServerAuthManager: Failed to save balance addition for user" << username;
        user->updateBalance(-amount); // Rollback in-memory change
    }
    qDeleteAll(users);
    return success;
}

QString ServerAuthManager::getUserType(const QString& username) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    QString type = "";
    if (users.contains(username)) {
        type = users[username]->getUserType();
    } else {
        qWarning() << "ServerAuthManager: Requested type for non-existent user" << username;
    }
    qDeleteAll(users);
    return type;
}
