#include "productmodel.h"
#include "filemanager.h"
#include "book.h"
#include "clothing.h"
#include "food.h"

ProductModel::ProductModel(QObject *parent) : QAbstractListModel(parent) {
    // 初始化角色名称（与QML绑定）
    m_roleNames[NameRole] = "name";
    m_roleNames[DescriptionRole] = "description";
    m_roleNames[PriceRole] = "price";
    m_roleNames[StockRole] = "stock";
    m_roleNames[CategoryRole] = "category";
    m_roleNames[DiscountRole] = "discount";
    m_roleNames[CurrentPriceRole] = "currentPrice";

    // 加载初始商品数据
    loadProducts();
}

int ProductModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_products.size();
}

QVariant ProductModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_products.size())
        return QVariant();

    Product *product = m_products.at(index.row());
    switch (role) {
    case NameRole:
        return product->getName();
    case DescriptionRole:
        return product->getDescription();
    case PriceRole:
        return product->getPrice();
    case StockRole:
        return product->getStock();
    case CategoryRole:
        return product->getCategory();
    case DiscountRole:
        return product->getDiscount();
    case CurrentPriceRole:
        return product->getPrice();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProductModel::roleNames() const {
    return m_roleNames;
}

void ProductModel::addProduct(const QString &name, const QString &desc, double price, int stock, const QString &category) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    Product *product = nullptr;
    if (category == "图书") {
        product = new Book(name, desc, price, stock);
    } else if (category == "服装") {
        product = new Clothing(name, desc, price, stock);
    } else if (category == "食品") {
        product = new Food(name, desc, price, stock);
    }

    if (product) {
        m_products.append(product);
        m_allProducts.append(product);
        saveProducts();
    }

    endInsertRows();
}

void ProductModel::search(const QString &keyword) {
    // 此处需触发数据变化信号（如layoutChanged）
    QList<Product*> filtered;
    for (Product * product : m_allProducts) {
        if(product->getName().contains(keyword, Qt::CaseInsensitive)) {
            filtered.append(product);
        }
    }
    beginResetModel();
    m_products = filtered;
    endResetModel();
}

void ProductModel::resetSearch() {
    beginResetModel();
    m_products = m_allProducts;
    endResetModel();
}

void ProductModel::loadProducts() {
    // 通过FileManager从文件加载商品数据
    QList<Product*> loaded = FileManager::loadProducts();
    qDeleteAll(m_allProducts);
    m_allProducts.clear();

    beginResetModel();
    m_products = loaded;
    m_allProducts = loaded;
    endResetModel();
}

void ProductModel::saveProducts() {
    FileManager::saveProducts(m_allProducts);
}

void ProductModel::updateProduct(int index, const QString &name, const QString &desc, double price, int stock){
    if (index < 0 || index >= m_allProducts.size()) return ;
    Product *product = m_allProducts.at(index);
    if(!name.isEmpty()) product->setName(name);
    if(!desc.isEmpty()) product->setDescription(desc);
    if(price >= 0) product->setPrice(price);
    if(stock >= 0) product->setStock(stock);
    saveProducts();
    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
}

void ProductModel::copyImage(const QString& srcPath, const QString& destPath) {
    QString cleanSrc = srcPath;
    cleanSrc.replace("file:///", ""); // 移除 URL 前缀
    QFile src(cleanSrc);

    QString cleanDest = destPath;
    cleanDest.replace("file:///", ""); // 移除目标路径前缀
    QFileInfo destInfo(cleanDest);

    // 确保目标目录存在
    QDir().mkpath(destInfo.path());

    if (src.copy(cleanDest)) {
        qDebug() << "图片保存成功：" << cleanDest;
    } else {
        qDebug() << "图片保存失败：" << src.errorString();
    }
}

void ProductModel::setCategoryDiscount(const QString& category, double discount){
    if(category == "图书") Book::discount = discount;
    else if(category == "服装") Clothing::discount = discount;
    else if(category == "食品") Food::discount = discount;
    else {
        qDebug() << "无法改变图书、服装、食品三类之外的商品折扣" << '\n';
    }

    QVector<int>affectedRows;
    for(int i=0;i<m_products.size();i++){
        if(m_products[i]->getCategory() == category) {
            affectedRows.append(i);
        }
    }

    if(!affectedRows.isEmpty()) {
        QModelIndex top = createIndex(affectedRows.first(), 0);
        QModelIndex bottom = createIndex(affectedRows.last(), 0);
        emit dataChanged(top, bottom, {PriceRole, DiscountRole});
    }
}

