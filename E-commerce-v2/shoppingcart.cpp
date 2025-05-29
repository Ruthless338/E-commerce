#include "shoppingcart.h"
#include "filemanager.h"
#include "productmodel.h"
#include "globalstate.h"
#include <QMetaType>
Q_DECLARE_METATYPE(QList<QVariant>)

bool ShoppingCart::addItem(Product *product, int quantity) {
    if(product->getStock() < quantity) {
        qDebug() << "库存不足";
        return false;
    }
    items[product] += quantity;
    emit cartChanged();
    saveShoppingCart(GlobalState::instance()->username());
    return true;
}

bool ShoppingCart::updateQuantity(Product* product, int newQuantity) {
    if(!items.contains(product)) return false;
    items[product] = newQuantity;
    emit cartChanged();
    saveShoppingCart(GlobalState::instance()->username());
    return true;
}

bool ShoppingCart::removeItem(Product* product) {
    if(!items.contains(product)) return false;
    items.remove(product);
    emit cartChanged();
    saveShoppingCart(GlobalState::instance()->username());
    return true;
}

double ShoppingCart::getTotalPrice() const {
    double total = 0;
    for(auto it = items.begin(); it!= items.end(); it++) {
        total += it.key()->getPrice() * it.value();
    }
    return total;
}

void ShoppingCart::loadShoppingCart(const QString& username) {
    QVariantMap allCarts = FileManager::loadAllShoppingCarts();
    QVariantMap userCart = allCarts.value(username).toMap();

    items.clear();
    for (auto it = userCart.begin(); it != userCart.end(); ++it) {
        QStringList parts = it.key().split("_");
        QString productName = parts[0], merchant = parts[1];
        Product* product = ProductModel::instance()->findProduct(productName);
        if (product) items[product] = it.value().toInt();
    }
}

void ShoppingCart::saveShoppingCart(const QString& username) {
    QVariantMap allCarts = FileManager::loadAllShoppingCarts();
    QVariantMap userCart;

    for (auto it = items.begin(); it != items.end(); ++it) {
        QString key = it.key()->getName() + "_" + it.key()->getMerchantUsername();
        userCart[key] = it.value();
    }

    allCarts[username] = userCart;
    FileManager::saveShoppingCarts(allCarts);
}

QList<QVariant> ShoppingCart::getCartItems() const {
    QVariantList itemsList;
    for (auto it = items.begin(); it != items.end(); ++it) {
        QVariantMap item;
        item["name"] = it.key()->getName();
        item["price"] = it.key()->getPrice();
        item["imagePath"] = it.key()->getImagePath();
        item["description"] = it.key()->getDescription();
        item["quantity"] = it.value();
        itemsList.append(item);
    }
    return itemsList;
}

bool ShoppingCart::updateQuantityByName(const QString& productName, int newQuantity) {
    for(auto it = items.begin();it != items.end(); it++) {
        if(it.key()->getName() == productName) {
            if (newQuantity > it.key()->getStock()) {
                qDebug() << "库存不足，当前库存：" << it.key()->getStock();
                return false;
            }
            items[it.key()] = newQuantity;
            emit cartChanged();
            saveShoppingCart(GlobalState::instance()->username());
            return true;
        }
    }
    return false;
}

bool ShoppingCart::removeItemByName(const QString& productName) {
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it.key()->getName() == productName) {
            items.remove(it.key());
            emit cartChanged();
            saveShoppingCart(GlobalState::instance()->username());
            return true;
        }
    }
    return false;
}

