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

// updateProduct 的 QML 接口是 Product* productToUpdate
// 这在网络版中很麻烦，因为客户端没有 Product* 对象的概念了，只有 QVariantMap
// 如果QML端强行保留了这个调用方式，我们需要根据 productToUpdate 的某些唯一信息来定位
bool ProductModel::updateProduct(Product *productToUpdate, const QString &name, const QString &desc, double price, int stock, const QString& imagePath) {
    // 这是一个大问题：QML 传递了 Product*，但这个指针在客户端是没有意义的，
    // 因为 Product 对象只存在于单机版内存中，或者说其数据现在是 m_productsData 中的 QVariantMap。
    // 假设 productToUpdate 在QML中是通过 modelData (即ProductDelegate的model) 得到的，
    // 而 modelData 背后是 m_productsData 中的一个 QVariantMap。
    // QML 无法直接传递那个 QVariantMap 或其索引。
    //
    // 折衷方案：如果QML中 ProductDelegate 有一个 index 属性，或者能通过 productToUpdate (如果它实际是个索引或包含索引)
    // 获取到它在 m_productsData 中的原始名称和商家。
    //
    // 关键：需要一种方法从 QML 传递过来的 `productToUpdate` 得到它在服务器上的唯一标识。
    // 在你的单机版中，`productToUpdate` 是一个真实的指针。
    // 如果QML是通过 `ListView.model.get(index)` 传递的，那 `productToUpdate` 可能是一个包含数据的JS对象。
    //
    // **假设QML端调用 updateProduct 时，第一个参数能以某种方式提供原始商品名和商家名。**
    // **如果不能，这个函数签名必须改，QML也必须改。**
    // **为了不改QML，我们只能做最大努力的猜测或要求QML提供额外信息。**
    //
    // 最坏情况：QML传过来的 `productToUpdate` 完全无用。
    // 我们需要QML在调用时，额外传递一个 `originalProductName` 和 `originalMerchantUsername`。
    // 如果做不到，那这个不改QML的目标就无法完美达成。
    //
    // 假设QML调用类似：`productModel.updateProduct(currentProductDelegate.modelData, newName, newDesc, ...)`
    // 并且 `currentProductDelegate.modelData` 包含 `originalName` 和 `originalMerchant` 字段。
    // 实际上，QML中 `Product*` 类型的参数会很复杂。
    //
    // **如果单机版 QML 中 `updateProduct` 的第一个参数是 `index` 而不是 `Product*`，那就好办了。**
    // 假设是 index：
    // `bool ProductModel::updateProduct(int productIndex, ...)`
    // `const QVariantMap& originalProduct = m_productsData.at(productIndex);`
    // `QString originalName = originalProduct.value("name").toString();`
    // `QString originalMerchant = originalProduct.value("merchantUsername").toString();`
    //
    // **如果 QML 传的是 `Product*`，并且你的 `Product` 类在QML中注册为了一个类型，并且QML中 `productToUpdate` 是这个类型的实例**
    // 那么 `productToUpdate->getName()` 等方法是有效的，但这需要 `Product` 类在客户端仍然存在且被QML实例化。
    // 为了最少改动，我们假设QML能以某种方式提供原商品的唯一标识。
    // 这里我将假设 `productToUpdate` 包含 `name` 和 `merchantUsername` 属性，代表要修改的商品。

    if (!productToUpdate) return false; // 或者处理空指针

    // 从 productToUpdate 中提取原商品的标识符
    // 这取决于 productToUpdate 在QML中到底是什么。
    // 假设它是一个可以访问属性的 QObject* (如果Product类在QML注册并使用)
    // 或者是一个包含原始数据的 QVariantMap (如果QML通过 get(index) 获取)
    // 我们需要原商品名来定位服务器上的商品。
    // 最简单的方式是 QML 在调用此方法时直接传递 originalName 和 originalMerchantUsername。
    // 既然目标是不改QML，这是一个难点。
    //
    // 我们退一步，假设 QML 传过来的 `name` 属性是原始名称（不合理，因为name可能就是要改的）
    // 或者，如果 productToUpdate 确实是一个有效的 Product* （意味着 Product 类定义在客户端仍然存在，
    // 并且 ProductModel 内部仍然管理 Product* 列表，只是数据来源是服务器）。
    // 这与 m_productsData 是 QList<QVariantMap> 冲突。
    //
    // **妥协：假定QML在调用 `updateProduct` 时，会想办法传入要更新的商品的唯一标识，**
    // **例如，`updateProduct` 的签名在单机版中可能实际是：**
    // `bool updateProduct(const QString& originalProductName, const QString& originalMerchant, ... new values ...)`
    // 如果是这样，就好办。如果真的是 `Product*`，则非常棘手且几乎不可能不改QML。
    //
    // **为了继续，我将假设QML通过某种方式（例如通过一个额外的全局变量或参数）指明了要修改的商品。**
    // **这里我们无法直接使用 `productToUpdate`。**
    // **你需要检查你的单机版QML是如何调用这个函数的。**

    qWarning() << "ProductModel::updateProduct called. This function's first parameter (Product*) is problematic for network version without QML changes. Assuming QML provides original product identifier elsewhere or via a different mechanism not directly through Product*.";
    // **你需要找到一种方法来获取要更新的商品的原始名称和商家名**
    // 示例：假设它们存储在 GlobalState 中（不推荐，但为了不改QML的极端情况）
    // QString originalName = globalStateInstance->getSelectedItemOriginalName();
    // QString originalMerchant = globalStateInstance->getSelectedItemOriginalMerchant();
    // 如果找不到，则此函数无法正确工作。
    // return false;

    // **如果你的ProductModel在单机版中确实返回了Product*给QML，并且QML用这个指针调用了此方法**
    // 那么，你的ProductModel在网络版中，即使数据来自服务器，也需要在内部创建Product对象实例
    // 并将这些实例暴露给QML。这样m_productsData就应该是 QList<Product*>，而不是QList<QVariantMap>。
    // 这意味着 Product.h/cpp 需要保留在客户端。
    // ProductModel::loadProductsFromServer 就需要 new Product(...)。

    // 假设你采取了后者：ProductModel 内部是 QList<Product*>
    // 并且 Product 类有 getName(), getMerchantUsername()
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

// productStockNotify(Product* productToUpdate)
// 这个方法在网络版中，如果ProductModel只管理QVariantMap，则Product*参数无意义。
// 如果ProductModel管理Product*实例，则可以尝试更新对应实例的stock。
// 但更简单的方式是，任何可能导致库存变化的操作（如支付成功）后，
// 都调用 loadProductsFromServer() 来保证数据最新。
bool ProductModel::productStockNotify(Product* productToUpdate) {
    Q_UNUSED(productToUpdate);
    // 在这种伪同步模式下，此函数主要用于兼容单机版接口。
    // 实际的库存更新应该通过重新从服务器加载数据来完成。
    // 或者，如果 OrderManager::payOrder 成功后，服务器返回了哪些商品库存变化了，
    // OrderManager 可以调用 ProductModel 的一个特定方法来更新这些商品的本地缓存（m_productsData）。
    // 最简单：什么都不做，依赖后续的 loadProductsFromServer()。
    qDebug() << "ProductModel::productStockNotify called. Stock will be updated on next full load from server.";
    return true; // 总是返回true，避免QML端逻辑中断
}

void ProductModel::copyImage(const QString& srcPath, const QString& destPath) {
    // 这个函数是本地文件操作，保持不变。
    // 但要注意，这个 destPath 如何被服务器使用是个问题。
    // 通常，图片需要上传到服务器，而不是服务器来读取客户端的本地路径。
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
