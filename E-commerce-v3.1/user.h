#ifndef USER_H
#define USER_H
#include <QString>
#include <QObject>
#include <QVariant>
#include <QCryptographicHash>

class FileManager;  //前向声明

class User : public QObject
{
    Q_OBJECT
protected:
    QString username;
    QString password;
    double balance;

public:
    explicit User(QString uname, QString pwd, double bal):
        username(uname),password(pwd),balance(bal){}

    explicit User(const User& other)
        : username(other.username), password(other.password), balance(other.balance) {}

    virtual ~User() = default;

    Q_INVOKABLE virtual QString getUserType() const = 0;

    Q_INVOKABLE QString getUsername() const;
    Q_INVOKABLE double getBalance() const;

    Q_INVOKABLE void updateBalance(double amount);

    Q_INVOKABLE bool verifyPassword(const QString &inPwd) const;
    Q_INVOKABLE static QVariantMap registerUser(
        const QString& username,
        const QString& pwd,
        const QString& type,
        const double& balance
    );
    Q_INVOKABLE static bool verifyLogin(
        const QString& username,
        const QString& password
    );
    Q_INVOKABLE QString getPassword() const { return password; }
    Q_INVOKABLE static QString hashPassword(const QString &password);

    void changePassword(const QString &newPwd);
};

#endif // USER_H
