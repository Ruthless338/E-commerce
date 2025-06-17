#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QList>
class ClientHandler;

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
    void removeClient(ClientHandler* client);
};

#endif // SERVER_H
