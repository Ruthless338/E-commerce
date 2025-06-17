#include "shoppingcart.h"
#include "authmanager.h" // For sendRequestAndWait
#include "globalstate.h" // To get current username
#include <QJsonArray>
#include <QDebug>

// 假设 globalStateInstance 在 main.cpp 中定义和初始化
extern GlobalState* globalStateInstance;

ShoppingCart::ShoppingCart(QObject *parent) : QObject(parent) {
    // 当用户登录或应用启动时，尝试从服务器加载购物车
    // 这里简化为构造时加载，实际应用中应在用户登录成功后加载
    if (globalStateInstance && !globalStateInstance->username().isEmpty()) {
        loadCartFromServer();
    }
    // 可以连接 GlobalState 的登录成功信号来触发 loadCartFromServer
}

void ShoppingCart::loadCartFromServer() {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        m_cartItems.clear();
        // emit itemsChanged(); // 如果有
        emit totalPriceChanged();
        return;
    }

    QJsonObject request;
    request["action"] = "getCart";
    // payload P 通常为空，服务器根据当前登录用户获取购物车
    // QJsonObject payload; payload["username"] = globalStateInstance->username();
    // request["payload"] = payload;


    QJsonObject response = AuthManager::sendRequestAndWait(request);
    m_cartItems.clear(); // 清空本地旧数据

    if (response["status"].toString() == "success") {
        QJsonArray itemsArray = response["data"].toObject()["items"].toArray();
        for (const QJsonValue &val : itemsArray) {
            m_cartItems.append(val.toObject().toVariantMap());
        }
    } else {
        qWarning() << "ShoppingCart: Failed to load cart -" << response["message"].toString();
    }
    // emit itemsChanged();
    emit totalPriceChanged();
}

bool ShoppingCart::addItem(const QString& productName, const QString& merchantUsername, int quantity) {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        emit cartUpdated(false, "User not logged in.");
        return false;
    }
    if (quantity <= 0) {
        emit cartUpdated(false, "Quantity must be positive.");
        return false;
    }

    QJsonObject request;
    request["action"] = "addToCart";
    QJsonObject payload;
    // payload["username"] = globalStateInstance->username(); // 服务器从会话获取
    payload["productName"] = productName;
    payload["merchantUsername"] = merchantUsername;
    payload["quantity"] = quantity;
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        loadCartFromServer(); // 操作成功后，重新从服务器加载整个购物车
        emit cartUpdated(true, "Item added to cart.");
        return true;
    } else {
        qWarning() << "ShoppingCart: Failed to add item -" << response["message"].toString();
        emit cartUpdated(false, response["message"].toString());
        return false;
    }
}

bool ShoppingCart::removeItem(const QString& productName, const QString& merchantUsername) {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        emit cartUpdated(false, "User not logged in.");
        return false;
    }

    QJsonObject request;
    request["action"] = "removeFromCart";
    QJsonObject payload;
    // payload["username"] = globalStateInstance->username();
    payload["productName"] = productName;
    payload["merchantUsername"] = merchantUsername;
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        loadCartFromServer();
        emit cartUpdated(true, "Item removed from cart.");
        return true;
    } else {
        qWarning() << "ShoppingCart: Failed to remove item -" << response["message"].toString();
        emit cartUpdated(false, response["message"].toString());
        return false;
    }
}

bool ShoppingCart::updateQuantity(const QString& productName, const QString& merchantUsername, int newQuantity) {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        emit cartUpdated(false, "User not logged in.");
        return false;
    }
    if (newQuantity < 0) { // 通常不允许负数，0表示移除
        emit cartUpdated(false, "Quantity cannot be negative.");
        return false;
    }
    if (newQuantity == 0) { // 数量为0等同于删除
        return removeItem(productName, merchantUsername);
    }


    QJsonObject request;
    request["action"] = "updateCartQuantity";
    QJsonObject payload;
    // payload["username"] = globalStateInstance->username();
    payload["productName"] = productName;
    payload["merchantUsername"] = merchantUsername;
    payload["newQuantity"] = newQuantity;
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        loadCartFromServer();
        emit cartUpdated(true, "Cart quantity updated.");
        return true;
    } else {
        qWarning() << "ShoppingCart: Failed to update quantity -" << response["message"].toString();
        emit cartUpdated(false, response["message"].toString());
        return false;
    }
}

void ShoppingCart::clearCart() {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        // 本地也清一下，即使未登录
        if (!m_cartItems.isEmpty()) {
            m_cartItems.clear();
            // emit itemsChanged();
            emit totalPriceChanged();
        }
        emit cartUpdated(false, "User not logged in.");
        return;
    }

    QJsonObject request;
    request["action"] = "clearCart"; // 服务器需要实现此 action
    // QJsonObject payload; payload["username"] = globalStateInstance->username();
    // request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        m_cartItems.clear(); // 服务器成功后，清空本地
        // emit itemsChanged();
        emit totalPriceChanged();
        emit cartUpdated(true, "Cart cleared.");
    } else {
        // 即使服务器失败，也可能选择清空本地（取决于业务逻辑）
        // m_cartItems.clear();
        // emit itemsChanged();
        // emit totalPriceChanged();
        qWarning() << "ShoppingCart: Failed to clear cart on server -" << response["message"].toString();
        emit cartUpdated(false, "Failed to clear cart on server: " + response["message"].toString());
    }
}

QVariantList ShoppingCart::getItems() const {
    QVariantList list;
    for (const QVariantMap &item : m_cartItems) {
        list.append(item);
    }
    return list;
}

double ShoppingCart::getTotalPrice() const {
    double total = 0.0;
    for (const QVariantMap &item : m_cartItems) {
        // 服务器返回的购物车项中应包含计算好的 itemTotalPrice
        // 或者包含 price 和 quantity，客户端计算
        // 我们假设服务器返回的item中包含 "itemTotalPrice"
        // 或者包含 "price" (单价) 和 "quantity"
        double itemPrice = item.value("price", 0.0).toDouble();
        int quantity = item.value("quantity", 0).toInt();
        total += itemPrice * quantity; // 重新计算以防服务器没给 itemTotalPrice
    }
    return total;
}

QVariantList ShoppingCart::getCartItemsForOrder() const {
    // 此方法专为创建订单准备数据，格式可能与 getItems() 略有不同，
    // 取决于 OrderManager::prepareOrder 服务器接口期望的格式。
    // 通常，它需要每个商品的 name, merchantUsername, quantity, price (购买时的单价)
    QVariantList orderItems;
    for (const QVariantMap& cartItem : m_cartItems) {
        QVariantMap orderItem;
        orderItem["productName"] = cartItem.value("name"); // 或 "productName"
        orderItem["merchantUsername"] = cartItem.value("merchantUsername");
        orderItem["quantity"] = cartItem.value("quantity");
        orderItem["price"] = cartItem.value("price"); // 购买时的单价
        orderItems.append(orderItem);
    }
    return orderItems;
}
