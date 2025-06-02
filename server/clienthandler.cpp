#include "clienthandler.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>
#include <QDebug>

// Include server-side manager headers
#include "serverauthmanager.h"
#include "serverproductmanager.h"
#include "servershoppingcartmanager.h"
#include "serverordermanager.h"
#include "product.h" // For serializing product data
#include "order.h"   // For serializing order data

ClientHandler::ClientHandler(qintptr socketDescriptor,
                             ServerAuthManager* authMgr, ServerProductManager* prodMgr,
                             ServerShoppingCartManager* cartMgr, ServerOrderManager* orderMgr,
                             QObject *parent)
    : QObject(parent), m_socketDescriptor(socketDescriptor),
    m_authManager_s(authMgr), m_productManager_s(prodMgr),
    m_shoppingCartManager_s(cartMgr), m_orderManager_s(orderMgr) {
}

ClientHandler::~ClientHandler() {
    if (m_socket) {
        m_socket->disconnectFromHost();
        // delete m_socket; // m_socket will be deleted due to parent=this or explicitly
    }
    qDebug() << "ClientHandler for descriptor" << m_socketDescriptor << "destroyed.";
}

void ClientHandler::process() {
    m_socket = new QTcpSocket(this); // Parent to ClientHandler for auto-cleanup
    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qCritical() << "ClientHandler: Failed to set socket descriptor" << m_socketDescriptor << ":" << m_socket->errorString();
        emit finished();
        return;
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onSocketDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, &ClientHandler::onSocketError);

    qInfo() << "ClientHandler: Processing connection" << m_socketDescriptor << "on thread" << QThread::currentThreadId();
}

void ClientHandler::onReadyRead() {
    m_buffer.append(m_socket->readAll());

    // Basic message framing: try to parse JSON. A robust solution uses length prefixes or delimiters.
    // This simplified version assumes one JSON object per write or clearly separated.
    // For multiple JSONs in one read: loop and try to parse.
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(m_buffer, &error);

    while (error.error == QJsonParseError::NoError && doc.isObject()) {
        qDebug() << "ClientHandler (" << m_socketDescriptor << ") RX:" << doc.toJson(QJsonDocument::Compact);
        processMessage(doc.object());

        // Remove processed message from buffer
        // This is tricky without knowing message boundaries.
        // If we assume one message per transmission and server handles it:
        m_buffer.clear(); // Simplistic: clear after one message.
            // A better way: find end of JSON, slice buffer.

        // Try to parse next message if buffer still has data (not robustly handled here)
        if (m_buffer.isEmpty()) break;
        doc = QJsonDocument::fromJson(m_buffer, &error);
    }

    if (error.error != QJsonParseError::NoError && !m_buffer.isEmpty()) {
        // If not a complete JSON object yet, error.error might be UnterminatedObject
        // We keep the partial data in buffer.
        if (error.offset >= m_buffer.size() -1 && // 错误发生在缓冲区末尾附近
            (error.error == QJsonParseError::UnterminatedObject || // 常见的未结束错误
             error.error == QJsonParseError::UnterminatedArray ||
             error.error == QJsonParseError::MissingNameSeparator || // 这些也是可能由不完整数据造成
             error.error == QJsonParseError::IllegalValue // 有时末尾的非法值也因不完整
             // 你可以根据你的Qt版本文档查找 QJsonParseError::ParseError 的可用值
             )) {
            qDebug() << "ClientHandler (" << m_socketDescriptor << ") Incomplete JSON in buffer, waiting for more data. Error:" << error.errorString() << "at offset" << error.offset;
        } else {
            // 确定的解析错误，不是因为数据不完整
            qWarning() << "ClientHandler (" << m_socketDescriptor << ") JSON parse error:" << error.errorString() << ". Buffer:" << m_buffer.constData();
            QJsonObject errResponse;
            errResponse["status"] = "error";
            errResponse["message"] = "Invalid JSON request: " + error.errorString();
            errResponse["response_to_action"] = "unknown_malformed";
            sendResponse(errResponse);
            m_buffer.clear(); // Discard malformed message
        }
    }
}


void ClientHandler::onSocketDisconnected() {
    qInfo() << "ClientHandler: Socket disconnected" << m_socketDescriptor;
    emit disconnectedFromClient(this);
    emit finished(); // Signal that this handler's work is done
}

void ClientHandler::onSocketError(QAbstractSocket::SocketError socketError) {
    qWarning() << "ClientHandler: Socket error on" << m_socketDescriptor << ":" << m_socket->errorString() << "(Code:" << socketError << ")";
    // emit disconnectedFromClient(this); // Server will handle removal
    // emit finished(); // Depending on error, may need to terminate
}

void ClientHandler::sendResponse(const QJsonObject& response) {
    if (m_socket && m_socket->isOpen() && m_socket->isWritable()) {
        QByteArray data = QJsonDocument(response).toJson(QJsonDocument::Compact);
        m_socket->write(data);
        m_socket->flush();
        qDebug() << "ClientHandler (" << m_socketDescriptor << ") TX:" << data;
    } else {
        qWarning() << "ClientHandler (" << m_socketDescriptor << ") Cannot send response, socket not writable.";
    }
}

void ClientHandler::processMessage(const QJsonObject& request) {
    QString action = request["action"].toString();
    QJsonObject payload = request["payload"].toObject();
    QJsonObject responsePayload; // Data part of the response

    QString status = "success"; // Default status
    QString message = "";       // Error message if any

    // --- Authentication ---
    if (action == "login") responsePayload = handleLogin(payload);
    else if (action == "register") responsePayload = handleRegister(payload);
    else if (action == "changePassword") responsePayload = handleChangePassword(payload);
    else if (action == "recharge") responsePayload = handleRecharge(payload);
    else if (action == "getBalance") responsePayload = handleGetBalance(payload);
    // --- Products ---
    else if (action == "getProducts") responsePayload = handleGetProducts(payload);
    else if (action == "searchProducts") responsePayload = handleSearchProducts(payload);
    else if (action == "addProduct") responsePayload = handleAddProduct(payload);
    else if (action == "updateProduct") responsePayload = handleUpdateProduct(payload);
    else if (action == "setCategoryDiscount") responsePayload = handleSetCategoryDiscount(payload);
    // --- Shopping Cart ---
    else if (action == "getCart") responsePayload = handleGetCart(payload);
    else if (action == "addToCart") responsePayload = handleAddToCart(payload);
    else if (action == "removeFromCart") responsePayload = handleRemoveFromCart(payload);
    else if (action == "updateCartQuantity") responsePayload = handleUpdateCartQuantity(payload);
    // --- Orders ---
    else if (action == "prepareOrder") responsePayload = handlePrepareOrder(payload);
    else if (action == "payOrder") responsePayload = handlePayOrder(payload);
    else if (action == "getOrders") responsePayload = handleGetOrders(payload);
    else {
        status = "error";
        message = "Unknown action: " + action;
    }

    // If handler methods set their own status/message, respect it
    if (responsePayload.contains("status")) status = responsePayload["status"].toString();
    if (responsePayload.contains("message") && !responsePayload["message"].toString().isEmpty()) {
        message = responsePayload["message"].toString();
    }


    QJsonObject finalResponse;
    finalResponse["response_to_action"] = action; // Echo action for client to route
    finalResponse["status"] = status;
    if (status == "success") {
        finalResponse["data"] = responsePayload.value("data"); // Assuming handlers put data under "data" key
    } else {
        finalResponse["message"] = message.isEmpty() ? responsePayload.value("message").toString("Unknown error") : message;
    }
    sendResponse(finalResponse);
}


// --- Individual Handler Implementations ---
QJsonObject ClientHandler::handleLogin(const QJsonObject& payload) {
    QString username = payload["username"].toString();
    QString password = payload["password"].toString();
    QVariantMap result = m_authManager_s->verifyLogin(username, password);
    QJsonObject response;
    if (result["success"].toBool()) {
        m_loggedInUsername = username; // Store for this session
        response["status"] = "success";
        response["data"] = QJsonObject::fromVariantMap(result["userData"].toMap());
    } else {
        response["status"] = "error";
        response["message"] = result["error"].toString();
    }
    return response;
}

QJsonObject ClientHandler::handleRegister(const QJsonObject& payload) {
    QVariantMap result = m_authManager_s->registerUser(
        payload["username"].toString(),
        payload["password"].toString(),
        payload["type"].toString(),
        0.0 // New users start with 0 balance
        );
    QJsonObject response;
    if (result["success"].toBool()) {
        response["status"] = "success";
        // Optionally auto-login: m_loggedInUsername = payload["username"].toString();
        // response["data"] = ... if sending user data back
    } else {
        response["status"] = "error";
        response["message"] = result["error"].toString();
    }
    return response;
}
// ... Implement ALL other handle<Action> methods similarly ...
// They call the corresponding Server<ManagerName> method and format the response.

QJsonObject ClientHandler::handleChangePassword(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) { // Security: rely on server's logged-in state
        response["status"] = "error";
        response["message"] = "Not logged in or session invalid.";
        return response;
    }
    bool success = m_authManager_s->changePassword(
        m_loggedInUsername, // Use server-side username
        payload["oldPwd"].toString(),
        payload["newPwd"].toString()
        );
    response["status"] = success ? "success" : "error";
    if (!success) response["message"] = "Password change failed (e.g., wrong old password).";
    return response;
}

QJsonObject ClientHandler::handleRecharge(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    double amount = payload["amount"].toDouble();
    bool success = m_authManager_s->recharge(m_loggedInUsername, amount);
    if (success) {
        response["status"] = "success";
        QJsonObject data;
        data["newBalance"] = m_authManager_s->getBalance(m_loggedInUsername);
        response["data"] = data;
    } else {
        response["status"] = "error";
        response["message"] = "Recharge failed.";
    }
    return response;
}

QJsonObject ClientHandler::handleGetBalance(const QJsonObject &payload) {
    QJsonObject response;
    QString usernameToQuery = m_loggedInUsername; // Default to current user

    // Optional: Allow admin to query any user, but requires role check
    // if (payload.contains("username_query") && m_authManager_s->isAdmin(m_loggedInUsername)) {
    //     usernameToQuery = payload["username_query"].toString();
    // }

    if (usernameToQuery.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in or user not specified.";
        return response;
    }
    QJsonObject data;
    data["balance"] = m_authManager_s->getBalance(usernameToQuery);
    response["status"] = "success";
    response["data"] = data;
    return response;
}


QJsonObject ClientHandler::handleGetProducts(const QJsonObject& payload) {
    Q_UNUSED(payload);
    QList<Product*> products = m_productManager_s->getAllProducts();
    QJsonArray productsArray;
    for (Product* p : products) {
        QJsonObject productJson;
        productJson["name"] = p->getName();
        productJson["description"] = p->getDescription();
        productJson["basePrice"] = p->getBasePrice();
        productJson["price"] = p->getPrice(); // Current price with discount
        productJson["stock"] = p->getStock();
        productJson["category"] = p->getCategory();
        productJson["imagePath"] = p->getImagePath();
        productJson["merchantUsername"] = p->getMerchantUsername();
        productJson["discount"] = p->getDiscount();
        productsArray.append(productJson);
    }
    QJsonObject response;
    response["status"] = "success";
    QJsonObject data;
    data["products"] = productsArray;
    response["data"] = data;
    return response;
}

QJsonObject ClientHandler::handleSearchProducts(const QJsonObject &payload) {
    double minPriceVal = -1.0;
    if (payload.contains("minPrice") && payload["minPrice"].isDouble()) {
        minPriceVal = payload["minPrice"].toDouble();
    }

    double maxPriceVal = -1.0;
    if (payload.contains("maxPrice") && payload["maxPrice"].isDouble()) {
        maxPriceVal = payload["maxPrice"].toDouble();
    }
    QList<Product*> products = m_productManager_s->searchProducts(
        payload["keyword"].toString(),
        payload["searchType"].toInt(),
        minPriceVal,
        maxPriceVal
        );
    QJsonArray productsArray;
    for (Product* p : products) {
        QJsonObject productJson;
        // ... (same serialization as getProducts) ...
        productJson["name"] = p->getName();
        productJson["description"] = p->getDescription();
        productJson["basePrice"] = p->getBasePrice();
        productJson["price"] = p->getPrice();
        productJson["stock"] = p->getStock();
        productJson["category"] = p->getCategory();
        productJson["imagePath"] = p->getImagePath();
        productJson["merchantUsername"] = p->getMerchantUsername();
        productJson["discount"] = p->getDiscount();
        productsArray.append(productJson);
    }
    QJsonObject response;
    response["status"] = "success";
    QJsonObject data;
    data["products"] = productsArray;
    response["data"] = data;
    return response;
}

QJsonObject ClientHandler::handleAddProduct(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty() || m_authManager_s->getUserType(m_loggedInUsername) != "Merchant") {
        response["status"] = "error";
        response["message"] = "Permission denied. Only merchants can add products.";
        return response;
    }
    bool success = m_productManager_s->addProduct(
        payload["name"].toString(), payload["description"].toString(),
        payload["price"].toDouble(), payload["stock"].toInt(),
        payload["category"].toString(), m_loggedInUsername, // Use logged-in merchant's username
        payload["imagePath"].toString()
        );
    response["status"] = success ? "success" : "error";
    if(!success) response["message"] = "Failed to add product (e.g., duplicate name, invalid data).";
    return response;
}

QJsonObject ClientHandler::handleUpdateProduct(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty() || m_authManager_s->getUserType(m_loggedInUsername) != "Merchant") {
        response["status"] = "error";
        response["message"] = "Permission denied.";
        return response;
    }
    // Product identified by its original name and the logged-in merchant
    bool success = m_productManager_s->updateProduct(
        payload["originalName"].toString(), m_loggedInUsername,
        payload["name"].toString(), payload["description"].toString(),
        payload["price"].toDouble(), payload["stock"].toInt(),
        payload["imagePath"].toString()
        );
    response["status"] = success ? "success" : "error";
    if(!success) response["message"] = "Failed to update product (e.g., product not found).";
    return response;
}

QJsonObject ClientHandler::handleSetCategoryDiscount(const QJsonObject &payload) {
    QJsonObject response;
    // Typically an admin/platform function, but requirements imply merchant might do it.
    // Let's assume any merchant can for now, as per project scope.
    if (m_loggedInUsername.isEmpty() || m_authManager_s->getUserType(m_loggedInUsername) != "Merchant") {
        response["status"] = "error";
        response["message"] = "Permission denied.";
        return response;
    }
    m_productManager_s->setCategoryDiscount(
        payload["category"].toString(),
        payload["discount"].toDouble() // Expect 0.0 to 1.0
        );
    response["status"] = "success"; // This action usually succeeds unless input is invalid
    return response;
}

QJsonObject ClientHandler::handleGetCart(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    QVariantList cartItems = m_shoppingCartManager_s->getCartItems(m_loggedInUsername);
    QJsonObject data;
    data["items"] = QJsonArray::fromVariantList(cartItems);
    response["status"] = "success";
    response["data"] = data;
    return response;
}

QJsonObject ClientHandler::handleAddToCart(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    bool success = m_shoppingCartManager_s->addItem(
        m_loggedInUsername,
        payload["productName"].toString(),
        payload["merchantUsername"].toString(),
        payload["quantity"].toInt()
        );
    response["status"] = success ? "success" : "error";
    if(!success) response["message"] = "Failed to add to cart (e.g. stock issue, product not found).";
    return response;
}

QJsonObject ClientHandler::handleRemoveFromCart(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    bool success = m_shoppingCartManager_s->removeItem(
        m_loggedInUsername,
        payload["productName"].toString(),
        payload["merchantUsername"].toString()
        );
    response["status"] = success ? "success" : "error";
    if(!success) response["message"] = "Failed to remove from cart.";
    return response;
}

QJsonObject ClientHandler::handleUpdateCartQuantity(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    bool success = m_shoppingCartManager_s->updateQuantity(
        m_loggedInUsername,
        payload["productName"].toString(),
        payload["merchantUsername"].toString(),
        payload["newQuantity"].toInt()
        );
    response["status"] = success ? "success" : "error";
    if(!success) response["message"] = "Failed to update cart quantity (e.g. stock issue).";
    return response;
}


QJsonObject ClientHandler::handlePrepareOrder(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    QJsonArray itemsDataJson = payload["itemsData"].toArray();
    QVariantList itemsData;
    for(const QJsonValue& val : itemsDataJson){
        itemsData.append(val.toObject().toVariantMap());
    }

    QVariantMap orderResult = m_orderManager_s->prepareOrder(m_loggedInUsername, itemsData);

    if (orderResult.value("success", false).toBool()) {
        response["status"] = "success";
        response["data"] = QJsonObject::fromVariantMap(orderResult["orderData"].toMap());
    } else {
        response["status"] = "error";
        response["message"] = orderResult.value("message", "Failed to prepare order.").toString();
    }
    return response;
}

QJsonObject ClientHandler::handlePayOrder(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    QString orderId = payload["orderId"].toString();
    QVariantMap paymentResult = m_orderManager_s->payOrder(m_loggedInUsername, orderId);

    if (paymentResult.value("success", false).toBool()) {
        response["status"] = "success";
        QJsonObject data;
        data["newBalance"] = paymentResult.value("newBalance", m_authManager_s->getBalance(m_loggedInUsername)).toDouble();
        response["data"] = data;
        // Server-side OrderManager should also clear the cart after successful payment.
    } else {
        response["status"] = "error";
        response["message"] = paymentResult.value("message", "Payment failed.").toString();
    }
    return response;
}

QJsonObject ClientHandler::handleGetOrders(const QJsonObject &payload) {
    QJsonObject response;
    if (m_loggedInUsername.isEmpty()) {
        response["status"] = "error";
        response["message"] = "Not logged in.";
        return response;
    }
    QVariantList orders = m_orderManager_s->getOrdersForUser(m_loggedInUsername);
    QJsonObject data;
    data["orders"] = QJsonArray::fromVariantList(orders);
    response["status"] = "success";
    response["data"] = data;
    return response;
}
