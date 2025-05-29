#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <QList>
#include <QObject>
#include <QVariant>
#include "order.h"

class OrderManager : public QObject {
    Q_OBJECT
public:
    explicit OrderManager(QObject *parent = nullptr) : QObject(parent) {}
    
    Q_INVOKABLE QVariant createOrder(const QString& username, const QVariantList& items);
    Q_INVOKABLE bool payOrder(Order* order, const QString& consumerUsername);
    Q_INVOKABLE QList<Order*> getOrders() const { return orders; }

private slots:
    void checkTimeoutOrders();

private:
    QList<Order*> orders;
    Order* createOrderInternal(const QMap<Product*, int>& items);
};

#endif // ORDERMANAGER_H
