#include "server.h"
#include "clienthandler.h"
#include <QThread>

Server::Server(QObject *parent) : QTcpServer(parent) {
    qInfo() << "Server initialized.";
}

Server::~Server() {}

bool Server::startServer(quint16 port) {
    if (!listen(QHostAddress::LocalHost, port)) {
        qCritical() << "Server: Unable to start -" << errorString();
        return false;
    }
    qInfo() << "Server: Listening on port" << port;
    return true;
}

void Server::incomingConnection(qintptr socketDescriptor) {
    qCritical() << "Server: Incoming connection with descriptor" << socketDescriptor;
    // Create a new ClientHandler for each connection
    // Pass manager instances to the handler

    ClientHandler *handler = new ClientHandler(socketDescriptor,
                                               this);

    // For multi-threading, move handler to a new QThread:
    QThread *thread = new QThread();
    handler->moveToThread(thread); // Move handler to the new thread

    connect(thread, &QThread::started, handler, &ClientHandler::process); // Call process when thread starts
    connect(handler, &ClientHandler::finished, thread, &QThread::quit); // When handler is done, quit thread
    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater); // Schedule handler for deletion
    connect(thread, &QThread::finished, thread, &QThread::deleteLater); // Schedule thread for deletion
    connect(handler, &ClientHandler::disconnectedFromClient, this, &Server::onClientDisconnected);


    m_clients.append(handler); // Keep track of active handlers
    thread->start();
    qInfo() << "Server: Client handler started in a new thread. Total active handlers:" << m_clients.count();
}

void Server::onClientDisconnected(ClientHandler* client) {
    removeClient(client);
    qInfo() << "连接断开，目前总连接数：" << m_clients.count();
}
void Server::removeClient(ClientHandler* client) {
    m_clients.removeAll(client);
}
