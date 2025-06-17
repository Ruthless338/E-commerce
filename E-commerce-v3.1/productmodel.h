#ifndef PRODUCTMODEL_H
#define PRODUCTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "product.h"
#include "authmanager.h"

class ProductModel : public QAbstractListModel
{
    Q_OBJECT
public:
    // 定义角色枚举
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        PriceRole,
        StockRole,
        CategoryRole,
        DiscountRole,
        CurrentPriceRole,
        MerchantUsernameRole,
        ImagePathRole,
        ProductPointerRole = Qt::UserRole + 10
    };

    explicit ProductModel(QObject *parent = nullptr);

    // 必须实现的虚函数
    // 返回当前商品数量
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    //
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    // 返回角色
    QHash<int, QByteArray> roleNames() const override;

    // 添加商品
    Q_INVOKABLE bool addProduct(const QString &name, const QString &desc, double price, int stock, const QString &category, const QString& merchantUsername, const QString& imagePath);
    // 搜索商品
    Q_INVOKABLE void search(
        const QString &keyword,
        int searchType,
        const QString& minPrice,
        const QString& maxPrice
    );
    // 重置搜索
    Q_INVOKABLE void resetSearch();

    // 加载/保存商品数据到文件  数据持久化
    void loadProducts();
    bool saveProducts();
    // 更新商品数据
    Q_INVOKABLE bool updateProduct(int index, const QString &name, const QString &desc, double price, int stock, const QString& imagePath);
    Q_INVOKABLE bool purchaseProduct(int index, const QString& username);

    Product* findProduct(const QString& name) const;
    Product* findProductByNameAndMerchant(const QString& name, const QString& merchantUsername) const;

    // 用于确认订单时发送单个商品数据更改信号
    bool productStockNotify(Product* productToUpdate);
    static ProductModel* instance();
public slots:
    void copyImage(const QString& srcPath, const QString& destPath);
    void setCategoryDiscount(const QString& category, double discount);
private:
    QList<Product*> m_products;  // 当前显示的商品（过滤后）
    QList<Product*> m_allProducts; // 存储所有商品
    QHash<int, QByteArray> m_roleNames;  // 角色定义

    static ProductModel* m_instance;
};

#endif // PRODUCTMODEL_H
