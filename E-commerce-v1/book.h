#ifndef BOOK_H
#define BOOK_H
#include <product.h>

class Book : public Product {

public:
    static double discount;
    Book(const QString& n, const QString& desc, double price, int stk)
        : Product(n, desc, price, stk) {
        category = "图书";
    }
    // 由于要对某一品类下所有商品都打折，故discount应为静态变量

    void setDiscount(double d) { discount = d; }
    double getDiscount() const { return discount; }

    double getPrice() const override { return basePrice*discount; }
};

#endif // BOOK_H
