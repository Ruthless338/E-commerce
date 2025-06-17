#include "ordermanager.h"
#include "authmanager.h" // For sendRequestAndWait
#include "globalstate.h" // For username and updating balance
// #include "productmodel.h" // 如果支付成功后需要直接通知ProductModel
#include <QJsonArray>
#include <QDebug>

// 假设 globalStateInstance 和 productModelInstance (如果需要) 在 main.cpp 定义和初始化
extern GlobalState* globalStateInstance;
// extern ProductModel* productModelInstance;

OrderManager::OrderManager(QObject *parent) : QObject(parent), m_shoppingCart(nullptr) {
    // 通常在 main.cpp 中，创建 OrderManager 后会调用 setCartInstance
}

void OrderManager::setCartInstance(ShoppingCart* cart) {
    m_shoppingCart = cart;
}

QVariantMap OrderManager::prepareOrderFromCart() {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        emit orderPrepared(false, QVariantMap(), "User not logged in.");
        return QVariantMap{{"success", false}, {"message", "User not logged in."}};
    }
    if (!m_shoppingCart) {
        emit orderPrepared(false, QVariantMap(), "Shopping cart not available.");
        return QVariantMap{{"success", false}, {"message", "Shopping cart not available."}};
    }

    QVariantList cartItemsData = m_shoppingCart->getCartItemsForOrder();
    if (cartItemsData.isEmpty()) {
        emit orderPrepared(false, QVariantMap(), "Shopping cart is empty.");
        return QVariantMap{{"success", false}, {"message", "Shopping cart is empty."}};
    }

    QJsonObject request;
    request["action"] = "prepareOrder";
    QJsonObject payload;
    // payload["username"] = globalStateInstance->username(); // 服务器从会话获取
    QJsonArray itemsJsonArray = QJsonArray::fromVariantList(cartItemsData);
    payload["itemsData"] = itemsJsonArray; // 服务器期望的购物车项数据格式
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        QVariantMap orderData = response["data"].toObject().toVariantMap();
        // orderData 应包含 "orderId", "items", "totalAmount", "status" (e.g., "PendingPayment") 等
        orderData["success"] = true; // 添加一个 success 标志给QML
        emit orderPrepared(true, orderData, "Order prepared successfully.");
        return orderData;
    } else {
        QString errorMsg = response["message"].toString("Failed to prepare order.");
        emit orderPrepared(false, QVariantMap(), errorMsg);
        return QVariantMap{{"success", false}, {"message", errorMsg}};
    }
}

bool OrderManager::payOrder(const QString& orderId) {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        emit orderPaid(false, orderId, "User not logged in.");
        return false;
    }
    if (orderId.isEmpty()) {
        emit orderPaid(false, orderId, "Invalid order ID.");
        return false;
    }

    QJsonObject request;
    request["action"] = "payOrder";
    QJsonObject payload;
    // payload["username"] = globalStateInstance->username();
    payload["orderId"] = orderId;
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        QJsonObject data = response["data"].toObject();
        double newBalance;
        if (data.contains("newBalance") && data["newBalance"].isDouble()) {
            newBalance = data["newBalance"].toDouble();
        } else {
            qWarning() << "OrderManager::payOrder - Server response did not contain a valid 'newBalance'. Using current global balance as fallback (or refetching).";
            // 可以选择使用当前的余额作为后备，或者更好的做法是，如果服务器没返回，就再请求一次余额
            // newBalance = globalStateInstance->balance();
            // 或者触发一次 AuthManager::getBalance(globalStateInstance->username())
            // 这里我们先用一个明确的警告，并依赖服务器必须返回 newBalance
            // 如果服务器保证会返回，但可能不是double，可以做更复杂的类型检查
            newBalance = globalStateInstance->balance(); // 暂时使用旧余额，理想情况是服务器必须返回
        }        globalStateInstance->setBalance(newBalance); // 更新全局余额

        // 支付成功后，购物车通常会被清空 (服务器端处理)
        if (m_shoppingCart) {
            m_shoppingCart->clearCart(); // 让客户端也清空并刷新（或服务器清空后，下次getCart返回空）
        }
        emit stockPossiblyChanged(); // 通知UI（间接通知ProductModel）库存可能变了
        emit orderPaid(true, orderId, "Order paid successfully. New balance: " + QString::number(newBalance));
        return true;
    } else {
        QString errorMsg = response["message"].toString("Payment failed.");
        emit orderPaid(false, orderId, errorMsg);
        emit paymentError(errorMsg); // 可以有一个更具体的支付错误信号
        return false;
    }
}

QVariantList OrderManager::getUserOrders() {
    if (!globalStateInstance || globalStateInstance->username().isEmpty()) {
        emit ordersLoaded(false, QVariantList(), "User not logged in.");
        return QVariantList();
    }

    QJsonObject request;
    request["action"] = "getOrders"; // 获取当前用户的所有订单
    // QJsonObject payload; payload["username"] = globalStateInstance->username();
    // request["payload"] = payload;


    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        QJsonArray ordersArray = response["data"].toObject()["orders"].toArray();
        QVariantList ordersData;
        for (const QJsonValue& val : ordersArray) {
            ordersData.append(val.toObject().toVariantMap());
        }
        // m_userOrders = ordersData; // (可选) 本地缓存
        emit ordersLoaded(true, ordersData, "Orders loaded successfully.");
        return ordersData;
    } else {
        QString errorMsg = response["message"].toString("Failed to load orders.");
        emit ordersLoaded(false, QVariantList(), errorMsg);
        return QVariantList();
    }
}
