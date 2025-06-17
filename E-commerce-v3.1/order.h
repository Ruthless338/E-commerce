// order.h
#ifndef ORDER_H
#define ORDER_H
#include <QDateTime>
#include <QObject>
#include <QVariant>
#include "product.h"

class Order :public QObject {
    Q_OBJECT
public:
    enum Status { Pending, Paid, Cancelled };
    Q_ENUM(Status)

    Order(const QMap<Product*, int>& items, QObject* parent = nullptr)
        : items(items), createTime(QDateTime::currentDateTime()), QObject(parent) {}

    Order(const QString& consumerUsername, const QMap<Product*, int>& items, QObject* parent = nullptr)
        : items(items),
        consumerUsername(consumerUsername),
        status(Pending),
        createTime(QDateTime::currentDateTime()),
        QObject(parent) {}

    Order(const QString& consumerUsername, const QMap<Product*, int>& items, Order::Status status, QObject* parent = nullptr)
        : items(items),
        consumerUsername(consumerUsername),
        status(status),
        createTime(QDateTime::currentDateTime()),
        QObject(parent) {}

    Order(const QString& consumerUsername,
          const QList<QPair<QString, QString>>& productIdentifiers,
          const QList<Product*>& allProducts,
          QObject* parent = nullptr);

    Order(const Order& other)
        : consumerUsername(other.consumerUsername),
        status(other.status),
        createTime(other.createTime) {}

    // 计算订单总金额
    Q_INVOKABLE double calculateTotal() const;


    Status getStatus() const { return status; }
    void setStatus(Status s) { status = s; }
    QMap<Product*, int> getItems() const { return items; }
    QDateTime getCreateTimer() const { return createTime; }
    Q_INVOKABLE int getRemainingSeconds() const ;
    QString getConsumerUsername() const { return consumerUsername; }
    QList<QPair<QString, QString>> getProductIdentifiers() const { return productIdentifiers; }

    Q_INVOKABLE QList<QPair<Product*, int>> getItemPairs() const;
    Q_INVOKABLE QString getStatusString() const;
    Q_INVOKABLE QList<QVariant> getQmlItems() const;


    void setCreateTimeForLoadedOrder(const QDateTime& time) { createTime = time; }

    void confirmStock();
    void releaseStock();
    bool freezeStock();


private:
    QMap<Product*, int> items; // 商品指针 -> 数量
    Status status = Pending;
    QDateTime createTime;
    QString consumerUsername;
    QList<QPair<QString, QString>> productIdentifiers;
};
#endif
