#ifndef SERVERSHOPPINGCARTMANAGER_H
#define SERVERSHOPPINGCARTMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariantList>

class ServerProductManager; // 前向声明
class Product;

class ServerShoppingCartManager : public QObject {
    Q_OBJECT
public:
    // ServerProductManager 用于查找商品实例
    explicit ServerShoppingCartManager(ServerProductManager* productMgr, QObject *parent = nullptr);

    // 返回的是可序列化的 QVariantList，每个元素是 QVariantMap 代表一个购物车项
    QVariantList getCartItems(const QString& username);
    bool addItem(const QString& username, const QString& productName, const QString& merchantUsername, int quantity);
    bool removeItem(const QString& username, const QString& productName, const QString& merchantUsername);
    bool updateQuantity(const QString& username, const QString& productName, const QString& merchantUsername, int newQuantity);
    bool clearCart(const QString& username); // 订单支付成功后调用

    // 内部辅助获取购物车，用于订单处理等
    QMap<Product*, int> getCartForUserInternal(const QString& username);


private:
    // username -> (product_identifier -> quantity)
    // product_identifier 采用 "productName_merchantUsername" 的形式
    QMap<QString, QMap<QString, int>> m_allUserCarts;
    ServerProductManager* m_productManager; // 依赖 ProductManager 查找商品

    void loadAllCartsFromFile();
    bool saveAllCartsToFile();
    QString getProductIdentifier(Product* product);
    Product* findProductByIdentifier(const QString& identifier);
};

#endif // SERVERSHOPPINGCARTMANAGER_H
