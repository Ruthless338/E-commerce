#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>


class ClientHandler : public QObject {
    Q_OBJECT
public:
    explicit ClientHandler(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientHandler();

public slots:
    void process();

signals:
    void finished();
    void disconnectedFromClient(ClientHandler* self);

private slots:
    void onReadyRead();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_socket = nullptr;
    qintptr m_socketDescriptor;
    QByteArray m_buffer;

    void processMessage(const QJsonObject& request);
    void sendResponse(const QJsonObject& response);

    QJsonObject handleReadFile(const QJsonObject& payload);
    QJsonObject handleWriteFile(const QJsonObject& payload);
    QJsonObject handleFileExists(const QJsonObject & payload);
};
#endif // CLIENTHANDLER_H
