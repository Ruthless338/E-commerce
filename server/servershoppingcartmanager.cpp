#include "servershoppingcartmanager.h"
#include "serverproductmanager.h"
#include "filemanager.h" // FileManager 应该有 loadAllShoppingCarts 和 saveShoppingCarts
#include "product.h"     // 用于 Product* 类型
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ServerShoppingCartManager::ServerShoppingCartManager(ServerProductManager* productMgr, QObject *parent)
    : QObject(parent), m_productManager(productMgr) {
    loadAllCartsFromFile();
}

QString ServerShoppingCartManager::getProductIdentifier(Product* product) {
    if (!product) return QString();
    return product->getName() + "_" + product->getMerchantUsername();
}

Product* ServerShoppingCartManager::findProductByIdentifier(const QString& identifier) {
    QStringList parts = identifier.split('_');
    if (parts.size() == 2) {
        return m_productManager->findProductByNameAndMerchant(parts[0], parts[1]);
    }
    return nullptr;
}


void ServerShoppingCartManager::loadAllCartsFromFile() {
    m_allUserCarts.clear();
    QVariantMap loadedCarts = FileManager::loadAllShoppingCarts(); // FileManager 返回 QVariantMap
    for (auto userIt = loadedCarts.constBegin(); userIt != loadedCarts.constEnd(); ++userIt) {
        QString username = userIt.key();
        QVariantMap userCartData = userIt.value().toMap();
        QMap<QString, int> cartForUser;
        for (auto itemIt = userCartData.constBegin(); itemIt != userCartData.constEnd(); ++itemIt) {
            // key 是 "productName_merchantUsername"
            cartForUser.insert(itemIt.key(), itemIt.value().toInt());
        }
        m_allUserCarts.insert(username, cartForUser);
    }
    qInfo() << "ServerShoppingCartManager: Loaded" << m_allUserCarts.count() << "user carts.";
}

bool ServerShoppingCartManager::saveAllCartsToFile() {
    QVariantMap cartsToSave;
    for (auto userIt = m_allUserCarts.constBegin(); userIt != m_allUserCarts.constEnd(); ++userIt) {
        QVariantMap userCartVariantMap;
        const QMap<QString, int>& cartItems = userIt.value();
        for (auto itemIt = cartItems.constBegin(); itemIt != cartItems.constEnd(); ++itemIt) {
            userCartVariantMap.insert(itemIt.key(), itemIt.value());
        }
        cartsToSave.insert(userIt.key(), userCartVariantMap);
    }
    bool success = FileManager::saveShoppingCarts(cartsToSave);
    if (success) {
        qInfo() << "ServerShoppingCartManager: All shopping carts saved.";
    } else {
        qWarning() << "ServerShoppingCartManager: Failed to save shopping carts.";
    }
    return success;
}


QVariantList ServerShoppingCartManager::getCartItems(const QString& username) {
    QVariantList itemsList;
    if (!m_allUserCarts.contains(username)) {
        return itemsList;
    }

    const QMap<QString, int>& userCart = m_allUserCarts[username];
    for (auto it = userCart.constBegin(); it != userCart.constEnd(); ++it) {
        Product* product = findProductByIdentifier(it.key());
        if (product) {
            QVariantMap itemMap;
            itemMap["name"] = product->getName();
            itemMap["description"] = product->getDescription();
            itemMap["price"] = product->getPrice(); // Current price
            itemMap["imagePath"] = product->getImagePath();
            itemMap["merchantUsername"] = product->getMerchantUsername();
            itemMap["quantity"] = it.value();
            itemsList.append(itemMap);
        } else {
            qWarning() << "ServerShoppingCartManager: Product for identifier" << it.key() << "not found while getting cart for" << username;
            // Optionally remove invalid item from cart here
        }
    }
    return itemsList;
}

bool ServerShoppingCartManager::addItem(const QString& username, const QString& productName, const QString& merchantUsername, int quantity) {
    if (quantity <= 0) return false;
    Product* product = m_productManager->findProductByNameAndMerchant(productName, merchantUsername);
    if (!product) {
        qWarning() << "ServerShoppingCartManager: Cannot add to cart, product not found:" << productName << "by" << merchantUsername;
        return false;
    }

    QString identifier = getProductIdentifier(product);
    int currentInCart = m_allUserCarts[username].value(identifier, 0);
    int newTotalQuantity = currentInCart + quantity;

    if (product->getAvailableStock() < newTotalQuantity) { // Check against total desired in cart
        qWarning() << "ServerShoppingCartManager: Not enough stock for" << productName << ". Available:" << product->getAvailableStock() << "Requested in cart:" << newTotalQuantity;
        return false;
    }

    m_allUserCarts[username][identifier] = newTotalQuantity;
    return saveAllCartsToFile();
}

bool ServerShoppingCartManager::removeItem(const QString& username, const QString& productName, const QString& merchantUsername) {
    Product* product = m_productManager->findProductByNameAndMerchant(productName, merchantUsername);
    if (!product) return false; // Or if identifier not found directly

    QString identifier = getProductIdentifier(product);
    if (m_allUserCarts.contains(username) && m_allUserCarts[username].contains(identifier)) {
        m_allUserCarts[username].remove(identifier);
        if (m_allUserCarts[username].isEmpty()) {
            m_allUserCarts.remove(username);
        }
        return saveAllCartsToFile();
    }
    return false;
}

bool ServerShoppingCartManager::updateQuantity(const QString& username, const QString& productName, const QString& merchantUsername, int newQuantity) {
    if (newQuantity < 0) return false; // Cannot have negative quantity
    if (newQuantity == 0) {
        return removeItem(username, productName, merchantUsername);
    }

    Product* product = m_productManager->findProductByNameAndMerchant(productName, merchantUsername);
    if (!product) {
        qWarning() << "ServerShoppingCartManager: Cannot update cart, product not found:" << productName << "by" << merchantUsername;
        return false;
    }
    if (product->getAvailableStock() < newQuantity) {
        qWarning() << "ServerShoppingCartManager: Not enough stock for" << productName << "to update quantity to" << newQuantity;
        return false;
    }

    QString identifier = getProductIdentifier(product);
    m_allUserCarts[username][identifier] = newQuantity;
    return saveAllCartsToFile();
}

bool ServerShoppingCartManager::clearCart(const QString& username) {
    if (m_allUserCarts.contains(username)) {
        m_allUserCarts.remove(username);
        return saveAllCartsToFile();
    }
    return true; // Cart was already empty or user didn't exist, effectively cleared
}

QMap<Product*, int> ServerShoppingCartManager::getCartForUserInternal(const QString& username) {
    QMap<Product*, int> cartMap;
    if (!m_allUserCarts.contains(username)) {
        return cartMap;
    }
    const QMap<QString, int>& userCartIdentifiers = m_allUserCarts[username];
    for (auto it = userCartIdentifiers.constBegin(); it != userCartIdentifiers.constEnd(); ++it) {
        Product* product = findProductByIdentifier(it.key());
        if (product) {
            cartMap.insert(product, it.value());
        } else {
            qWarning() << "ServerShoppingCartManager: Product for identifier" << it.key() << "not found during internal cart retrieval for" << username;
        }
    }
    return cartMap;
}
