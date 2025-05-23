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
    m_roleNames[ImagePathRole] = "imagePath";

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
        return product->getBasePrice();
    case StockRole:
        return product->getStock();
    case CategoryRole:
        return product->getCategory();
    case DiscountRole:
        return product->getDiscount();
    case CurrentPriceRole:
        return product->getPrice();
    case ImagePathRole:
        return product->getImagePath();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProductModel::roleNames() const {
    return m_roleNames;
}

bool ProductModel::addProduct(const QString&name,
                              const QString& desc,
                              double price,
                              int stock,
                              const QString& category,
                              const QString& imagePath) {
    if(name.isEmpty()||desc.isEmpty()||price<0||stock<0||imagePath.isEmpty()) {
        qDebug() << "无效商品参数";
        return false;
    }
    if(category!="图书"&&category!="服装"&&category!="食品"){
        qDebug() << "无效商品种类" << category;
        return false;
    }

    Product *product = nullptr;
    if (category == "图书") {
        product = new Book(name, desc, price, stock, imagePath);
    } else if (category == "服装") {
        product = new Clothing(name, desc, price, stock, imagePath);
    } else if (category == "食品") {
        product = new Food(name, desc, price, stock, imagePath);
    }

    if(!product) {
        qDebug() << "添加商品失败：无法为product分配内存";
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_products.append(product);
    m_allProducts.append(product);
    endInsertRows();

    if(!saveProducts()) {
        qDebug() << "添加商品：保存商品至文件失败";
        return false;
    }
    return true;
}

void ProductModel::search(
    const QString &keyword,
    int searchType,
    const QString& minPrice,
    const QString& maxPrice
    ) {
    // 此处需触发数据变化信号（如layoutChanged）
    QList<Product*> filtered;

    double mi = minPrice.isEmpty() ? 0 : minPrice.toDouble();
    double ma = maxPrice.isEmpty() ? std::numeric_limits<double>::max() : maxPrice.toDouble();

    for (Product * product : m_allProducts) {
        double currentPrice = product->getPrice();
        if(currentPrice < mi || currentPrice > ma) continue;

        bool match = false;
        switch(searchType) {
        case 0:
            match = product->getName().contains(keyword, Qt::CaseInsensitive);
            break;
        case 1:
            match = product->getDescription().contains(keyword, Qt::CaseInsensitive);
            break;
        }
        if(match || keyword.isEmpty()) {
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

bool ProductModel::saveProducts() {
    return FileManager::saveProducts(m_allProducts);
}

bool ProductModel::updateProduct(int index, const QString &name, const QString &desc, double price, int stock, const QString& imagePath){
    if (index < 0 || index >= m_allProducts.size()) return false;
    Product *product = m_allProducts.at(index);
    if(!name.isEmpty()) product->setName(name);
    if(!desc.isEmpty()) product->setDescription(desc);
    if(price >= 0) product->setPrice(price);
    if(stock >= 0) product->setStock(stock);
    if(!imagePath.isEmpty()) product->setImagePath(imagePath);
    bool res = saveProducts();
    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
    qDebug() << "商品更新成功";
    return res;
}

void ProductModel::copyImage(const QString& srcPath, const QString& destPath) {
    QString cleanSrc = srcPath;
    cleanSrc.replace("file:///", ""); // 移除 URL 前缀
    if(cleanSrc.startsWith("/")) {
        cleanSrc = cleanSrc.mid(1);
    }
    QFile src(cleanSrc);

    QString cleanDest = destPath;
    cleanDest.replace("file:///", ""); // 移除目标路径前缀
    if(cleanDest.startsWith("/")) {
        cleanDest = cleanDest.mid(1);
    }
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
    saveProducts();
}

bool ProductModel::purchaseProduct(int index, const QString& username) {
    if(index < 0 || index >= m_products.size()) return false;
    Product* product = m_products.at(index);

    if(product->getStock()<1) {
        qDebug() << "库存不足";
        return false;
    }

    double currentPrice = product->getPrice();
    if(AuthManager::getBalance(username) < currentPrice) {
        qDebug() << "余额不足";
        return false;
    }

    product->setStock(product->getStock() - 1);
    if(!AuthManager::deductBalance(username, currentPrice)) {
        qDebug() << "扣款失败";
        return false;
    }

    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
    saveProducts();

    return true;
}

