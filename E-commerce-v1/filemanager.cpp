#include "filemanager.h"

QMap<QString, User*> FileManager::loadAllUsers()
{
    QMap<QString, User*> users;
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v1/data/users.json");
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
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v1/data/products.json");
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
        QString imagePath = obj["imagePath"].toString();

        Product* product = nullptr;
        if (category == "图书") product = new Book(name, desc, price, stock);
        else if (category == "服装") product = new Clothing(name, desc, price, stock);
        else if (category == "食品") product = new Food(name, desc, price, stock);
        if (product) {
            product->setImagePath(imagePath); // 设置图片路径
            products.append(product);
        }
    }
    return products;
}

void FileManager::saveUser(const User* user)
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
        return ;
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

    QDir dir("D:/Qt_projects/E-commerce-v1/data");
    if(!dir.exists()) {
        dir.mkpath(".");
    }

    // 写入文件
    QFile file("D:/Qt_projects/E-commerce/E-commerce-v1/data/users.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法写入 users.json" << file.errorString();
        return;
    }
    file.write(QJsonDocument(jsonArray).toJson());
    file.close();
}

void FileManager::saveProducts(const QList<Product*>& products){
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
        obj["price"] = product->getPrice();
        obj["stock"] = product->getStock();
        obj["category"] = product->getCategory();
        obj["imagePath"] = product->getImagePath();
        productArray.append(obj);
    }
    root["products"] = productArray;

    QFile file("D:/Qt_projects/E-commerce/E-commerce-v1/data/products.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法写入 products.json";
        return;
    }
    file.write(QJsonDocument(root).toJson());
    file.close();
}

