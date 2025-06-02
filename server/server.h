#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QList>
// Forward declare managers that will live on the server
class ServerAuthManager;
class ServerProductManager;
class ServerShoppingCartManager;
class ServerOrderManager;
class ClientHandler; // Handles individual client connections

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();
    bool startServer(quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientDisconnected(ClientHandler* client);

private:
    QList<ClientHandler*> m_clients;
    // Server-side instances of your managers
    ServerAuthManager* m_authManager;
    ServerProductManager* m_productManager;
    ServerShoppingCartManager* m_shoppingCartManager;
    ServerOrderManager* m_orderManager;

    void removeClient(ClientHandler* client);
};

#endif // SERVER_H
