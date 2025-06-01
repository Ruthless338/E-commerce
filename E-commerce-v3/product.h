#ifndef PRODUCT_H
#define PRODUCT_H
#include <QString>
#include <QObject>

class Product {
protected:
    QString name;
    QString description;
    double basePrice;
    int stock;
    int frozenStock;
    QString category;
    QString imagePath;
    QString merchantUsername;

public:
    Product(const QString& n,
            const QString& desc,
            double price,
            int stk,
            const QString& merchantUsername,
            const QString& imgPath = ""):
        name(n),
        description(desc),
        basePrice(price),
        stock(stk),
        merchantUsername(merchantUsername),
        imagePath(imgPath){}

    virtual ~Product() = default;

    double getBasePrice() const { return basePrice; }
    virtual double getPrice() const = 0;
    QString getName() const { return name; }
    int getStock() const { return stock; }
    QString getDescription() const { return description; }
    QString getCategory() const { return category; }
    QString getImagePath() const { return imagePath; }
    virtual double getDiscount() const { return 0.0; }
    QString getMerchantUsername() const { return merchantUsername; }
    int getFrozenStock() const { return frozenStock; }

    void setPrice(double p) { basePrice = p; }
    void setDescription(const QString &d) { description = d; }
    void setName(const QString &n) { name = n; }
    void setStock(int s) { stock = s; }
    void setImagePath(const QString& path) { imagePath = path; }
    void setMerchantUsername(const QString& username) { merchantUsername = username; }

    void freezeStock(int quantity) { frozenStock += quantity; }
    void releaseStock(int quantity) { frozenStock -= quantity; }
    int getAvailableStock() const { return stock - frozenStock; }
    void deductStock(int quantity) { stock -= quantity; }
};



#endif // PRODUCT_H
