#include "productmodel.h"
#include "authmanager.h" // 为了使用 AuthManager::sendRequestAndWait
#include "networkclient.h"
#include <QJsonArray>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>


// ProductModel* ProductModel::instance = nullptr; // 如果是单例
extern GlobalState* globalStateInstance;

ProductModel::ProductModel(QObject *parent) : QAbstractListModel(parent) {
    // instance = this;
    m_roleNamesH[NameRole] = "name";
    m_roleNamesH[DescriptionRole] = "description";
    m_roleNamesH[PriceRole] = "price";
    m_roleNamesH[StockRole] = "stock";
    m_roleNamesH[CategoryRole] = "category";
    m_roleNamesH[DiscountRole] = "discount";
    m_roleNamesH[ImagePathRole] = "imagePath";
    m_roleNamesH[MerchantUsernameRole] = "merchantUsername";
    m_roleNamesH[BasePriceRole] = "basePrice";

    loadProductsFromServer();
}

QHash<int, QByteArray> ProductModel::roleNames() const {
    return m_roleNamesH;
}

void ProductModel::loadProductsFromServer() {
    QJsonObject request;
    request["action"] = "getProducts";

    QJsonObject response = AuthManager::sendRequestAndWait(request); // 使用 AuthManager 的辅助函数

    if (response["status"].toString() == "success") {
        beginResetModel();
        m_productsData.clear();
        QJsonArray productsArray = response["data"].toObject()["products"].toArray();
        for (const QJsonValue &val : productsArray) {
            m_productsData.append(val.toObject().toVariantMap());
        }
        endResetModel();
    } else {
        qWarning() << "ProductModel: Failed to load products -" << response["message"].toString();
    }
}

// QML调用的 search
void ProductModel::search(const QString &keyword, int searchType, const QString& minPriceStr, const QString& maxPriceStr) {
    QJsonObject request;
    request["action"] = "searchProducts";
    QJsonObject payload;
    payload["keyword"] = keyword;
    payload["searchType"] = searchType; // 0 for name, 1 for category in your old code
    // 根据服务器接口调整searchType的含义，这里假设服务器也用0/1
    // minPrice 和 maxPrice 处理
    bool okMin, okMax;
    double minP = minPriceStr.toDouble(&okMin);
    double maxP = maxPriceStr.toDouble(&okMax);
    if(okMin && minP >=0) payload["minPrice"] = minP;
    if(okMax && maxP >=0) payload["maxPrice"] = maxP;

    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        beginResetModel();
        m_productsData.clear();
        QJsonArray productsArray = response["data"].toObject()["products"].toArray();
        for (const QJsonValue &val : productsArray) {
            m_productsData.append(val.toObject().toVariantMap());
        }
        endResetModel();
    } else {
        qWarning() << "ProductModel: Search failed -" << response["message"].toString();
    }
}

bool ProductModel::addProduct(const QString &name, const QString &desc, double price, int stock, const QString &category, const QString& merchantUsername, const QString& imagePath) {
    QJsonObject request;
    request["action"] = "addProduct";
    QJsonObject payload;
    payload["name"] = name;
    payload["description"] = desc;
    payload["price"] = price; // 这是 basePrice
    payload["stock"] = stock;
    payload["category"] = category;
    // merchantUsername 来自QML参数，服务器应验证其是否为当前登录商家
    payload["merchantUsername"] = merchantUsername;
    payload["imagePath"] = imagePath; // 图片路径如何处理是个大问题
        // 简单假设服务器能识别这个路径，或客户端已上传
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        // 添加成功后，重新从服务器加载所有商品以更新模型
        loadProductsFromServer();
        return true;
    }
    qWarning() << "ProductModel: Failed to add product -" << response["message"].toString();
    return false;
}

bool ProductModel::updateProduct(Product *productToUpdate, const QString &name, const QString &desc, double price, int stock, const QString& imagePath) {

    if (!productToUpdate) return false;

    qWarning() << "ProductModel::updateProduct called. This function's first parameter (Product*) is problematic for network version without QML changes. Assuming QML provides original product identifier elsewhere or via a different mechanism not directly through Product*.";
    if (!productToUpdate) return false;
    QString originalName = productToUpdate->getName();
    QString originalMerchant = productToUpdate->getMerchantUsername();


    QJsonObject request;
    request["action"] = "updateProduct";
    QJsonObject payload;
    payload["originalName"] = originalName;
    payload["originalMerchantUsername"] = originalMerchant; // 服务器端也需要商家名来唯一确定
    payload["name"] = name;
    payload["description"] = desc;
    payload["price"] = price; // basePrice
    payload["stock"] = stock;
    payload["imagePath"] = imagePath;
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        loadProductsFromServer(); // 更新成功后刷新
        return true;
    }
    qWarning() << "ProductModel: Failed to update product -" << response["message"].toString();
    return false;
}


bool ProductModel::purchaseProduct(int index, const QString& username) {
    // QML 调用时，index 是基于当前 ProductModel 的视图索引
    if (index < 0 || index >= m_productsData.size()) return false;

    const QVariantMap& productMap = m_productsData.at(index);
    QString productName = productMap.value("name").toString();
    QString merchantUsername = productMap.value("merchantUsername").toString();

    QJsonObject request;
    request["action"] = "purchaseProductDirectly"; // 假设服务器有此action
    QJsonObject payload;
    payload["username"] = username; // 购买者
    payload["productName"] = productName;
    payload["merchantUsername"] = merchantUsername;
    payload["quantity"] = 1; // 直接购买通常是1个
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        // 购买成功，需要更新本地商品库存（通过刷新）和用户余额
        loadProductsFromServer(); // 刷新商品列表（库存变化）
        if (globalStateInstance && globalStateInstance->username() == username) {
            // 服务器应在响应中返回新的余额
            QJsonObject data = response["data"].toObject();
            if (data.contains("newBalance")) {
                globalStateInstance->setBalance(data["newBalance"].toDouble());
            } else {
                AuthManager::getBalance(username); // 重新获取余额
            }
        }
        return true;
    }
    qWarning() << "ProductModel: Purchase failed -" << response["message"].toString();
    return false;
}

void ProductModel::setCategoryDiscount(const QString& category, double discount) {
    QJsonObject request;
    request["action"] = "setCategoryDiscount";
    QJsonObject payload;
    payload["category"] = category;
    payload["discount"] = discount; // 服务器期望的折扣表示方式，如0.1表示10% off
    request["payload"] = payload;

    QJsonObject response = AuthManager::sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        loadProductsFromServer(); // 价格会变，刷新列表
    } else {
        qWarning() << "ProductModel: Failed to set category discount -" << response["message"].toString();
    }
}

bool ProductModel::productStockNotify(Product* productToUpdate) {
    Q_UNUSED(productToUpdate);
    qDebug() << "ProductModel::productStockNotify called. Stock will be updated on next full load from server.";
    return true; // 总是返回true，避免QML端逻辑中断
}

void ProductModel::copyImage(const QString& srcPath, const QString& destPath) {
    QString cleanSrc = srcPath;
    if (srcPath.startsWith("file:///")) { // QML传来的路径可能是URL格式
        cleanSrc = QUrl(srcPath).toLocalFile();
    }
    QFile src(cleanSrc);

    QString cleanDest = destPath;
    if (destPath.startsWith("file:///")) {
        cleanDest = QUrl(destPath).toLocalFile();
    }
    QFileInfo destInfo(cleanDest);
    QDir().mkpath(destInfo.path()); // 确保目标目录存在

    if (QFile::exists(cleanDest)) { // 如果目标文件已存在，先删除
        QFile::remove(cleanDest);
    }

    if (src.copy(cleanDest)) {
        qDebug() << "Image copied (local):" << cleanDest;
    } else {
        qDebug() << "Image copy failed (local):" << src.errorString() << "from" << cleanSrc << "to" << cleanDest;
    }
}


int ProductModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_productsData.size();
}

QVariant ProductModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_productsData.size() || index.row() < 0)
        return QVariant();

    const QVariantMap &productMap = m_productsData.at(index.row());
    switch (role) {
    case NameRole: return productMap.value("name").toString();
    case DescriptionRole: return productMap.value("description").toString();
    case PriceRole: return productMap.value("price").toDouble(); // 服务器计算的当前售价
    case StockRole: return productMap.value("stock").toInt();
    case CategoryRole: return productMap.value("category").toString();
    case DiscountRole: return productMap.value("discount", 0.0).toDouble(); // 服务器返回的折扣率
    case ImagePathRole: return productMap.value("imagePath").toString();
    case MerchantUsernameRole: return productMap.value("merchantUsername").toString();
    case BasePriceRole: return productMap.value("basePrice", productMap.value("price")).toDouble(); // 如果服务器没单独给basePrice，就用price
    default: return QVariant();
    }
}

QVariantMap ProductModel::findProductData(const QString& name, const QString& merchantUsername) const {
    for(const QVariantMap& pData : m_productsData) {
        if (pData.value("name").toString() == name && pData.value("merchantUsername").toString() == merchantUsername) {
            return pData;
        }
    }
    return QVariantMap(); // Not found
}
