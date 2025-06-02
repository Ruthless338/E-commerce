#ifndef SERVERPRODUCTMANAGER_H
#define SERVERPRODUCTMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantMap> // 虽然主要在内部使用，但有时返回复杂结构可能用QVariantMap

// 前向声明 Product 类，实际会包含 "product.h"
class Product;
class Book;
class Clothing;
class Food;


class ServerProductManager : public QObject {
    Q_OBJECT
public:
    explicit ServerProductManager(QObject *parent = nullptr);
    ~ServerProductManager();

    // 从 ProductModel 改编而来的数据管理方法
    QList<Product*> getAllProducts();
    QList<Product*> searchProducts(const QString &keyword, int searchType, double minPrice, double maxPrice);
    bool addProduct(const QString& name, const QString& desc, double price, int stock,
                    const QString& category, const QString& merchantUsername, const QString& imagePath);
    bool updateProduct(const QString& originalProductName, const QString& merchantUsername, // 用原名和商家定位
                       const QString& newName, const QString& newDescription,
                       double newBasePrice, int newStock, const QString& newImagePath);
    void setCategoryDiscount(const QString& category, double discount); // discount 是 0.0 - 1.0 的值

    Product* findProductByNameAndMerchant(const QString& name, const QString& merchantUsername); // 辅助函数

    // 当订单支付成功，实际扣减库存并释放冻结库存
    bool confirmStockDeduction(Product* product, int quantity);
    // 当订单创建或支付失败，释放冻结库存
    bool releaseFrozenStock(Product* product, int quantity);
    // 当订单创建时，尝试冻结库存
    bool freezeStock(Product* product, int quantity);


private:
    QList<Product*> m_allProducts; // 内存中持有的所有商品

    void loadProductsFromFile();
    bool saveProductsToFile();
};

#endif // SERVERPRODUCTMANAGER_H
