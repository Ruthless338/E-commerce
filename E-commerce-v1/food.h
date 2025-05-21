#ifndef FOOD_H
#define FOOD_H
#include <product.h>

class Food : public Product {
public:
    Food(const QString& n, const QString& desc, double price, int stk)
        : Product(n, desc, price, stk) {
        category = "食品";
    }
    // 由于要对某一品类下所有商品都打折，故discount应为静态变量
    static double discount;
    void setDiscount(double d) { discount = d; }
    double getDiscount() const { return discount; }
    double getPrice() const override { return basePrice*discount; }
};

#endif // FOOD_H
