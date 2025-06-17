#include <QCoreApplication>
#include "server.h" // To be created

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    Server server;
    quint16 port = 8080;
    if (!server.startServer(port)) {
        qCritical() << "Server could not start on port" << port;
        return -1;
    }
    qInfo() << "Server started on port" << port;
    return a.exec();
}
