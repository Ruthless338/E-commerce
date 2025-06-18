#ifndef PRODUCTMODEL_H
#define PRODUCTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QVariantMap>
#include "product.h"

class ProductModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        PriceRole,
        StockRole,
        CategoryRole,
        DiscountRole,
        ImagePathRole,
        MerchantUsernameRole,
        BasePriceRole 
    };

    explicit ProductModel(QObject *parent = nullptr); // 构造函数

    // Q_INVOKABLE 方法，保持与单机版一致的签名和返回值
    Q_INVOKABLE void search(const QString &keyword, int searchType, const QString& minPrice = "-1", const QString& maxPrice = "-1");
    Q_INVOKABLE bool addProduct(const QString &name, const QString &desc, double price, int stock, const QString &category, const QString& merchantUsername, const QString& imagePath);
    Q_INVOKABLE bool updateProduct(Product *productToUpdate, const QString &name, const QString &desc, double price, int stock, const QString& imagePath);
    Q_INVOKABLE bool purchaseProduct(int index, const QString& username); // QML 调用时的参数
    Q_INVOKABLE void setCategoryDiscount(const QString& category, double discount);
    Q_INVOKABLE bool productStockNotify(Product* productToUpdate); // 单机版的方法
    Q_INVOKABLE void copyImage(const QString& srcPath, const QString& destPath); // 保持

    // QAbstractListModel 的虚函数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantMap findProductData(const QString& name, const QString& merchantUsername) const; // 返回 QVariantMap 更安全

private:
    void loadProductsFromServer(); // 内部方法，从服务器加载数据到 m_productsData
    QList<QVariantMap> m_productsData; // 存储从服务器获取的商品数据
    QHash<int, QByteArray> m_roleNamesH;
    // static ProductModel* instance; // 如果是单例，需要有实例
};
#endif // PRODUCTMODEL_H
