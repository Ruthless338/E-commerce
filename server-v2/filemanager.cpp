#include "filemanager.h"

QMap<QString, User*> FileManager::loadAllUsers()
{
    QMap<QString, User*> users;
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/users.json");
    //处理打开失败
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if(file.error() == QFile::OpenError) {
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            file.write("[]");
            file.close();
        }
        return QMap<QString, User*>();
    }

    QByteArray data = file.readAll();
    file.close();
    //解析JSON数据
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(!doc.isArray())
    {
        qDebug() << "Invalid JSON format.Expected an array.";
        return users;
    }

    QJsonArray jsonArray = doc.array();
    for(const QJsonValue &value : jsonArray)
    {
        if(!value.isObject())
        {
            qDebug() << "Invalid JSON format.Expected an object.";
            continue;
        }
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString();
        QString name = obj["name"].toString();
        QString password = obj["password"].toString();
        double balance = obj["balance"].toDouble();

        User* user = nullptr;
        if(type == "Consumer") user = new Consumer(name, password, balance);
        else if(type == "Merchant") user = new Merchant(name, password, balance);
        else {
            qDebug() << "Unknown user type:" << type;
            continue;
        }
        users[name] = user;
    }
    return users;
}

bool FileManager::userExist(const QString& username){
    QMap<QString, User*> users = FileManager::loadAllUsers();
    return users[username]!=nullptr;
}

QList<Product*> FileManager::loadProducts(){
    QList<Product*> products;
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/products.json");
    if (!file.open(QIODevice::ReadOnly)) return products;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonObject categories = root["categories"].toObject();
    Book::discount = categories["图书"].toDouble(1.0); // 加载失败将返回默认值1.0
    Clothing::discount = categories["服装"].toDouble(1.0);
    Food::discount = categories["食品"].toDouble(1.0);

    QJsonArray productArray = root["products"].toArray();
    for (const QJsonValue& value : productArray) {
        QJsonObject obj = value.toObject();
        QString category = obj["category"].toString();
        QString name = obj["name"].toString();
        QString desc = obj["description"].toString();
        double price = obj["price"].toDouble();
        int stock = obj["stock"].toInt();
        QString merchantUsername = obj["merchantUsername"].toString();
        QString imagePath = obj["imagePath"].toString();
        int frozenStock = obj["frozenStock"].toInt();

        Product* product = nullptr;
        if (category == "图书") product = new Book(name, desc, price, stock, merchantUsername, imagePath);
        else if (category == "服装") product = new Clothing(name, desc, price, stock, merchantUsername, imagePath);
        else if (category == "食品") product = new Food(name, desc, price, stock, merchantUsername, imagePath);
        if (product) {
            products.append(product);
        }
    }
    return products;
}

bool FileManager::saveUser(const User* user)
{
    QMap<QString, User*> existingUsers = loadAllUsers();
    // 删除同名用户
    QString username = user->getUsername();
    if(existingUsers.contains(username)) {
        delete existingUsers[username];
        existingUsers.remove(username);
    }
    User* newUser = nullptr;
    QString type = user->getUserType();
    if(type == "Consumer") {
        newUser = new Consumer(user->getUsername(), user->getPassword(), user->getBalance());
    } else if(type == "Merchant") {
        newUser = new Merchant(user->getUsername(), user->getPassword(), user->getBalance());
    } else {
        qDebug() << "未知用户类型：" << type;
        return false;
    }
    existingUsers.insert(username, newUser);

    QJsonArray jsonArray;
    for (User* u : existingUsers) {
        QJsonObject obj;
        obj["name"] = u->getUsername();
        obj["password"] = u->getPassword();
        obj["balance"] = u->getBalance();
        obj["type"] = u->getUserType();
        jsonArray.append(obj);
    }

    // 写入文件
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/users.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法写入 users.json" << file.errorString();
        return false;
    }
    file.write(QJsonDocument(jsonArray).toJson());
    file.close();
    return true;
}

bool FileManager::saveProducts(const QList<Product*>& products){
    QJsonObject root;
    QJsonObject categories;
    categories["图书"] = Book::discount;
    categories["服装"] = Clothing::discount;
    categories["食品"] = Food::discount;
    root["categories"] = categories;

    QJsonArray productArray;
    for(Product* product : products) {
        QJsonObject obj;
        obj["name"] = product->getName();
        obj["description"] = product->getDescription();
        obj["price"] = product->getBasePrice();
        obj["stock"] = product->getStock();
        obj["category"] = product->getCategory();
        obj["imagePath"] = product->getImagePath();
        obj["merchantUsername"] = product->getMerchantUsername();
        obj["frozenStock"] = product->getFrozenStock();
        productArray.append(obj);
    }
    root["products"] = productArray;

    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/products.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法写入 products.json";
        return false;
    }
    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

bool FileManager::saveShoppingCarts(const QVariantMap& allCarts) {
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/shoppingCart.json");
    if (!file.open(QIODevice::WriteOnly)) return false;

    QJsonObject root;
    for (auto userIt = allCarts.begin(); userIt != allCarts.end(); ++userIt) {
        QString username = userIt.key();
        QVariantMap userCart = userIt.value().toMap();
        QJsonObject userCartJson;
        for (auto itemIt = userCart.begin(); itemIt != userCart.end(); ++itemIt) {
            userCartJson[itemIt.key()] = itemIt.value().toInt();
        }
        root[username] = userCartJson;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    return true;
}

QVariantMap FileManager::loadAllShoppingCarts() {
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/shoppingCart.json");
    QVariantMap allCarts;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        for (auto userIt = root.begin(); userIt != root.end(); ++userIt) {
            QString username = userIt.key();
            QJsonObject userCartJson = userIt.value().toObject();
            QVariantMap userCart;
            for (auto itemIt = userCartJson.begin(); itemIt != userCartJson.end(); ++itemIt) {
                userCart[itemIt.key()] = itemIt.value().toInt();
            }
            allCarts[username] = userCart;
        }
    }
    return allCarts;
}

QString orderStatusToString(Order::Status status) {
    switch(status) {
    case Order::Pending:
        return "Pending";
    case Order::Paid:
        return "Paid";
    case Order::Cancelled:
        return "Cancelled";
    default:
        return "Unknown";
    }
}

Order::Status stringToOrderStatus(const QString& statusStr) {
    if(statusStr == "Pending") return Order::Pending;
    if(statusStr == "Paid") return Order::Paid;
    if(statusStr == "Cancelled") return Order::Cancelled;
    return Order::Pending;
}

// 保存订单数据
bool FileManager::saveOrders(const QList<Order*>& orders) {
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/order.json");
    if (!file.open(QIODevice::WriteOnly)) return false;

    QJsonArray orderArray;
    for (Order* order : orders) {
        QJsonObject orderObj;
        orderObj["consumerUsername"] = order->getConsumerUsername();
        orderObj["creationTime"]  = order->getCreateTimer().toString(Qt::ISODate);
        orderObj["status"] = orderStatusToString(order->getStatus());
        orderObj["orderId"] = order->getOrderId();

        QJsonArray itemsArray;
        const QMap<Product*, int>& orderItemsMap = order->getItems();
        for (auto it = orderItemsMap.begin(); it != orderItemsMap.end(); ++it) {
            Product* product = it.key();
            int quantity = it.value();
            QJsonObject itemObj;
            itemObj["productName"] = product->getName();
            itemObj["merchantUsername"] = product->getMerchantUsername();
            itemObj["quantity"] = quantity;
            itemsArray.append(itemObj);
        }
        orderObj["items"] = itemsArray;
        orderArray.append(orderObj);
    }

    QJsonDocument doc(orderArray);
    file.write(doc.toJson());
    file.close();
    return true;
}

// 加载订单数据
QList<Order*> FileManager::loadOrders(const QList<Product*>& allProducts) {
    QList<Order*> orders;
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v2/data/order.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray orderArray = doc.array();
        for (const QJsonValue& orderVal : orderArray) {
            QJsonObject orderObj = orderVal.toObject();
            QString consumer = orderObj["consumer"].toString();
            QDateTime creationTime = QDateTime::fromString(orderObj["creationTime"].toString(), Qt::ISODate);
            Order::Status status = stringToOrderStatus(orderObj["status"].toString());
            QJsonArray itemsArray = orderObj["items"].toArray();
            QString orderId = orderObj["orderId"].toString();

            QMap<Product*, int> loadedOrderItems;
            for (const QJsonValue& itemVal : itemsArray) {
                QJsonObject itemObj = itemVal.toObject();
                QString productName = itemObj["productName"].toString();
                QString merchantUsername = itemObj["merchantUsername"].toString();
                int quantity = itemObj["quantity"].toInt(1);

                Product* foundProduct = nullptr;
                for (Product* p : allProducts) {
                    if (p->getName() == productName && p->getMerchantUsername() == merchantUsername) {
                        foundProduct = p;
                        break;
                    }
                }
                if (foundProduct) {
                    loadedOrderItems.insert(foundProduct, quantity);
                } else {
                    qWarning() << "Product not found during order load:" << productName << "by" << merchantUsername;
                }
            }

            if (loadedOrderItems.isEmpty() && !itemsArray.isEmpty()) {
                qWarning() << "Order for consumer" << consumer << "has item references that could not be resolved. Skipping this order potentially.";
            }

            Order* order = new Order(consumer, loadedOrderItems);
            order->setCreateTimeForLoadedOrder(creationTime);
            order->setStatus(status);
            order->setOrderId(orderId);
            orders.append(order);
        }
    }
    return orders;
}

bool FileManager::clearUserShoppingCart(const QString& username) {
    if(username.isEmpty()) {
        qDebug() << "清空购物车时，用户名为空";
        return false;
    }

    QVariantMap allCarts = FileManager::loadAllShoppingCarts();
    if(allCarts.contains(username)) {
        allCarts.remove(username);
    }

    bool success = FileManager::saveShoppingCarts(allCarts);

    return success;
}
