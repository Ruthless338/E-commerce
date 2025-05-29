#include "order.h"

Order::Order(const QString& consumerUsername,
             const QList<QPair<QString, QString>>& productIdentifiers,
             const QList<Product*>& allProducts)
    : consumerUsername(consumerUsername),
    productIdentifiers(productIdentifiers),
    createTime(QDateTime::currentDateTime()),
    status(Pending) {

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
    for(auto it = items.begin(); it != items.end(); it++) {
        Product* product = it.key();
        // 冻结库存
        if(product->getStock() < it.value()) {
            return false;
        }
        product->freezeStock(it.value());
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

