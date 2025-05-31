#include "order.h"

Order::Order(const QString& consumerUsername,
             const QList<QPair<QString, QString>>& productIdentifiers,
             const QList<Product*>& allProducts,
             QObject* parent)
    : consumerUsername(consumerUsername),
    productIdentifiers(productIdentifiers),
    createTime(QDateTime::currentDateTime()),
    status(Pending),
    QObject(parent) {

    // 根据商品标识符查找对应的 Product 对象
    for (const auto& identifier : productIdentifiers) {
        QString productName = identifier.first;
        QString merchant = identifier.second;
        for (Product* product : allProducts) {
            if (product->getName() == productName &&
                product->getMerchantUsername() == merchant) {
                items[product]++; // 默认数量为1（需根据实际数据调整）
                break;
            }
        }
    }
}


bool Order::freezeStock() {
    QList<Product*> successfullyFrozen;
    for(auto it = items.begin(); it != items.end(); it++) {
        Product* product = it.key();
        int quantity = it.value();
        // 冻结库存数量不足，回滚操作
        if(product->getStock() < quantity) {
            for(Product* p : successfullyFrozen) {
                p->releaseStock(items.value(p));
            }
            return false;
        }
        // 冻结库存
        product->freezeStock(quantity);
        successfullyFrozen.append(product);
    }
    status = Pending;
    return true;
}

void Order::releaseStock() {
    for(auto it = items.begin(); it != items.end(); it++) {
        it.key()->releaseStock(it.value());
    }
    status = Cancelled;
}

void Order::confirmStock() {
    for(auto it = items.begin(); it != items.end(); it++) {
        Product* product = it.key();
        product->deductStock(it.value());
        product->releaseStock(it.value());
    }
    status = Paid;
}

// 计算订单总金额
double Order::calculateTotal() const {
    double total = 0;
    for(auto it = items.begin(); it != items.end(); it++) {
        total += it.key()->getPrice() * it.value();
    }
    return total;
}

int Order::getRemainingSeconds() const {
    if(status != Pending) return 0;
    QDateTime now = QDateTime::currentDateTime();
    int elapsed = createTime.secsTo(now);
    return qMax(300 - elapsed, 0); // 5分钟倒计时
}

QList<QPair<Product*, int>> Order::getItemPairs() const {
    QList<QPair<Product*, int>> pairs;
    for(auto it = items.begin(); it != items.end(); ++it) {
        pairs.append(qMakePair(it.key(), it.value()));
    }
    return pairs;
}

QList<QVariant> Order::getQmlItems() const {
    QList<QVariant> itemList;
    for (auto it = items.begin(); it != items.end(); ++it) {
        Product* product = it.key();
        int quantity = it.value();
        QVariantMap map;
        map["name"] = product->getName();
        map["description"] = product->getDescription();
        map["price"] = product->getPrice();
        map["quantity"] = quantity;
        map["imagePath"] = product->getImagePath();
        map["merchantUsername"] = product->getMerchantUsername();
        itemList.append(map);
    }
    return itemList;
}

QString Order::getStatusString() const {
    switch(status) {
    case Order::Pending: return "Pending";
    case Order::Paid: return "Paid";
    case Order::Cancelled: return "Cancelled";
    default: return "Unknown";
    }
}
