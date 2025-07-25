#ifndef SHOPPINGCART_H
#define SHOPPINGCART_H

#include <QObject>
#include <QVariantList> // 用于QML交互
#include <QVariantMap>  // 用于存储购物车项数据
#include <QJsonObject>


class ShoppingCart : public QObject {
    Q_OBJECT
    Q_PROPERTY(double totalPrice READ getTotalPrice NOTIFY totalPriceChanged)

public:
    explicit ShoppingCart(QObject *parent = nullptr);

    // QML调用的方法，保持与单机版一致
    Q_INVOKABLE bool addItem(const QString& productName, const QString& merchantUsername, int quantity = 1);
    Q_INVOKABLE bool removeItem(const QString& productName, const QString& merchantUsername);
    Q_INVOKABLE bool updateQuantity(const QString& productName, const QString& merchantUsername, int newQuantity);
    Q_INVOKABLE void clearCart(); // 清空本地和服务器购物车
    Q_INVOKABLE QVariantList getItems() const; // 返回购物车项列表给QML
    Q_INVOKABLE double getTotalPrice() const;  // 计算总价

    // 供 OrderManager 使用，获取当前购物车内容以创建订单
    QVariantList getCartItemsForOrder() const;

signals:
    void totalPriceChanged();
    void cartUpdated(bool success, const QString& message); // 通用更新信号

private:
    void loadCartFromServer(); // 从服务器加载购物车到 m_cartItems
    void syncCartWithServer();

    QList<QVariantMap> m_cartItems; // 存储购物车项 {productName, merchantUsername, quantity, price, itemTotalPrice, imagePath, ...}
    // price 是单个商品当前售价，itemTotalPrice = price * quantity
};

#endif // SHOPPINGCART_H
