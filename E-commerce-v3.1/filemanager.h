// filemanager.h (客户端)
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include "Merchant.h"
#include "consumer.h"
#include "book.h"
#include "clothing.h"
#include "food.h"

// 前向声明你的数据类
class User;
class Product;
class Order;
class ShoppingCart; // 假设这是购物车项的数据结构

class FileManager {
public:
    FileManager() = delete; // 使其成为纯静态工具类

    // --- User Data ---
    static QList<User*> loadAllUsers();
    static bool saveUser(const User* user); // 保存单个用户（可能是更新或新增）
    static bool saveAllUsers(const QList<User*>& users); // 覆盖保存所有用户

    // --- Product Data ---
    static QList<Product*> loadAllProducts();
    static bool saveProduct(const Product* product);
    static bool saveAllProducts(const QList<Product*>& products);
    static bool deleteProduct(const QString& productName, const QString& merchantUsername);

    // --- Order Data ---
    static QList<Order*> loadOrders(const QList<Product*>& allProducts); // 加载订单时需要商品信息来恢复Product指针
    static bool saveOrder(const Order* order);
    static bool saveAllOrders(const QList<Order*>& orders); // 包含所有订单的完整列表

    // --- Shopping Cart Data (每个用户一个购物车文件) ---
    static QList<ShoppingCart*> loadShoppingCarts();
    static bool saveShoppingCart(const ShoppingCart* shoppingCart);
    static bool saveAllShoppingCarts(QList<ShoppingCart*> shoppingCarts);
    static bool clearUserShoppingCart(const QString consumerUsername);

private:
    // 网络文件操作的辅助方法
    static QString remoteReadFile(const QString& relativeFilePath);
    static bool remoteWriteFile(const QString& relativeFilePath, const QString& content);
    static bool remoteFileExists(const QString& relativeFilePath);
    static bool remoteDeleteFile(const QString& relativeFilePath);

    // 辅助函数：发送请求并等待响应（简化版，用于 FileManager 内部）
    static QJsonObject sendRequestAndWaitForFileManager(const QJsonObject& request, int timeoutMs = 5000);

    // JSON (反)序列化辅助方法 (这些方法保持不变，因为你的类有 toJson/fromJson)
    static User* userFromJson(const QJsonObject& jsonObj);
    static QJsonObject userToJson(const User* user);
    // ... 类似地为 Product, Order, ShoppingCartItemData 实现 toJson/fromJson 辅助
    // 注意：Product 的子类 (Book, Clothing, Food) 在序列化时需要包含类型信息
    static Product* productFromJson(const QJsonObject& jsonObj);
    static QJsonObject productToJson(const Product* product);
    static Order* orderFromJson(const QJsonObject& jsonObj, const QList<Product*>& allProducts);
    static QJsonObject orderToJson(const Order* order);
    static ShoppingCart* shoppingCartFromJson(const QJsonObject& jsonObj, const QList<Product*>& allProducts);
    static QJsonObject shoppingCartToJson(const ShoppingCart* item);

    static const QString USERS_FILE;
    static const QString PRODUCTS_FILE;
    static const QString ORDERS_FILE;
    static const QString SHOPPING_CARTS_FILE; // 购物车文件存放的子目录名
};
#endif // FILEMANAGER_H
