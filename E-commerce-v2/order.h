// order.h
#ifndef ORDER_H
#define ORDER_H
#include <QDateTime>
#include "product.h"

class Order {
public:
    enum Status { Pending, Paid, Cancelled };

    Order(const QMap<Product*, int>& items)
        : items(items), createTime(QDateTime::currentDateTime()) {}

    Order(const QString& consumerUsername,
          const QList<QPair<QString, QString>>& productIdentifiers,
          const QList<Product*>& allProducts);

    // 计算订单总金额
    double calculateTotal() const;


    Status getStatus() const { return status; }
    void setStatus(Status s) { status = s; }
    QMap<Product*, int> getItems() const { return items; }
    QDateTime getCreateTimer() const { return createTime; }
    Q_INVOKABLE int getRemainingSeconds() const ;
    QString getConsumerUsername() const { return consumerUsername; }
    QList<QPair<QString, QString>> getProductIdentifiers() const { return productIdentifiers; }

    Q_INVOKABLE QList<QPair<Product*, int>> getItemPairs() const ;

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
