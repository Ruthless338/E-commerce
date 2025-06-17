#include "user.h"
#include "filemanager.h"

QString User::getUsername() const
{
    return username;
}

double User::getBalance() const
{
    return balance;
}

void User::updateBalance(double amount)
{
    balance += amount;
}

bool User::verifyPassword(const QString &inPwd) const
{
    return password == hashPassword(inPwd);
}

QVariantMap User::registerUser(const QString& username, const QString& pwd, const QString& type, const double& balance)
{
    QList<User*> res1 = FileManager::loadAllUsers();

    QMap<QString, User*> users;
    for(User* u:res1) {
        users[u->getUsername()] = u;
    }
    qDebug() << "注册参数 - 用户名:" << username << "类型:" << type;
    QVariantMap res;
    if (users.contains(username)) {
        res["error"] = "用户名已存在";
        res["success"] = false;
    } else {
        User* newUser = nullptr;
        QString hashedPwd = hashPassword(pwd);
        if (type == "Consumer") newUser = new Consumer(username, hashedPwd, balance);
        else if (type == "Merchant") newUser = new Merchant(username, hashedPwd, balance);
        FileManager::saveUser(newUser);
        res["success"] = true;
    }
    return res;
}

bool User::verifyLogin(const QString& username, const QString& password){
    QList<User*> res = FileManager::loadAllUsers();

    QMap<QString, User*> users;
    for(User* u:res) {
        users[u->getUsername()] = u;
    }
    if (!users.contains(username)) return false;
    QString inputHashedPwd = hashPassword(password);
    return users[username]->getPassword() == inputHashedPwd;
}

QString User::hashPassword(const QString &password) {
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(),
        QCryptographicHash::Sha256
        );
    return QString(hash.toHex());
}
void User::changePassword(const QString &newPwd) {
    this->password = newPwd;
}


