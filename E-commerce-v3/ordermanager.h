#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <QObject>
#include <QVariantList> // 用于QML交互和订单项
#include <QVariantMap>  // 用于单个订单数据
#include <QJsonObject>
#include "shoppingcart.h" // 可能需要 ShoppingCart* 来获取购物车项

class OrderManager : public QObject {
    Q_OBJECT

public:
    explicit OrderManager(QObject *parent = nullptr);
    void setCartInstance(ShoppingCart* cart); // 依赖注入

    // QML调用的方法
    // 1. 从购物车生成订单预览/准备订单
    Q_INVOKABLE QVariantMap prepareOrderFromCart(); // 返回订单预览信息 (含总价、商品列表等)
    // 2. 支付订单
    Q_INVOKABLE bool payOrder(const QString& orderId); // 支付指定ID的订单
    // 3. 获取用户历史订单
    Q_INVOKABLE QVariantList getUserOrders();

signals:
    void orderPrepared(bool success, const QVariantMap& orderData, const QString& message);
    void orderPaid(bool success, const QString& orderId, const QString& message);
    void ordersLoaded(bool success, const QVariantList& ordersData, const QString& message);
    void paymentError(const QString& message); // 特定于支付的错误
    void stockPossiblyChanged(); // 通知ProductModel可能需要刷新

private:
    // QList<QVariantMap> m_userOrders; // 本地缓存的用户历史订单 (可选)
    ShoppingCart* m_shoppingCart; // 指向购物车实例
};

#endif // ORDERMANAGER_H
