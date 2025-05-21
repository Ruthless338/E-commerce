#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include "user.h"
#include <QMap>
#include <QFile>
#include <product.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include "consumer.h"
#include "Merchant.h"
#include "book.h"
#include "clothing.h"
#include "food.h"

class FileManager
{
public:
    // Map: username->User
    static QMap<QString, User*> loadAllUsers();
    // 从文件加载商品数据，解析JSON并实例化对应子类
    static QList<Product*> loadProducts();
    static bool userExist(const QString& username);
    static void saveUser(const User* user);
    static void saveProducts(const QList<Product*>& products);
};

#endif // FILEMANAGER_H
