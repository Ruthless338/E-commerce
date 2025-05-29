#include "ordermanager.h"
#include "authmanager.h"
#include "productmodel.h"
#include "globalstate.h"
#include "filemanager.h"
#include <QTimer>
#include <QDebug>

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

bool OrderManager::payOrder(Order* order, const QString& consumerUsername) {
    if(order->getStatus() != Order::Pending) {
        qDebug() << "订单状态不是待支付状态";
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
    for(auto it = items.begin(); it != items.end(); it++) {
        QString merchant = it.key()->getMerchantUsername();
        double itemTotal = it.key()->getPrice() * it.value();
        if (!AuthManager::addBalance(merchant, itemTotal)) {
            qDebug() << "商家" << merchant << "加款失败";
            allSuccess = false;
            break;
        }
    }

    if (!allSuccess) {
        // 如果有任何一个商家加款失败，回滚所有操作
        AuthManager::addBalance(consumerUsername, total);
        order->releaseStock();
        return false;
    }

    order->confirmStock();

    for(auto it = items.begin(); it != items.end(); it++) {
        ProductModel::instance() -> productStockNotify(it.key());
    }

    ProductModel::instance() -> saveProducts();
    saveOrders(); // 保存订单状态
    return true;
}

void OrderManager::checkTimeoutOrders() {
    QDateTime now = QDateTime::currentDateTime();
    bool ordersChanged = false;
    
    for(auto it = orders.begin(); it != orders.end();) {
        if((*it)->getStatus() == Order::Pending &&
            (*it)->getCreateTimer().secsTo(now) > 5 * 60) {
            (*it)->releaseStock();
            delete *it;
            it = orders.erase(it);
            ordersChanged = true;
        } else {
            ++it;
        }
    }
    
    if (ordersChanged) {
        saveOrders(); // 保存订单状态
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
