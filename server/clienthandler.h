#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class ServerAuthManager;
class ServerProductManager;
class ServerShoppingCartManager;
class ServerOrderManager;

class ClientHandler : public QObject {
    Q_OBJECT
public:
    explicit ClientHandler(qintptr socketDescriptor,
                           ServerAuthManager* authMgr,
                           ServerProductManager* prodMgr,
                           ServerShoppingCartManager* cartMgr,
                           ServerOrderManager* orderMgr,
                           QObject *parent = nullptr); // Parent will be null when moved to thread
    ~ClientHandler();

public slots:
    void process(); // Main processing loop/setup in thread

signals:
    void finished(); // To signal thread to quit
    void disconnectedFromClient(ClientHandler* self); // To notify server
private slots:
    void onReadyRead();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_socket = nullptr;
    qintptr m_socketDescriptor;
    QString m_loggedInUsername; // Track user for this connection

    // Pointers to server-wide manager instances
    ServerAuthManager* m_authManager_s;
    ServerProductManager* m_productManager_s;
    ServerShoppingCartManager* m_shoppingCartManager_s;
    ServerOrderManager* m_orderManager_s;

    QByteArray m_buffer; // Buffer for incoming data

    void processMessage(const QJsonObject& request);
    void sendResponse(const QJsonObject& response);

    QJsonObject handleLogin(const QJsonObject& payload);
    QJsonObject handleRegister(const QJsonObject& payload);
    QJsonObject handleChangePassword(const QJsonObject& payload);
    QJsonObject handleRecharge(const QJsonObject& payload);
    QJsonObject handleGetBalance(const QJsonObject& payload);

    QJsonObject handleGetProducts(const QJsonObject& payload);
    QJsonObject handleSearchProducts(const QJsonObject& payload);
    QJsonObject handleAddProduct(const QJsonObject& payload);
    QJsonObject handleUpdateProduct(const QJsonObject& payload);
    QJsonObject handleSetCategoryDiscount(const QJsonObject& payload);

    QJsonObject handleGetCart(const QJsonObject& payload);
    QJsonObject handleAddToCart(const QJsonObject& payload);
    QJsonObject handleRemoveFromCart(const QJsonObject& payload);
    QJsonObject handleUpdateCartQuantity(const QJsonObject& payload);

    QJsonObject handlePrepareOrder(const QJsonObject& payload);
    QJsonObject handlePayOrder(const QJsonObject& payload);
    QJsonObject handleGetOrders(const QJsonObject& payload);
};

#endif // CLIENTHANDLER_H
