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

    user->changePassword(User::hashPassword(newPwd));
    FileManager::saveUser(user);
    return true;
}

bool AuthManager::verifyPassword(const QString &username,
                                 const QString &pwd) {
    QMap<QString, User*> users = FileManager::loadAllUsers();
    return users[username] && users[username]->verifyPassword(pwd);
}
