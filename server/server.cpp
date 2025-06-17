#include "server.h"
#include "clienthandler.h" // To be created
// Include headers for server-side manager implementations (modified from your originals)
#include "serverauthmanager.h"
#include "serverproductmanager.h"
#include "servershoppingcartmanager.h"
#include "serverordermanager.h"
#include "filemanager.h" // Ensure FileManager paths are correct for server environment
#include <QThread>
#include <cstdio>
#include <stdio.h>
#include <QTimer>

Server::Server(QObject *parent) : QTcpServer(parent) {
    // Initialize server-side managers
    // These will use FileManager to interact with data files.
    m_authManager = new ServerAuthManager(this);
    m_productManager = new ServerProductManager(this);
    m_shoppingCartManager = new ServerShoppingCartManager(m_productManager, this);
    m_orderManager = new ServerOrderManager(m_productManager, m_authManager, m_shoppingCartManager, this);
    qInfo() << "Server initialized with managers.";
}

Server::~Server() {
    // m_clients list items are QObjects with parent, auto-deleted (or handle manually if moved to threads without parent)
    // Managers are parented to Server, auto-deleted.
}

bool Server::startServer(quint16 port) {
    if (!listen(QHostAddress::Any, port)) {
        qCritical() << "Server: Unable to start -" << errorString();
        return false;
    }
    qInfo() << "Server: Listening on port" << port;
    return true;
}

void Server::incomingConnection(qintptr socketDescriptor) {
    fprintf(stderr, "SERVER DEBUG: incomingConnection ENTERED with descriptor: %lld\n", static_cast<long long>(socketDescriptor));
    fflush(stderr);
    qDebug() << "\n=== 新连接到达 ===";
    qDebug() << "服务器：收到新连接，描述符：" << socketDescriptor;
    
    // Create a new ClientHandler for each connection
    // Pass manager instances to the handler
    ClientHandler *handler = new ClientHandler(socketDescriptor,
                                               m_authManager,
                                               m_productManager,
                                               m_shoppingCartManager,
                                               m_orderManager,
                                               this); // Parent to server initially

    // For multi-threading, move handler to a new QThread:
    QThread *thread = new QThread(this);
    handler->moveToThread(thread); // Move handler to the new thread

    connect(thread, &QThread::started, handler, &ClientHandler::process); // Call process when thread starts
    connect(handler, &ClientHandler::finished, thread, &QThread::quit); // When handler is done, quit thread
    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater); // Schedule handler for deletion
    connect(thread, &QThread::finished, thread, &QThread::deleteLater); // Schedule thread for deletion
    connect(handler, &ClientHandler::disconnectedFromClient, this, &Server::onClientDisconnected);

    m_clients.append(handler); // Keep track of active handlers
    QTimer::singleShot(100, handler, [handler]() {
        handler->process(); // 延迟启动处理逻辑
    });
    thread->start();
    qDebug() << "服务器：客户端处理器已在新线程中启动。当前活动连接数：" << m_clients.count() << "\n";
}

void Server::onClientDisconnected(ClientHandler* client) {
    removeClient(client);
    qInfo() << "连接断开，目前总连接数：" << m_clients.count();
}
void Server::removeClient(ClientHandler* client) {
    m_clients.removeAll(client);
}
