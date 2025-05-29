// shoppingcart.h
#ifndef SHOPPINGCART_H
#define SHOPPINGCART_H

#include <QMap>
#include "product.h"
#include <QObject>
#include <QDebug>
#include <QVariant>
#include <QList>

class ShoppingCart : public QObject {
    Q_OBJECT
public:
    explicit ShoppingCart(QObject *parent = nullptr):QObject(parent) {}
    // 添加商品到购物车
    Q_INVOKABLE bool addItem(Product* product, int quantity);
    // 移除商品
    Q_INVOKABLE bool removeItem(Product* product);
    // 修改商品数量
    Q_INVOKABLE bool updateQuantity(Product* product, int newQuantity);
    Q_INVOKABLE bool updateQuantityByName(const QString& productName, int newQuantity);

    Q_INVOKABLE bool removeItemByName(const QString& productName);

    // 获取购物车总金额
    Q_INVOKABLE double getTotalPrice() const;
    // 获取购物车中所有商品及数量
    QMap<Product*, int> getItems() const { return items; }
    Q_INVOKABLE QList<QVariant> getCartItems() const;


    Q_INVOKABLE void loadShoppingCart(const QString& username);
    Q_INVOKABLE void saveShoppingCart(const QString& username);

signals:
    void cartChanged();

private:
    QMap<Product*, int> items; // 商品指针 -> 数量
};

#endif // SHOPPINGCART_H
