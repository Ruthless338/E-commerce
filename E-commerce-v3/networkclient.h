#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

class NetworkClient : public QObject {
    Q_OBJECT
public:
    static NetworkClient* instance(); // Singleton
    ~NetworkClient();

    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;
    void sendRequest(const QJsonObject& request); // 发送请求

signals:
    void connected();
    void disconnected();
    void errorOccurred(QAbstractSocket::SocketError socketError, const QString& errorString);
    void responseReceived(const QJsonObject& response); // 接收到响应时发射

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    explicit NetworkClient(QObject *parent = nullptr);
    QTcpSocket *m_socket;
    static NetworkClient* m_pInstance;
    QByteArray m_buffer;
};
#endif // NETWORKCLIENT_H
