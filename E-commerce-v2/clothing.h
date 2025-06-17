#ifndef CLOTHING_H
#define CLOTHING_H
#include <product.h>

class Clothing : public Product {
public:
    Clothing(const QString& n, const QString& desc, double price, int stk, const QString& merchantUsername, const QString& imagePath, int frozenStock = 0)
        : Product(n, desc, price, stk, merchantUsername, imagePath, frozenStock) {
        category = "服装";
    }
    // 由于要对某一品类下所有商品都打折，故discount应为静态变量
    static double discount;
    void setDiscount(double d) { discount = d; }
    double getDiscount() const override { return discount; }
    double getPrice() const override { return basePrice*discount; }
};


#endif // CLOTHING_H
