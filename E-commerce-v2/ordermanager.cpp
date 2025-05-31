#include "ordermanager.h"
#include "authmanager.h"
#include "productmodel.h"
#include "globalstate.h"
#include "filemanager.h"
#include <QTimer>
#include <QDebug>

OrderManager::OrderManager(QObject *parent) : QObject(parent) {
    loadOrders();
    QTimer *timeoutTimer = new QTimer(this);
    connect(timeoutTimer, &QTimer::timeout, this, &OrderManager::checkTimeoutOrders);
    timeoutTimer->start(5*60*1000);
}

void OrderManager::loadOrders() {
    // 清理现有订单
    qDeleteAll(orders);
    orders.clear();
    
    // 从文件加载订单
    QList<Product*> allProducts = FileManager::loadProducts();
    orders = FileManager::loadOrders(allProducts);
}

void OrderManager::saveOrders() {
    FileManager::saveOrders(orders);
}

Order* OrderManager::prepareOrder(const QString& username, const QVariantList& itemsData) {
    QMap<Product*, int> orderItemsMap;

    for (const QVariant& itemVar : itemsData) {
        QVariantMap itemMap = itemVar.toMap();
        QString productName = itemMap["name"].toString();
        QString merchantUsername = itemMap["merchantUsername"].toString(); // Expecting this from ShoppingCart::getCartItems
        int quantity = itemMap["quantity"].toInt();

        if (quantity <= 0) {
            qDebug() << "OrderManager::prepareOrder: Invalid quantity" << quantity << "for" << productName;
            return nullptr;
        }

        Product* product = ProductModel::instance()->findProductByNameAndMerchant(productName, merchantUsername);

        if (!product) {
            qDebug() << "OrderManager::prepareOrder: Product not found:" << productName << "by" << merchantUsername;
            return nullptr;
        }
        if (product->getStock() < quantity) {
            qDebug() << "OrderManager::prepareOrder: Stock insufficient for" << productName << ". Required:" << quantity << "Available:" << product->getStock();
            return nullptr;
        }
        orderItemsMap[product] = quantity;
    }

    if (orderItemsMap.isEmpty()) {
        qDebug() << "OrderManager::prepareOrder: No valid items to order.";
        return nullptr;
    }

    Order* order = new Order(username, orderItemsMap); // QObject parent is nullptr by default
    // order object will be parented by QML engine if returned to QML and assigned to a property,
    // or managed by OrderManager's list. Consider explicit parenting if necessary.

    if (!order->freezeStock()) {
        qDebug() << "OrderManager::prepareOrder: Failed to freeze stock for order.";
        // Order::freezeStock should handle internal rollback of partial freezes.
        delete order; // Clean up order object if freeze fails
        return nullptr;
    }

    this->orders.append(order);
    saveOrders(); // Save the new pending order

    qDebug() << "Order prepared for" << username << "with" << orderItemsMap.count() << "item types. Order status:" << order->getStatusString();
    return order;
}

bool OrderManager::payOrder(Order* order, const QString& consumerUsername) {
    if(order->getStatus() != Order::Pending) {
        qDebug() << "订单状态不是待支付状态";
        return false;
    }

    if (order->getRemainingSeconds() <= 0) {
        qDebug() << "订单超时, 解冻库存";
        order->releaseStock();
        ProductModel::instance()->saveProducts();
        saveOrders();
        return false;
    }

    double total = order->calculateTotal();
    if(!AuthManager::deductBalance(consumerUsername, total)) {
        qDebug() << "扣款失败，余额不足";
        order->releaseStock();
        return false;
    }

    QMap<Product*, int> items = order->getItems();
    bool allSuccess = true;
    QMap<QString, double> merchantPayouts;
    for(auto it = items.begin(); it != items.end(); it++) {
        QString merchant = it.key()->getMerchantUsername();
        double itemTotal = it.key()->getPrice() * it.value();
        if (!AuthManager::addBalance(merchant, itemTotal)) {
            qDebug() << "商家" << merchant << "加款失败";
            allSuccess = false;
            break;
        }
        merchantPayouts[merchant] += itemTotal;
    }

    if (!allSuccess) {
        // 如果有任何一个商家加款失败，回滚所有操作
        AuthManager::addBalance(consumerUsername, total);
        for(auto const& [merchant, amount] : merchantPayouts.toStdMap()) {
            AuthManager::deductBalance(merchant, amount);
        }
        order->releaseStock();
        return false;
    }

    // 解冻库存，并真正意义上扣除库存
    order->confirmStock();

    // 向前端发送数据（主要是库存）更改的信号
    for(auto it = items.begin(); it != items.end(); it++) {
        ProductModel::instance() -> productStockNotify(it.key());
    }

    ProductModel::instance() -> saveProducts();
    saveOrders(); // 保存订单状态
    if(!FileManager::clearUserShoppingCart(consumerUsername)) {
        qDebug() << "清空购物车失败，用户名为：" << consumerUsername << '\n';
    }
    return true;
}

void OrderManager::checkTimeoutOrders() {
    QDateTime now = QDateTime::currentDateTime();
    bool ordersChanged = false;
    QList<Order*> ordersToRemove;

    for(int i = 0; i < orders.size(); ++i) {
        Order* order = orders.at(i);
        if(order->getStatus() == Order::Pending && order->getRemainingSeconds() <= 0) {
            qDebug() << "Order for" << order->getConsumerUsername() << "订单超时，解冻库存";
            order->releaseStock();

            QMap<Product*, int> items = order->getItems();
            for(auto it = items.begin(); it != items.end(); it++) {
                ProductModel::instance()->productStockNotify(it.key());
            }
            ProductModel::instance()->saveProducts();

            ordersChanged = true;
        }
    }

    if (ordersChanged) {
        saveOrders();
    }

}

QVariant OrderManager::createOrder(const QString& username, const QVariantList& items) {
    QMap<Product*, int> orderItems;
    double totalPrice = 0;
    
    // 检查库存和计算总价
    for (const QVariant& item : items) {
        QVariantMap itemMap = item.toMap();
        QString productName = itemMap["name"].toString();
        int quantity = itemMap["quantity"].toInt();
        
        Product* product = ProductModel::instance()->findProduct(productName);
        if (!product) {
            qDebug() << "商品不存在：" << productName;
            return QVariant();
        }
        
        if (product->getStock() < quantity) {
            qDebug() << "商品库存不足：" << productName;
            return QVariant();
        }
        
        orderItems[product] = quantity;
        totalPrice += product->getPrice() * quantity;
    }
    
    // 检查用户余额
    if (AuthManager::getBalance(username) < totalPrice) {
        qDebug() << "用户余额不足";
        return QVariant();
    }
    
    // 创建订单
    Order* order = createOrderInternal(orderItems);
    if (!order) {
        qDebug() << "创建订单失败";
        return QVariant();
    }
    
    // 冻结库存
    if (!order->freezeStock()) {
        qDebug() << "冻结库存失败";
        delete order;
        return QVariant();
    }
    
    // 支付订单
    if (!payOrder(order, username)) {
        qDebug() << "支付订单失败";
        order->releaseStock();
        delete order;
        return QVariant();
    }

    QVariantMap orderData;
    orderData["status"] = order->getStatus();
    orderData["total"] = totalPrice;
    return orderData;
}

Order* OrderManager::createOrderInternal(const QMap<Product*, int>& items) {
    if (items.isEmpty()) return nullptr;

    Order* order = new Order(items);
    orders.append(order);

    QTimer::singleShot(300000, this, &OrderManager::checkTimeoutOrders);
    return order;
}
