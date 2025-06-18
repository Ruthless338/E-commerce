// main.cpp (客户端)
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "networkclient.h"
#include "authmanager.h"
#include "productmodel.h"
#include "shoppingcart.h"
#include "ordermanager.h"
#include "globalstate.h"
#include <QQuickStyle>

// 全局实例指针，供 AuthManager 等内部使用
GlobalState* globalStateInstance = nullptr;

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    QQuickStyle::setStyle("Material");

    // 1. 初始化 NetworkClient 并尝试连接
    NetworkClient::instance()->connectToServer("localhost", 8080);

    // 2. 初始化 GlobalState
    globalStateInstance = new GlobalState(); // 其他C++类将通过此指针更新它
    engine.rootContext()->setContextProperty("global", globalStateInstance);


    // 3. 注册其他管理器 - QML通过这些名字调用方法
    // AuthManager 主要通过静态方法被QML调用，如果QML中没有AuthManager.xxx的用法，则不需要注册
    // 但如果AuthManager内部的 sendRequestAndWait 需要 AuthManager 实例来 connect 信号，
    // 那么 AuthManager 不能只有静态方法，或者 sendRequestAndWait 需要更复杂的信号处理。
    // 我们之前的 AuthManager::sendRequestAndWait 是静态的，它直接用 NetworkClient::instance()。
    // 所以 AuthManager 的注册可能不是必须的，除非QML直接创建或引用 AuthManager 实例。
    // 你的单机版如果 User 类是 qmlRegisterSingletonType<User>，AuthManager的方法可能在User类里。
    // 我们这里假设QML直接调用 AuthManager::verifyLogin 等。
    qmlRegisterSingletonType<AuthManager>("ECommerce.Core", 1, 0, "AuthManager",
                                          [](QQmlEngine*, QJSEngine*) -> QObject* {
                                              // 但如果 User 类的方法现在在 AuthManager 中，QML 可能需要一个 AuthManager "对象"
                                              return new AuthManager();
                                          });


    ProductModel *productModel = new ProductModel();
    engine.rootContext()->setContextProperty("productModel", productModel);

    ShoppingCart *shoppingCart = new ShoppingCart();
    engine.rootContext()->setContextProperty("shoppingCart", shoppingCart);
    qmlRegisterSingletonType<ShoppingCart>(
        "ECommerce.Core", 1, 0, "ShoppingCart",
        [shoppingCart](QQmlEngine*, QJSEngine*) -> QObject* { return shoppingCart; });


    OrderManager *orderManager = new OrderManager(); // 内部操作也变为网络请求
    qmlRegisterSingletonType<OrderManager>(
        "ECommerce.Core", 1, 0, "OrderManager",
        [orderManager](QQmlEngine*, QJSEngine*) -> QObject* { return orderManager; });

    // User 类 (如果之前是单例，现在其功能大部分移到 AuthManager，或由 GlobalState 体现)
    // 如果QML中还有 User.someStaticMethod() 的调用，你需要将这些方法移到AuthManager或类似地方。
    // qmlRegisterSingletonType<User>(...); // 可能不再需要，或替换为 AuthManager


    // const QUrl url(QStringLiteral("qrc:/qml/MainWindow.qml")); // 你的主QML文件
    // QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
    //                  &app, [url](QObject *obj, const QUrl &objUrl) {
    //                      if (!obj && url == objUrl)
    //                          QCoreApplication::exit(-1);
    //                  }, Qt::QueuedConnection);
    // engine.load(url);

    int ret = app.exec();
    delete globalStateInstance; // 清理
    return ret;
}
