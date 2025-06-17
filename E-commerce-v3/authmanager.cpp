#include "authmanager.h"
#include "networkclient.h"
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QDebug>
#include <QUuid> // 用于生成唯一的请求ID

// 全局的 GlobalState 实例指针，假设在 main.cpp 中创建并可以这样访问
// 或者通过一个 getter 函数获取。为了简单，这里假设可以直接访问。
// 更好的做法是在 AuthManager 的方法中接收 GlobalState* 作为参数，或者 AuthManager 自身持有 GlobalState 实例。
// 为了严格不改QML，我们假设GlobalState的更新是由C++内部完成的。
extern GlobalState* globalStateInstance; // 假设在main.cpp定义和初始化

// 辅助函数：发送请求并等待响应
QJsonObject AuthManager::sendRequestAndWait(const QJsonObject& requestData, int timeoutMs) {
    if (!NetworkClient::instance()->isConnected()) {
        qWarning() << "AuthManager: Not connected to server.";
        // 返回一个QML能识别的错误格式，或者让调用者处理连接错误
        return QJsonObject{{"status", "error"}, {"message", "Not connected to server"}};
    }

    QJsonObject mutableRequestData = requestData; // 创建一个可修改的副本
    QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    mutableRequestData["requestId"] = requestId; // 为每个请求添加唯一ID

    QJsonObject responseJson;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QMetaObject::Connection conn = QObject::connect(NetworkClient::instance(), &NetworkClient::responseReceived,
                                                    [&](const QJsonObject& res) {
                                                        // 检查响应是否包含requestId并且与我们发送的匹配
                                                        if (res.value("requestId").toString() == requestId) {
                                                            responseJson = res;
                                                            if (timer.isActive()) timer.stop();
                                                            loop.quit();
                                                        }
                                                    });

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        qWarning() << "AuthManager: Request timeout for action" << mutableRequestData.value("action").toString() << "ID:" << requestId;
        // 返回一个QML能识别的错误格式
        responseJson = QJsonObject{{"status", "error"}, {"message", "Request timed out"}};
        loop.quit();
    });

    NetworkClient::instance()->sendRequest(mutableRequestData); // 发送带有requestId的请求
    timer.start(timeoutMs);
    loop.exec();

    QObject::disconnect(conn);
    return responseJson;
}


bool AuthManager::verifyLogin(const QString &username, const QString &password) {
    QJsonObject request;
    request["action"] = "login";
    QJsonObject payload;
    payload["username"] = username;
    payload["password"] = password;
    request["payload"] = payload;

    QJsonObject response = sendRequestAndWait(request);

    if (response["status"].toString() == "success") {
        QJsonObject userData = response["data"].toObject();
        if (globalStateInstance) { // 确保 globalStateInstance 已初始化
            globalStateInstance->setUsername(userData["username"].toString());
            globalStateInstance->setUserType(userData["type"].toString());
            globalStateInstance->setBalance(userData["balance"].toDouble());
            globalStateInstance->setIsConsumer(userData["type"].toString() == "Consumer");
            globalStateInstance->setIsMerchant(userData["type"].toString() == "Merchant");
        }
        return true;
    } else {
        qDebug() << "Login failed:" << response["message"].toString();
        if (globalStateInstance) { // 登录失败，清空全局状态
            globalStateInstance->logout(); // GlobalState 需要一个 logout 方法
        }
        return false;
    }
}

QVariantMap AuthManager::registerUser(const QString &username, const QString &pwd, const QString &type) {
    QJsonObject request;
    request["action"] = "register";
    QJsonObject payload;
    payload["username"] = username;
    payload["password"] = pwd;
    payload["type"] = type;
    // balance 由服务器端处理，客户端不应指定初始余额
    request["payload"] = payload;

    QJsonObject response = sendRequestAndWait(request);
    QVariantMap result; // QML 期望的返回类型
    if (response["status"].toString() == "success") {
        result["success"] = true;
        // 注册成功后，可以考虑自动登录，或者提示用户手动登录
        // 如果自动登录，则需要像 verifyLogin 一样更新 globalStateInstance
    } else {
        result["success"] = false;
        result["error"] = response["message"].toString();
    }
    return result;
}

bool AuthManager::changePassword(const QString &username, const QString &oldPwd, const QString &newPwd) {
    // 注意：单机版中 username 参数可能没有，因为是当前登录用户。
    // 网络版中，服务器端会使用 ClientHandler 中 m_loggedInUsername。
    QJsonObject request;
    request["action"] = "changePassword";
    QJsonObject payload;
    // payload["username"] = username; // 服务器会用会话中的用户名
    payload["oldPwd"] = oldPwd;
    payload["newPwd"] = newPwd;
    request["payload"] = payload;

    QJsonObject response = sendRequestAndWait(request);
    return response["status"].toString() == "success";
}

bool AuthManager::recharge(const QString& username, double amount) {
    // username 参数在这里可能不需要，服务器用会话中的用户
    QJsonObject request;
    request["action"] = "recharge";
    QJsonObject payload;
    // payload["username"] = username;
    payload["amount"] = amount;
    request["payload"] = payload;

    QJsonObject response = sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        if (globalStateInstance) {
            QJsonObject data = response["data"].toObject();
            globalStateInstance->setBalance(data["newBalance"].toDouble());
        }
        return true;
    }
    return false;
}

double AuthManager::getBalance(const QString& username) {
    // username 参数在这里可能不需要，服务器用会话中的用户
    QJsonObject request;
    request["action"] = "getBalance";
    QJsonObject payload;
    // payload["username"] = username; // 服务器可以用当前会话用户
    request["payload"] = payload;

    QJsonObject response = sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        double balance = response["data"].toObject()["balance"].toDouble();
        if (globalStateInstance && globalStateInstance->username() == username) { // 确保是当前用户的余额
            globalStateInstance->setBalance(balance);
        }
        return balance;
    }
    return 0.0; // 或一个错误指示值
}

// deductBalance, addBalance, getUserType 类似实现
bool AuthManager::deductBalance(const QString& username, double amount) {
    QJsonObject request;
    request["action"] = "deductBalance"; // 假设服务器有此action
    QJsonObject payload;
    // payload["username"] = username; // 服务器用会话用户
    payload["amount"] = amount;
    request["payload"] = payload;

    QJsonObject response = sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        if (globalStateInstance && globalStateInstance->username() == username) {
            // 服务器应在响应中返回新的余额
            QJsonObject data = response["data"].toObject();
            if (data.contains("newBalance")) {
                globalStateInstance->setBalance(data["newBalance"].toDouble());
            } else { // 如果服务器没返回新余额，则重新查询
                getBalance(username);
            }
        }
        return true;
    }
    return false;
}

bool AuthManager::addBalance(const QString& username, double amount) {
    QJsonObject request;
    request["action"] = "addBalance"; // 假设服务器有此action
    QJsonObject payload;
    // payload["username"] = username; // 服务器用会话用户
    payload["amount"] = amount;
    request["payload"] = payload;

    QJsonObject response = sendRequestAndWait(request);
    if (response["status"].toString() == "success") {
        if (globalStateInstance && globalStateInstance->username() == username) {
            QJsonObject data = response["data"].toObject();
            if (data.contains("newBalance")) {
                globalStateInstance->setBalance(data["newBalance"].toDouble());
            } else {
                getBalance(username);
            }
        }
        return true;
    }
    return false;
}

QString AuthManager::getUserType(const QString& username) {
    // 这个信息通常在登录时就获取并存储在 GlobalState 了
    // 如果非要单独请求，服务器需要提供接口
    // Q_UNUSED(username); // username 参数可能无用，用会话中的
    if (globalStateInstance && globalStateInstance->username() == username) {
        return globalStateInstance->userType();
    }
    // 或者发送一个 "getUserInfo" 请求到服务器
    // QJsonObject request; request["action"] = "getUserInfo"; ...
    // QJsonObject response = sendRequestAndWait(request); ...
    // return response["data"].toObject()["type"].toString();
    return ""; // 默认或错误
}
