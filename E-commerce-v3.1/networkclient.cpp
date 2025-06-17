#include "networkclient.h"
#include <QHostAddress>
#include <QJsonParseError>
#include <QDebug>

NetworkClient* NetworkClient::m_pInstance = nullptr;

NetworkClient* NetworkClient::instance() {
    if (!m_pInstance)
        m_pInstance = new NetworkClient();
    return m_pInstance;
}

NetworkClient::NetworkClient(QObject *parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onSocketReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, &NetworkClient::onSocketError);
}

NetworkClient::~NetworkClient() {
    // m_pInstance will be nullified if this is the instance, or handled by app exit
}

bool NetworkClient::connectToServer(const QString& host, quint16 port) {
    qDebug() << "NetworkClient::connectToServer - Attempting to connect to host:" << host << "port:" << port; // 关键日志
    if (m_socket->state() == QAbstractSocket::UnconnectedState || m_socket->state() == QAbstractSocket::ClosingState) {
        qInfo() << "NetworkClient: Socket state is Unconnected or Closing. Attempting connectToHost.";
        m_socket->connectToHost(QHostAddress(host), port);
    } else {
        qWarning() << "NetworkClient: Socket is already connected or in a connecting state. State:" << m_socket->state();
        return isConnected(); // 如果已经连接，直接返回状态
    }

    if (m_socket->waitForConnected(5000)) { // 增加超时时间，并检查返回值
        qInfo() << "NetworkClient: waitForConnected successful!";
        // onSocketConnected() 应该会被自动调用，并发出 connected() 信号
        return true;
    } else {
        qWarning() << "NetworkClient: waitForConnected FAILED. Socket error:" << m_socket->error() << m_socket->errorString();
        // onSocketError() 应该会被调用
        return false;
    }
}

void NetworkClient::disconnectFromServer() {
    if (m_socket->isOpen()) m_socket->disconnectFromHost();
}

bool NetworkClient::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::sendRequest(const QJsonObject& request) {
    if (!isConnected()) {
        qWarning() << "NetworkClient: Not connected. Cannot send request:" << request["action"].toString();
        // In pseudo-sync, this error should be propagated back by sendRequestAndWait
        return;
    }
    QByteArray data = QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n"; // Add newline as simple delimiter
    m_socket->write(data);
    m_socket->flush(); // Ensure data is sent
    qDebug() << "NetworkClient TX:" << data.trimmed();
}

void NetworkClient::onSocketConnected() {
    qInfo() << "NetworkClient: Connected to server.";
    emit connected();
}

void NetworkClient::onSocketDisconnected() {
    qInfo() << "NetworkClient: Disconnected from server.";
    emit disconnected();
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError socketError) {
    qWarning() << "NetworkClient: Socket error:" << m_socket->errorString();
    emit errorOccurred(socketError, m_socket->errorString());
}

void NetworkClient::onSocketReadyRead() {
    m_buffer.append(m_socket->readAll());
    // Simple delimiter-based framing (newline)
    // This is still a simplification. Robust framing uses length prefixes.
    while (true) {
        int newlinePos = m_buffer.indexOf('\n');
        if (newlinePos == -1) {
            break; // No complete message yet
        }

        QByteArray jsonData = m_buffer.left(newlinePos);
        m_buffer = m_buffer.mid(newlinePos + 1); // Remove processed message (and newline)

        if (jsonData.isEmpty()) continue; // Skip empty lines if any

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);

        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            qDebug() << "NetworkClient RX:" << doc.toJson(QJsonDocument::Compact);
            emit responseReceived(doc.object());
        } else {
            qWarning() << "NetworkClient: JSON parse error:" << error.errorString() << "Data:" << jsonData;
            // Optionally emit an error signal here too
        }
    }
}
