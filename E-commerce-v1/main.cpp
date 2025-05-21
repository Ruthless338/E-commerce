#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include "user.h"
#include "filemanager.h"
#include "product.h"
#include "productmodel.h"
#include "globalstate.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    QQuickStyle::setStyle("Material");

    //--------------------- 注册C++类到QML上下文 ---------------------
    // 1. 注册User类为单例，供QML全局调用
    qmlRegisterSingletonType<User>("ECommerce.Core", 1, 0, "User",
                                   [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
                                       return new Consumer("", "", 0.0);
                                   }
                                   );

    // 2. 注册商品数据模型,将ProductModel实例暴露为全局属性
    ProductModel productModel;
    engine.rootContext()->setContextProperty("productModel", &productModel);

    // 3. 注册全局状态对象（用于传递用户登录状态）
    GlobalState *globalState = new GlobalState();
    // globalState->setProperty("username", "");
    // globalState->setProperty("userType", "");
    // globalState->setProperty("isConsumer", false);
    // globalState->setProperty("isMerchant", false);
    engine.rootContext()->setContextProperty("global", globalState);

    //--------------------- 加载QML主界面 ---------------------
    const QUrl mainWindowUrl(QStringLiteral("qrc:/qml/MainWindow.qml"));
    engine.load(mainWindowUrl);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "无法加载主窗口!";
        return -1;
    }


    return app.exec();
}



