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

class FileManager
{
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
};

#endif // FILEMANAGER_H
