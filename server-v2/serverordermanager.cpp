#include "serverordermanager.h"
#include "serverproductmanager.h"
#include "serverauthmanager.h"
#include "servershoppingcartmanager.h"
#include "filemanager.h" // For saving/loading orders
#include "order.h"       // For Order class
#include "product.h"     // For Product class
#include <QTimer>
#include <QDebug>
#include <QUuid> // For generating order IDs

ServerOrderManager::ServerOrderManager(ServerProductManager* productMgr,
                                       ServerAuthManager* authMgr,
                                       ServerShoppingCartManager* cartMgr,
                                       QObject *parent)
    : QObject(parent), m_productManager(productMgr), m_authManager(authMgr), m_shoppingCartManager(cartMgr) {

    loadOrdersFromFile(); // Load existing orders

    m_timeoutTimer = new QTimer(this);
    connect(m_timeoutTimer, &QTimer::timeout, this, &ServerOrderManager::checkTimeoutOrders);
    m_timeoutTimer->start(60 * 1000); // Check for timeouts every minute
}

ServerOrderManager::~ServerOrderManager() {
    saveOrdersToFile(); // Save any final changes
    qDeleteAll(m_allOrders);
    m_allOrders.clear();
}

void ServerOrderManager::loadOrdersFromFile() {
    qDeleteAll(m_allOrders);
    m_allOrders.clear();
    // FileManager::loadOrders needs all products to resolve Product* in items
    m_allOrders = FileManager::loadOrders(m_productManager->getAllProducts());
    qInfo() << "ServerOrderManager: Loaded" << m_allOrders.count() << "orders from file.";

    // After loading, re-check any pending orders that might have timed out while server was offline
    // This is complex because createTime is based on when it was loaded.
    // A robust solution would store expirationTime directly in the order JSON.
    // For now, we'll rely on the periodic checkTimeoutOrders.
}

bool ServerOrderManager::saveOrdersToFile() {
    bool success = FileManager::saveOrders(m_allOrders);
    if (success) {
        qInfo() << "ServerOrderManager: Orders saved to file.";
    } else {
        qWarning() << "ServerOrderManager: Failed to save orders to file.";
    }
    return success;
}


QVariantMap ServerOrderManager::orderToVariantMap(Order* order, bool includeItemsDetails) {
    if (!order) return QVariantMap();
    QVariantMap map;
    map["orderId"] = order->getOrderId(); // Order class needs getOrderId()
    map["consumerUsername"] = order->getConsumerUsername();
    map["creationTime"] = order->getCreateTimer().toString(Qt::ISODate);
    map["status"] = order->getStatusString(); // Order class needs getStatusString()
    map["total"] = order->calculateTotal();
    map["remainingSeconds"] = order->getRemainingSeconds();

    if (includeItemsDetails) {
        map["items"] = order->getQmlItems(); // getQmlItems is good for this
    }
    return map;
}

QVariantList ServerOrderManager::ordersToVariantList(const QList<Order*>& orders) {
    QVariantList list;
    for (Order* order : orders) {
        list.append(orderToVariantMap(order));
    }
    return list;
}

QVariantMap ServerOrderManager::prepareOrder(const QString& consumerUsername, const QVariantList& itemsData) {
    QVariantMap result;
    QMap<Product*, int> orderItemsMap;
    QList<Product*> productsToRollbackFreeze; // For rollback

    if (itemsData.isEmpty()) {
        result["success"] = false;
        result["message"] = "Order must contain items.";
        return result;
    }

    for (const QVariant& itemVar : itemsData) {
        QVariantMap itemMap = itemVar.toMap();
        QString productName = itemMap["productName"].toString();
        QString merchantUsername = itemMap["merchantUsername"].toString();
        int quantity = itemMap["quantity"].toInt();

        if (quantity <= 0) {
            result["success"] = false;
            result["message"] = "Item quantity must be positive for " + productName;
            // Rollback any freezes done so far
            for(Product* p : productsToRollbackFreeze) m_productManager->releaseFrozenStock(p, orderItemsMap.value(p));
            return result;
        }

        Product* product = m_productManager->findProductByNameAndMerchant(productName, merchantUsername);
        if (!product) {
            result["success"] = false;
            result["message"] = "Product not found: " + productName + " by " + merchantUsername;
            for(Product* p : productsToRollbackFreeze) m_productManager->releaseFrozenStock(p, orderItemsMap.value(p));
            return result;
        }

        if (!m_productManager->freezeStock(product, quantity)) { // Try to freeze stock
            result["success"] = false;
            result["message"] = "Insufficient stock or failed to freeze for " + productName;
            for(Product* p : productsToRollbackFreeze) m_productManager->releaseFrozenStock(p, orderItemsMap.value(p));
            return result;
        }
        productsToRollbackFreeze.append(product);
        orderItemsMap[product] = quantity;
    }

    // If all items processed and stock frozen successfully
    Order* newOrder = new Order(consumerUsername, orderItemsMap); // Order constructor
    QString orderId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newOrder->setOrderId(orderId); // Order class needs setOrderId(QString)

    m_allOrders.append(newOrder);
    if (!saveOrdersToFile()) {
        // Critical failure, try to unfreeze stock
        for(auto it = orderItemsMap.constBegin(); it != orderItemsMap.constEnd(); ++it) {
            m_productManager->releaseFrozenStock(it.key(), it.value());
        }
        m_allOrders.removeOne(newOrder); // Remove from memory
        delete newOrder;
        result["success"] = false;
        result["message"] = "Failed to save new order after freezing stock.";
        return result;
    }

    result["success"] = true;
    result["orderData"] = orderToVariantMap(newOrder);
    qInfo() << "ServerOrderManager: Order" << orderId << "prepared for" << consumerUsername;
    return result;
}

QVariantMap ServerOrderManager::payOrder(const QString& consumerUsername, const QString& orderId) {
    QVariantMap result;
    Order* orderToPay = nullptr;
    for (Order* o : m_allOrders) {
        if (o->getOrderId() == orderId && o->getConsumerUsername() == consumerUsername) {
            orderToPay = o;
            break;
        }
    }

    if (!orderToPay) {
        result["success"] = false;
        result["message"] = "Order not found or does not belong to user.";
        return result;
    }

    if (orderToPay->getStatus() != Order::Pending) {
        result["success"] = false;
        result["message"] = "Order is not in Pending state. Current status: " + orderToPay->getStatusString();
        return result;
    }

    if (orderToPay->getRemainingSeconds() <= 0) {
        orderToPay->setStatus(Order::Cancelled); // Mark as cancelled due to timeout
        // Release stock (already handled by checkTimeoutOrders, but good to ensure)
        for(auto it = orderToPay->getItems().constBegin(); it != orderToPay->getItems().constEnd(); ++it) {
            m_productManager->releaseFrozenStock(it.key(), it.value());
        }
        saveOrdersToFile();
        result["success"] = false;
        result["message"] = "Order has timed out.";
        return result;
    }

    double total = orderToPay->calculateTotal();
    if (!m_authManager->deductBalance(consumerUsername, total)) {
        result["success"] = false;
        result["message"] = "Payment failed: Insufficient balance.";
        // No stock change needed yet, as it was only frozen
        return result;
    }

    // Attempt to credit merchants
    QMap<Product*, int> items = orderToPay->getItems();
    QMap<QString, double> merchantPayoutsAttempted; // Track who we tried to pay

    bool allMerchantPaymentsSucceeded = true;
    for (auto it = items.constBegin(); it != items.constEnd(); ++it) {
        Product* product = it.key();
        int quantity = it.value();
        double amountForMerchant = product->getPrice() * quantity; // Use current price
        QString merchant = product->getMerchantUsername();
        merchantPayoutsAttempted[merchant] += amountForMerchant; // Accumulate per merchant

        if (!m_authManager->addBalance(merchant, amountForMerchant)) {
            qWarning() << "ServerOrderManager: Critical! Failed to credit merchant" << merchant << "for order" << orderId;
            allMerchantPaymentsSucceeded = false;
            break; // Stop processing further merchant payments
        }
    }

    if (!allMerchantPaymentsSucceeded) {
        // Rollback: Refund consumer, and debit any merchants already paid
        m_authManager->addBalance(consumerUsername, total); // Refund consumer
        for(auto const& [merchant, amount] : merchantPayoutsAttempted.toStdMap()) {
            // Check if this merchant was actually paid before failure.
            // This part is tricky without knowing exactly which addBalance failed.
            // A more robust system would have a transaction log or 2-phase commit.
            // Simplification: Try to deduct what was attempted.
            m_authManager->deductBalance(merchant, amount);
            qWarning() << "ServerOrderManager: Rolled back payment to merchant" << merchant << "for order" << orderId;
        }
        // Order remains Pending (or could be moved to a FailedPayment state)
        // Stock remains frozen.
        result["success"] = false;
        result["message"] = "Payment failed: Could not complete fund transfer to merchant(s). Your balance has been restored.";
        return result;
    }

    // All payments successful, now confirm stock deduction
    for (auto it = items.constBegin(); it != items.constEnd(); ++it) {
        if(!m_productManager->confirmStockDeduction(it.key(), it.value())) {
            // This is highly unlikely if freezeStock worked and no external stock changes.
            // But if it happens, it's a major issue. System is in inconsistent state.
            // Manual intervention might be needed.
            qCritical() << "ServerOrderManager: CRITICAL! Failed to confirm stock deduction for" << it.key()->getName() << "on order" << orderId << "after payment!";
            // At this point, money is transferred. Try to mark order as problematic.
            orderToPay->setStatus(Order::Paid); // Mark as paid, but log the stock issue
            saveOrdersToFile();
            result["success"] = true; // Money part was ok
            result["message"] = "Payment successful, but a stock confirmation issue occurred. Please contact support.";
            result["newBalance"] = m_authManager->getBalance(consumerUsername);
            m_shoppingCartManager->clearCart(consumerUsername); // Clear cart after successful payment
            return result;
        }
    }

    orderToPay->setStatus(Order::Paid);
    saveOrdersToFile();
    m_shoppingCartManager->clearCart(consumerUsername); // Clear cart after successful payment

    result["success"] = true;
    result["newBalance"] = m_authManager->getBalance(consumerUsername);
    qInfo() << "ServerOrderManager: Order" << orderId << "paid successfully by" << consumerUsername;
    return result;
}

QVariantList ServerOrderManager::getOrdersForUser(const QString& consumerUsername) {
    QList<Order*> userOrders;
    for (Order* o : m_allOrders) {
        if (o->getConsumerUsername() == consumerUsername) {
            userOrders.append(o);
        }
    }
    // Sort by creation time, newest first
    std::sort(userOrders.begin(), userOrders.end(), [](Order* a, Order* b){
        return a->getCreateTimer() > b->getCreateTimer();
    });
    return ordersToVariantList(userOrders);
}

void ServerOrderManager::checkTimeoutOrders() {
    bool changed = false;
    QDateTime currentTime = QDateTime::currentDateTime();
    //qDebug() << "ServerOrderManager: Checking for timed out orders at" << currentTime.toString();

    QMutableListIterator<Order*> i(m_allOrders);
    while (i.hasNext()) {
        Order* order = i.next();
        if (order->getStatus() == Order::Pending && order->getRemainingSeconds() <= 0) {
            qInfo() << "ServerOrderManager: Order" << order->getOrderId() << "for" << order->getConsumerUsername() << "timed out.";
            order->setStatus(Order::Cancelled);
            // Release frozen stock
            for(auto it = order->getItems().constBegin(); it != order->getItems().constEnd(); ++it) {
                m_productManager->releaseFrozenStock(it.key(), it.value());
            }
            changed = true;
        }
    }

    if (changed) {
        saveOrdersToFile();
    }
}
