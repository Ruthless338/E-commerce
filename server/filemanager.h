#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include "user.h"
#include "order.h"
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
#include <QObject>
#include <QMutex>

class FileManager : public QObject
{
    Q_OBJECT
public:
    static QMap<QString, User*> loadAllUsers();
    static QList<Product*> loadProducts();
    static bool userExist(const QString& username);
    static bool saveUser(const User* user);
    static bool saveProducts(const QList<Product*>& products);

    static bool saveShoppingCarts(const QVariantMap& allCarts);
    static QVariantMap loadAllShoppingCarts();

    static bool saveOrders(const QList<Order*>& orders);
    static QList<Order*> loadOrders(const QList<Product*>& allProducts);
    static bool clearUserShoppingCart(const QString& username);

    static QJsonDocument loadJson(const QString& filename);
    static bool saveJson(const QString& filename, const QJsonDocument& doc);

private:
    static QString dataPathPrefix;
    static QMutex fileMutex; // 静态互斥锁，保护所有文件访问
};

#endif // FILEMANAGER_H
