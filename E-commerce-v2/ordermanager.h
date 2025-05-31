#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <QList>
#include <QObject>
#include <QVariant>
#include "order.h"

class OrderManager : public QObject {
    Q_OBJECT
public:
    explicit OrderManager(QObject *parent = nullptr);
    
    // 创建订单函数，后弃用
    Q_INVOKABLE QVariant createOrder(const QString& username, const QVariantList& items);
    // 生成订单
    Q_INVOKABLE Order* prepareOrder(const QString& username, const QVariantList& itemsData);;
    // 确认下单（支付）
    Q_INVOKABLE bool payOrder(Order* order, const QString& consumerUsername);
    Q_INVOKABLE QList<Order*> getOrders() const { return orders; }

    void loadOrders();
    void saveOrders();

private slots:
    void checkTimeoutOrders();

private:
    QList<Order*> orders;
    Order* createOrderInternal(const QMap<Product*, int>& items);
};

#endif // ORDERMANAGER_H
