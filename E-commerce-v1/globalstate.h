#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <QObject>

class GlobalState : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString userType READ userType WRITE setUserType NOTIFY userTypeChanged)
    Q_PROPERTY(bool isConsumer READ isConsumer WRITE setIsConsumer NOTIFY isConsumerChanged)
    Q_PROPERTY(bool isMerchant READ isMerchant WRITE setIsMerchant NOTIFY isMerchantChanged)

public:
    explicit GlobalState(QObject *parent = nullptr);

    // Getter 方法
    QString username() const;
    QString userType() const;
    bool isConsumer() const;
    bool isMerchant() const;

    // Setter 方法
    void setUsername(const QString &username);
    void setUserType(const QString &userType);
    void setIsConsumer(bool isConsumer);
    void setIsMerchant(bool isMerchant);

signals:
    // 属性变更信号
    void usernameChanged();
    void userTypeChanged();
    void isConsumerChanged();
    void isMerchantChanged();

private:
    // 私有成员变量
    QString m_username;
    QString m_userType;
    bool m_isConsumer = false;
    bool m_isMerchant = false;
};

#endif // GLOBALSTATE_H
