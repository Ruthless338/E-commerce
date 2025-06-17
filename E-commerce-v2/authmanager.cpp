#include "authmanager.h"
#include "filemanager.h"

bool AuthManager::changePassword(const QString &username,
                                 const QString &oldPwd,
                                 const QString &newPwd) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if (!users.contains(username)) {
        qDebug() << "修改密码失败：无此用户";
        return false;
    }

    User* user = users[username];
    if (!user->verifyPassword(oldPwd)) {
        qDebug() << "修改密码失败：密码错误";
        return false;
    }
    qDebug() << "Client: change password successfully." << '\n';

    user->changePassword(User::hashPassword(newPwd));
    FileManager::saveUser(user);
    return true;
}

bool AuthManager::verifyPassword(const QString &username,
                                 const QString &pwd) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    return users[username] && users[username]->verifyPassword(pwd);
}

bool AuthManager::recharge(const QString& username, double amount) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if(!users.contains(username)) return false;

    users[username]->updateBalance(amount);
    return FileManager::saveUser(users[username]);
}

double AuthManager::getBalance(const QString& username) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    return users.contains(username) ? users[username]->getBalance() : 0.0;
}

bool AuthManager::deductBalance(const QString& username, double amount){
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if(!users.contains(username)) return false;

    if(users[username]->getBalance() < amount) return false;

    users[username]->updateBalance(-amount);
    return FileManager::saveUser(users[username]);
}

bool AuthManager::addBalance(const QString& username, double amount) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    if(!users.contains(username)) return false;

    users[username]->updateBalance(amount);
    return FileManager::saveUser(users[username]);
}


