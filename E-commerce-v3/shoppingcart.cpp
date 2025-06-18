#include "shoppingcart.h"
#include "authmanager.h"
#include "globalstate.h"
#include <QJsonArray>
#include <QDebug>

extern GlobalState* globalStateInstance;

ShoppingCart::ShoppingCart(QObject *parent) : QObject(parent) {
    if (globalStateInstance && !globalStateInstance->username().isEmpty()) {
        loadCartFromServer();
    }
}

void ShoppingCart::loadCartFromServer() {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        m_cartItems.clear();
        emit totalPriceChanged();
        return;
    }

    QJsonObject request;
    request["action"] = "getCart";


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
    if (newQuantity < 0) {
        emit cartUpdated(false, "Quantity cannot be negative.");
        return false;
    }
    if (newQuantity == 0) {
        return removeItem(productName, merchantUsername);
    }


    QJsonObject request;
    request["action"] = "updateCartQuantity";
    QJsonObject payload;
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
        if (!m_cartItems.isEmpty()) {
            m_cartItems.clear();
            emit totalPriceChanged();
        }
        emit cartUpdated(false, "User not logged in.");
        return;
    }

    QJsonObject request;
    request["action"] = "clearCart"; // 服务器需要实现此 action

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        m_cartItems.clear(); // 服务器成功后，清空本地
        emit totalPriceChanged();
        emit cartUpdated(true, "Cart cleared.");
    } else {
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
        double itemPrice = item.value("price", 0.0).toDouble();
        int quantity = item.value("quantity", 0).toInt();
        total += itemPrice * quantity;
    }
    return total;
}

QVariantList ShoppingCart::getCartItemsForOrder() const {
    QVariantList orderItems;
    for (const QVariantMap& cartItem : m_cartItems) {
        QVariantMap orderItem;
        orderItem["productName"] = cartItem.value("name");
        orderItem["merchantUsername"] = cartItem.value("merchantUsername");
        orderItem["quantity"] = cartItem.value("quantity");
        orderItem["price"] = cartItem.value("price"); // 购买时的单价
        orderItems.append(orderItem);
    }
    return orderItems;
}
