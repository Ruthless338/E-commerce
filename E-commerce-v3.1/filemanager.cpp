// filemanager.cpp (客户端)
#include "filemanager.h"
#include "networkclient.h"
#include "user.h"
#include "product.h"
#include "book.h"
#include "clothing.h"
#include "food.h"
#include "order.h"
#include "shoppingcart.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

// --- 文件名常量 ---
const QString FileManager::USERS_FILE = "D:/Qt_projects/E-commerce/E-commerce-v3.1/data/users.json";
const QString FileManager::PRODUCTS_FILE = "D:/Qt_projects/E-commerce/E-commerce-v3.1/data/products.json";
const QString FileManager::ORDERS_FILE = "D:/Qt_projects/E-commerce/E-commerce-v3.1/data/order.json";
const QString FileManager::SHOPPING_CARTS_FILE = "D:/Qt_projects/E-commerce/E-commerce-v3.1/data/shoppingCart.json";


QJsonObject FileManager::sendRequestAndWaitForFileManager(const QJsonObject& requestData, int timeoutMs) {
    if (!NetworkClient::instance()->isConnected()) {
        qWarning() << "FileManager: Not connected to server for action" << requestData.value("action").toString();
        return QJsonObject{{"status", "error"}, {"message", "Not connected"}};
    }
    QJsonObject responseJson;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QMetaObject::Connection disconn = QObject::connect(NetworkClient::instance(), &NetworkClient::disconnected, &loop, [&]() {
        qWarning() << "FileManager (DEBUG): NetworkClient DISCONNECTED during wait loop.";
        loop.quit();
    });
    QMetaObject::Connection conn = QObject::connect(NetworkClient::instance(), &NetworkClient::responseReceived,
                                                    [&](const QJsonObject& res) {
                                                        if (res.value("response_to_action").toString() == requestData.value("action").toString()) {
                                                            responseJson = res;
                                                            if(timer.isActive()) timer.stop();
                                                            loop.quit();
                                                        }
                                                    });
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        qWarning() << "FileManager: Request timeout for action" << requestData.value("action").toString();
        responseJson = QJsonObject{{"status", "error"}, {"message", "File operation timed out"}};
        loop.quit();
    });
    NetworkClient::instance()->sendRequest(requestData);
    timer.start(timeoutMs+5000);
    loop.exec();
    QObject::disconnect(conn);
    return responseJson;
}

QString FileManager::remoteReadFile(const QString& absoluteFilePath) {
    QJsonObject request;
    request["action"] = "readFile";
    QJsonObject payload;
    payload["filePath"] = absoluteFilePath;
    request["payload"] = payload;
    QJsonObject response = sendRequestAndWaitForFileManager(request);
    if (response["status"].toString() == "success") {
        return response["data"].toObject()["content"].toString();
    }
    qWarning() << "FileManager: remoteReadFile failed for" << absoluteFilePath << ":" << response["message"].toString();
    return QString();
}

bool FileManager::remoteWriteFile(const QString& absoluteFilePath, const QString& content) {
    QJsonObject request;
    request["action"] = "writeFile";
    QJsonObject payload;
    payload["filePath"] = absoluteFilePath;
    payload["content"] = content;
    request["payload"] = payload;
    QJsonObject response = sendRequestAndWaitForFileManager(request);
    if (response["status"].toString() == "success") {
        return true;
    }
    qWarning() << "FileManager: remoteWriteFile failed for" << absoluteFilePath << ":" << response["message"].toString();
    return false;
}
User* FileManager::userFromJson(const QJsonObject& jsonObj) {
    User *user;
    if(jsonObj["type"].toString() == "Consumer") {
        User* user = new Consumer(
            jsonObj["username"].toString(),
            jsonObj["password"].toString(),
            jsonObj["balance"].toDouble()
            );
    } else {
        User* user = new Merchant(
            jsonObj["username"].toString(),
            jsonObj["password"].toString(),
            jsonObj["balance"].toDouble()
            );
    }
    return user;
}
QJsonObject FileManager::userToJson(const User* user) {
    if (!user) return QJsonObject();
    QJsonObject obj;
    obj["username"] = user->getUsername();
    obj["password"] = user->getPassword(); // 注意：保存密码时应是哈希后的
    obj["type"] = user->getUserType();
    obj["balance"] = user->getBalance();
    return obj;
}
Product* FileManager::productFromJson(const QJsonObject& jsonObj) {
    QString category = jsonObj["category"].toString();
    Product* product = nullptr;
    if (category == "Book") {
        product = new Book(
            jsonObj["name"].toString(),
            jsonObj["description"].toString(),
            jsonObj["price"].toDouble(),
            jsonObj["stock"].toInt(),
            jsonObj["merchantUsername"].toString(),
            jsonObj["imagePath"].toString()
            );
    } else if (category == "Clothing") {
        product = new Clothing(
            jsonObj["name"].toString(),
            jsonObj["description"].toString(),
            jsonObj["price"].toDouble(),
            jsonObj["stock"].toInt(),
            jsonObj["merchantUsername"].toString(),
            jsonObj["imagePath"].toString()
            );
    } else if (category == "Food") {
        product = new Food(
            jsonObj["name"].toString(),
            jsonObj["description"].toString(),
            jsonObj["price"].toDouble(),
            jsonObj["stock"].toInt(),
            jsonObj["merchantUsername"].toString(),
            jsonObj["imagePath"].toString()
            );
    } else {
        qDebug() << "无此分类: " << category << '\n';
    }
    return product;
}
QJsonObject FileManager::productToJson(const Product* product) {
    if (!product) return QJsonObject();
    QJsonObject obj;
    obj["name"] = product->getName();
    obj["description"] = product->getDescription();
    obj["price"] = product->getBasePrice();
    obj["stock"] = product->getStock();
    obj["category"] = product->getCategory();
    obj["merchantUsername"] = product->getMerchantUsername();
    obj["imagePath"] = product->getImagePath();
    return obj;
}
Order* FileManager::orderFromJson(const QJsonObject& jsonObj, const QList<Product*>& allProducts) {
    QString consumerUsername = jsonObj["consumerUsername"].toString();
    Order::Status status;
    QString statusStr = jsonObj["status"].toString();
    if(statusStr == "Paid") {
        status = Order::Paid;
    } else if(statusStr == "Cancelled") {
        status = Order::Cancelled;
    } else {
        status = Order::Pending;
    }

    QMap<Product*, int> items;
    QJsonArray itemsArray = jsonObj["items"].toArray();
    for (const QJsonValue& itemVal : itemsArray) {
        QJsonObject itemObj = itemVal.toObject();
        QString productName = itemObj["productName"].toString();
        QString merchantUsername = itemObj["merchantUsername"].toString();
        int quantity = itemObj["quantity"].toInt();
        Product* foundProduct = nullptr;
        for (Product* p : allProducts) {
            if (p->getName() == productName && p->getMerchantUsername() == merchantUsername) {
                foundProduct = p;
                break;
            }
        }
        if (foundProduct) items[foundProduct] = quantity;
    }
    return new Order(consumerUsername, items, status);
}
QJsonObject FileManager::orderToJson(const Order* order) {
    if (!order) return QJsonObject();
    QJsonObject obj;
    obj["consumerUsername"] = order->getConsumerUsername();
    QString statusStr;
    if(order->getStatus() == Order::Paid) {
        statusStr = "Paid";
    } else if(order->getStatus() == Order::Cancelled) {
            statusStr = "Cancelled";
    } else {
        statusStr = "Pending";
    }
    obj["status"] = statusStr;
    QJsonArray itemsArray;
    QMap<Product*, int> orderItems = order->getItems();
    for (auto it = orderItems.begin(); it != orderItems.end(); ++it) {
        QJsonObject itemObj;
        itemObj["productName"] = it.key()->getName();
        itemObj["merchantUsername"] = it.key()->getMerchantUsername();
        itemObj["quantity"] = it.value();
        itemsArray.append(itemObj);
    }
    obj["items"] = itemsArray;
    return obj;
}

ShoppingCart* FileManager::shoppingCartFromJson(const QJsonObject& jsonObj, const QList<Product*>& allProducts) {
    QString consumerUsername = jsonObj["consumerUsername"].toString();

    QMap<Product*, int> items;
    QJsonArray itemsArray = jsonObj["items"].toArray();
    for (const QJsonValue& itemVal : itemsArray) {
        QJsonObject itemObj = itemVal.toObject();
        QString productName = itemObj["productName"].toString();
        QString merchantUsername = itemObj["merchantUsername"].toString();
        int quantity = itemObj["quantity"].toInt();
        Product* foundProduct = nullptr;
        for (Product* p : allProducts) {
            if (p->getName() == productName && p->getMerchantUsername() == merchantUsername) {
                foundProduct = p;
                break;
            }
        }
        if (foundProduct) items[foundProduct] = quantity;
    }
    return new ShoppingCart(consumerUsername, items);
}
QJsonObject FileManager::shoppingCartToJson(const ShoppingCart* shoppingCart) {
    QJsonObject obj;
    obj["consumerUsername"] = shoppingCart->getConsumerUsername();
    QJsonArray itemsArray;
    QMap<Product*, int> items = shoppingCart->getItems();
    for (auto it = items.begin(); it != items.end(); ++it) {
        QJsonObject itemObj;
        itemObj["productName"] = it.key()->getName();
        itemObj["merchantUsername"] = it.key()->getMerchantUsername();
        itemObj["quantity"] = it.value();
        itemsArray.append(itemObj);
    }
    obj["items"] = itemsArray;
    return obj;
}

// --- User Data (实现与之前一致) ---
QList<User*> FileManager::loadAllUsers() {
    QList<User*> users;
    QString content = remoteReadFile(USERS_FILE);
    if (content.isEmpty()) return users;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (doc.isArray()) {
        QJsonArray usersArray = doc.array();
        for (const QJsonValue &value : usersArray) {
            if (value.isObject()) {
                User* user = userFromJson(value.toObject());
                if(user) users.append(user);
            }
        }
    }
    return users;
}
bool FileManager::saveAllUsers(const QList<User*>& users) {
    QJsonArray usersArray;
    for (const User* user : users) {
        usersArray.append(userToJson(user));
    }
    return remoteWriteFile(USERS_FILE, QJsonDocument(usersArray).toJson(QJsonDocument::Compact));
}
bool FileManager::saveUser(const User* userToSave) {
    if (!userToSave) return false;
    QList<User*> users = loadAllUsers();
    bool found = false;
    for (int i = 0; i < users.size(); ++i) {
        if (users[i]->getUsername() == userToSave->getUsername()) {
            delete users[i];
            if(userToSave->getUserType() == "Consumer") {
                users[i] = new Consumer(*userToSave); // Deep copy
            } else if(userToSave->getUserType() == "Merchant") {
                users[i] = new Merchant(*userToSave); // Deep copy
            } else {
                qDebug() << "未知类型: " << userToSave->getUserType() << '\n';
                return false;
            }
            found = true;
            break;
        }
    }
    if (!found) {
        if(userToSave->getUserType() == "Consumer") {
            users.append(new Consumer(*userToSave));
        } else if(userToSave->getUserType() == "Merchant") {
            users.append(new Merchant(*userToSave));
        } else {
            qDebug() << "未知类型: " << userToSave->getUserType() << '\n';
            return false;
        }
    }
    bool success = saveAllUsers(users);
    qDeleteAll(users);
    return success;
}

// --- Product Data (实现与之前一致) ---
QList<Product*> FileManager::loadAllProducts() {
    QList<Product*> products;
    QString content = remoteReadFile(PRODUCTS_FILE);
    if (content.isEmpty()) return products;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (doc.isArray()) {
        QJsonArray productsArray = doc.array();
        for (const QJsonValue &value : productsArray) {
            if (value.isObject()) {
                Product* product = productFromJson(value.toObject());
                if(product) products.append(product);
            }
        }
    }
    return products;
}
bool FileManager::saveAllProducts(const QList<Product*>& products) {
    QJsonArray productsArray;
    for (const Product* product : products) {
        productsArray.append(productToJson(product));
    }
    return remoteWriteFile(PRODUCTS_FILE, QJsonDocument(productsArray).toJson(QJsonDocument::Compact));
}
bool FileManager::saveProduct(const Product* productToSave) {
    if(!productToSave) return false;
    QList<Product*> products = loadAllProducts();
    bool found = false;
    for (int i = 0; i < products.size(); ++i) {
        if (products[i]->getName() == productToSave->getName() &&
            products[i]->getMerchantUsername() == productToSave->getMerchantUsername()) {
            delete products[i];
            if (dynamic_cast<const Book*>(productToSave)) products[i] = new Book(*dynamic_cast<const Book*>(productToSave));
            else if (dynamic_cast<const Clothing*>(productToSave)) products[i] = new Clothing(*dynamic_cast<const Clothing*>(productToSave));
            else if (dynamic_cast<const Food*>(productToSave)) products[i] = new Food(*dynamic_cast<const Food*>(productToSave));
            found = true;
            break;
        }
    }
    if (!found) {
        if (dynamic_cast<const Book*>(productToSave)) products.append(new Book(*dynamic_cast<const Book*>(productToSave)));
        else if (dynamic_cast<const Clothing*>(productToSave)) products.append(new Clothing(*dynamic_cast<const Clothing*>(productToSave)));
        else if (dynamic_cast<const Food*>(productToSave)) products.append(new Food(*dynamic_cast<const Food*>(productToSave)));
    }
    bool success = saveAllProducts(products);
    qDeleteAll(products);
    return success;
}
bool FileManager::deleteProduct(const QString& productName, const QString& merchantUsername) {
    QList<Product*> products = loadAllProducts();
    bool changed = false;
    for (int i = products.size() - 1; i >= 0; --i) {
        if (products[i]->getName() == productName && products[i]->getMerchantUsername() == merchantUsername) {
            delete products.takeAt(i);
            changed = true;
            break;
        }
    }
    bool success = false;
    if (changed) {
        success = saveAllProducts(products);
    } else {
        success = true;
    }
    qDeleteAll(products);
    return success;
}

// --- Order Data (实现与之前一致) ---
QList<Order*> FileManager::loadOrders(const QList<Product*>& allProductsParam) { // Changed param name for clarity
    QList<Order*> orders;
    QString content = remoteReadFile(ORDERS_FILE);
    if (content.isEmpty()) return orders;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (doc.isArray()) {
        QJsonArray ordersArray = doc.array();
        QList<Product*> productsToUse = allProductsParam;
        bool tempProductsLoaded = false;
        if(productsToUse.isEmpty()){
            productsToUse = loadAllProducts();
            tempProductsLoaded = true;
        }

        for (const QJsonValue &value : ordersArray) {
            if (value.isObject()) {
                Order* order = orderFromJson(value.toObject(), productsToUse);
                if(order) orders.append(order);
            }
        }
        if(tempProductsLoaded){
            qDeleteAll(productsToUse); // 清理临时加载的商品
        }
    }
    return orders;
}
bool FileManager::saveAllOrders(const QList<Order*>& orders) {
    QJsonArray ordersArray;
    for (const Order* order : orders) {
        ordersArray.append(orderToJson(order));
    }
    return remoteWriteFile(ORDERS_FILE, QJsonDocument(ordersArray).toJson(QJsonDocument::Compact));
}
bool FileManager::saveOrder(const Order* orderToSave) {
    if(!orderToSave) return false;
    QList<Product*> allProds = loadAllProducts(); // Order (de)serialization needs product list
    QList<Order*> orders = loadOrders(allProds); // Pass products
    bool found = false;
    for (int i = 0; i < orders.size(); ++i) {
        if (orders[i]->getConsumerUsername() == orderToSave->getConsumerUsername()) {
            delete orders[i];
            orders[i] = new Order(*orderToSave);
            found = true;
            break;
        }
    }
    if (!found) {
        orders.append(new Order(*orderToSave));
    }
    bool success = saveAllOrders(orders);
    qDeleteAll(orders);
    qDeleteAll(allProds); // Clean up products loaded for this operation
    return success;
}

QList<ShoppingCart*> FileManager::loadShoppingCarts() {
    QList<ShoppingCart*> shoppingCarts;
    QString content = remoteReadFile(SHOPPING_CARTS_FILE);
    if (content.isEmpty()) return shoppingCarts;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (doc.isArray()) {
        QJsonArray shoppingCartsArray = doc.array();
        QList<Product*> productsToUse = loadAllProducts();

        for (const QJsonValue &value : shoppingCartsArray) {
            if (value.isObject()) {
                ShoppingCart* shoppingCart = shoppingCartFromJson(value.toObject(), productsToUse);
                if(shoppingCart) shoppingCarts.append(shoppingCart);
            }
        }
        qDeleteAll(productsToUse); // 清理临时加载的商品
    }
    return shoppingCarts;
}

bool FileManager::saveShoppingCart(const ShoppingCart* shoppingCart) {
    if(!shoppingCart) return false;
    QList<Product*> allProds = loadAllProducts();
    QList<ShoppingCart*> shoppingCarts = loadShoppingCarts();
    bool found = false;
    for (int i = 0; i < shoppingCarts.size(); ++i) {
        if (shoppingCarts[i]->getConsumerUsername() == shoppingCart->getConsumerUsername()) {
            delete shoppingCarts[i];
            shoppingCarts[i] = new ShoppingCart(*shoppingCart);
            found = true;
            break;
        }
    }
    if (!found) {
        shoppingCarts.append(new ShoppingCart(*shoppingCart));
    }
    bool success = saveAllShoppingCarts(shoppingCarts);
    qDeleteAll(shoppingCarts);
    qDeleteAll(allProds); // Clean up products loaded for this operation
    return success;
}

bool FileManager::saveAllShoppingCarts(QList<ShoppingCart*> shoppingCarts) {
    QJsonArray shoppingCartsArray;
    for (const ShoppingCart* shoppingCart : shoppingCarts) {
        shoppingCartsArray.append(shoppingCartToJson(shoppingCart));
    }
    return remoteWriteFile(SHOPPING_CARTS_FILE, QJsonDocument(shoppingCartsArray).toJson(QJsonDocument::Compact));
}

bool FileManager::clearUserShoppingCart(const QString consumerUsername){
    QList<ShoppingCart*> shoppingCarts = FileManager::loadShoppingCarts();
    for(int i = 0;i < shoppingCarts.size(); i++) {
        if(shoppingCarts[i]->getConsumerUsername() == consumerUsername) {
            delete shoppingCarts[i];
            qDebug() << "已删除购物车： " << consumerUsername << '\n';
            return true;
        }
    }
    return false;
}

