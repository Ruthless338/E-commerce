#include "serverproductmanager.h"
#include "filemanager.h" // 假设 FileManager.h/.cpp 已加入服务器工程
#include "product.h"
#include "book.h"
#include "clothing.h"
#include "food.h"
#include <QDebug>
#include <limits> // For std::numeric_limits

ServerProductManager::ServerProductManager(QObject *parent) : QObject(parent) {
    loadProductsFromFile();
}

ServerProductManager::~ServerProductManager() {
    qDeleteAll(m_allProducts);
    m_allProducts.clear();
}

void ServerProductManager::loadProductsFromFile() {
    qDeleteAll(m_allProducts);
    m_allProducts.clear();
    m_allProducts = FileManager::loadProducts(); // FileManager 负责加载具体品类折扣
    qInfo() << "ServerProductManager: Loaded" << m_allProducts.count() << "products from file.";
}

bool ServerProductManager::saveProductsToFile() {
    bool success = FileManager::saveProducts(m_allProducts);
    if (success) {
        qInfo() << "ServerProductManager: Products saved to file.";
    } else {
        qWarning() << "ServerProductManager: Failed to save products to file.";
    }
    return success;
}

QList<Product*> ServerProductManager::getAllProducts() {
    // 返回的是指针列表，注意生命周期管理。
    // ClientHandler 在序列化时应只读取数据，不获取所有权。
    return m_allProducts;
}

Product* ServerProductManager::findProductByNameAndMerchant(const QString& name, const QString& merchantUsername) {
    for (Product* p : m_allProducts) {
        if (p->getName() == name && p->getMerchantUsername() == merchantUsername) {
            return p;
        }
    }
    return nullptr;
}

QList<Product*> ServerProductManager::searchProducts(const QString &keyword, int searchType, double minPrice, double maxPrice) {
    QList<Product*> filtered;
    double mi = (minPrice < 0) ? 0 : minPrice; // 如果传入负数，则认为无下限
    double ma = (maxPrice < 0) ? std::numeric_limits<double>::max() : maxPrice; // 如果传入负数，则认为无上限

    for (Product *product : m_allProducts) {
        double currentPrice = product->getPrice(); // 获取打折后的价格
        if (currentPrice < mi || currentPrice > ma) continue;

        bool match = false;
        if (keyword.isEmpty()) { // 如果关键词为空，则只按价格筛选
            match = true;
        } else {
            switch (searchType) {
            case 0: // 名称
                match = product->getName().contains(keyword, Qt::CaseInsensitive);
                break;
            case 1: // 描述
                match = product->getDescription().contains(keyword, Qt::CaseInsensitive);
                break;
            // 可以添加按分类、按商家等搜索类型
            default:
                match = product->getName().contains(keyword, Qt::CaseInsensitive); // 默认按名称
                break;
            }
        }
        if (match) {
            filtered.append(product);
        }
    }
    return filtered;
}

bool ServerProductManager::addProduct(const QString& name, const QString& desc, double price, int stock,
                                      const QString& category, const QString& merchantUsername, const QString& imagePath) {
    // 检查商品是否已存在（同名同商家）
    if (findProductByNameAndMerchant(name, merchantUsername)) {
        qWarning() << "ServerProductManager: Product" << name << "by" << merchantUsername << "already exists.";
        return false;
    }

    Product *product = nullptr;
    if (category == "图书") {
        product = new Book(name, desc, price, stock, merchantUsername, imagePath);
    } else if (category == "服装") {
        product = new Clothing(name, desc, price, stock, merchantUsername, imagePath);
    } else if (category == "食品") {
        product = new Food(name, desc, price, stock, merchantUsername, imagePath);
    } else {
        qWarning() << "ServerProductManager: Unknown product category" << category;
        return false;
    }

    if (product) {
        m_allProducts.append(product);
        return saveProductsToFile();
    }
    return false;
}

bool ServerProductManager::updateProduct(const QString& originalProductName, const QString& merchantUsername,
                                         const QString& newName, const QString& newDescription,
                                         double newBasePrice, int newStock, const QString& newImagePath) {
    Product* product = findProductByNameAndMerchant(originalProductName, merchantUsername);
    if (!product) {
        qWarning() << "ServerProductManager: Product to update" << originalProductName << "by" << merchantUsername << "not found.";
        return false;
    }

    // 如果商品名称也改变了，要确保新名称没有冲突
    if (newName != originalProductName && findProductByNameAndMerchant(newName, merchantUsername)) {
        qWarning() << "ServerProductManager: New product name" << newName << "for merchant" << merchantUsername << "would cause a duplicate.";
        return false;
    }

    if (!newName.isEmpty()) product->setName(newName);
    if (!newDescription.isEmpty()) product->setDescription(newDescription);
    if (newBasePrice >= 0) product->setPrice(newBasePrice); // setPrice 设置的是 basePrice
    if (newStock >= 0) product->setStock(newStock);
    if (!newImagePath.isEmpty()) product->setImagePath(newImagePath);
    // merchantUsername 和 category 通常不在这里修改，或者需要更复杂的逻辑

    return saveProductsToFile();
}

void ServerProductManager::setCategoryDiscount(const QString& category, double discount) {
    if (discount < 0.0 || discount > 1.0) {
        qWarning() << "ServerProductManager: Invalid discount value" << discount << ". Must be between 0.0 and 1.0.";
        return;
    }

    bool changed = false;
    if (category == "图书") {
        if (qAbs(Book::discount - discount) > 0.001) { Book::discount = discount; changed = true; }
    } else if (category == "服装") {
        if (qAbs(Clothing::discount - discount) > 0.001) { Clothing::discount = discount; changed = true; }
    } else if (category == "食品") {
        if (qAbs(Food::discount - discount) > 0.001) { Food::discount = discount; changed = true; }
    } else {
        qWarning() << "ServerProductManager: Cannot set discount for unknown category" << category;
        return;
    }

    if (changed) {
        qInfo() << "ServerProductManager: Discount for category" << category << "set to" << discount;
        saveProductsToFile(); // FileManager::saveProducts 会保存 category discounts
    }
}

bool ServerProductManager::freezeStock(Product* product, int quantity) {
    if (!product || quantity <= 0) return false;
    if (product->getAvailableStock() < quantity) { // 使用 getAvailableStock 检查实际可用库存
        qWarning() << "ServerProductManager: Not enough available stock to freeze for" << product->getName() << ". Available:" << product->getAvailableStock() << "Requested:" << quantity;
        return false;
    }
    product->freezeStock(quantity); // Product 类需要有 freezeStock 方法
    qInfo() << "ServerProductManager: Froze" << quantity << "of" << product->getName() << ". Current stock:" << product->getStock() << "Frozen:" << product->getFrozenStock(); // Assuming product has frozenStock member
    // No need to save to file yet, only on confirm/release
    return true;
}

bool ServerProductManager::releaseFrozenStock(Product* product, int quantity) {
    if (!product || quantity <= 0) return false;
    product->releaseStock(quantity); // Product 类需要有 releaseStock 方法
    qInfo() << "ServerProductManager: Released" << quantity << "of" << product->getName() << ". Current stock:" << product->getStock() << "Frozen:" << product->getFrozenStock();
    // No need to save to file, as stock didn't change, only frozen count
    return true;
}

bool ServerProductManager::confirmStockDeduction(Product* product, int quantity) {
    if (!product || quantity <= 0) return false;
    if (product->getStock() < quantity) { // 再次检查总库存，理论上冻结时已保证
        qWarning() << "ServerProductManager: Stock became insufficient before confirmation for" << product->getName();
        return false;
    }
    product->deductStock(quantity);   // 实际减少库存
    product->releaseStock(quantity);  // 从冻结中移除这部分（因为已经扣减了）
    qInfo() << "ServerProductManager: Confirmed stock deduction for" << product->getName() << "by" << quantity << ". New stock:" << product->getStock();
    return saveProductsToFile(); // 持久化库存变化
}
