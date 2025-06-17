#ifndef SERVERORDERMANAGER_H
#define SERVERORDERMANAGER_H

#include <QObject>
#include <QList>
#include <QVariantMap>
#include <QDateTime>
#include "order.h"

class Order; // Forward declaration
class ServerProductManager;
class ServerAuthManager;
class ServerShoppingCartManager;
class QTimer;

class ServerOrderManager : public QObject {
    Q_OBJECT
public:
    explicit ServerOrderManager(ServerProductManager* productMgr,
                                ServerAuthManager* authMgr,
                                ServerShoppingCartManager* cartMgr,
                                QObject *parent = nullptr);
    ~ServerOrderManager();

    // Client requests to prepare an order from a list of items
    // itemsData: QVariantList of QVariantMap, each map: {"productName", "merchantUsername", "quantity"}
    // Returns: QVariantMap with {"success": bool, "orderData": QVariantMap, "message": QString}
    // orderData: {"orderId", "items": QVariantList, "total", "status", "remainingSeconds"}
    QVariantMap prepareOrder(const QString& consumerUsername, const QVariantList& itemsData);

    // Client requests to pay for a previously prepared order
    // Returns: QVariantMap with {"success": bool, "newBalance": double, "message": QString}
    QVariantMap payOrder(const QString& consumerUsername, const QString& orderId);

    // Client requests their order history
    QVariantList getOrdersForUser(const QString& consumerUsername);


private slots:
    void checkTimeoutOrders();

private:
    QList<Order*> m_allOrders;
    ServerProductManager* m_productManager;
    ServerAuthManager* m_authManager;
    ServerShoppingCartManager* m_shoppingCartManager;
    QTimer* m_timeoutTimer;

    void loadOrdersFromFile();
    bool saveOrdersToFile();

    // Helper to convert Order* to QVariantMap for client response
    QVariantMap orderToVariantMap(Order* order, bool includeItemsDetails = true);
    // Helper to convert QList<Order*> to QVariantList
    QVariantList ordersToVariantList(const QList<Order*>& orders);
};

#endif // SERVERORDERMANAGER_H
