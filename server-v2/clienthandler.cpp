#include "clienthandler.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>


ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QObject(parent), m_socketDescriptor(socketDescriptor) {
}

ClientHandler::~ClientHandler() {}

void ClientHandler::process() {
    m_socket = new QTcpSocket(this);
    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qCritical() << "ClientHandler: Failed to set socket descriptor" << m_socketDescriptor << ":" << m_socket->errorString();
        emit finished();
        return;
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onSocketDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, &ClientHandler::onSocketError);

    qInfo() << "ClientHandler: Processing connection" << m_socketDescriptor << "on thread" << QThread::currentThreadId();
}

void ClientHandler::onReadyRead() {
    qDebug() << "Server CH(" << m_socketDescriptor << "): onReadyRead CALLED. Current buffer size:" << m_buffer.size();
    m_buffer.append(m_socket->readAll());
    qDebug() << "Server CH(" << m_socketDescriptor << "): Buffer after append, size:" << m_buffer.size() << "Content:" << m_buffer.constData(); // 打印原始数据
    // 使用换行符作为简单消息边界（客户端 NetworkClient 也应配合发送带换行符的请求）
    while(m_buffer.contains('\n')) {
        int newlinePos = m_buffer.indexOf('\n');
        QByteArray jsonData = m_buffer.left(newlinePos);
        m_buffer.remove(0, newlinePos + 1); // 移除已处理的消息和换行符

        if (jsonData.isEmpty()) continue; // 忽略空行

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            qDebug() << "Server RX (RawFileOp):" << doc.toJson(QJsonDocument::Compact);
            processMessage(doc.object());
        } else {
            qWarning() << "Server RX (RawFileOp): JSON Parse Error -" << parseError.errorString() << "Data:" << jsonData;
            // 可以选择发送一个错误响应给客户端
            QJsonObject errResponse;
            errResponse["response_to_action"] = "unknown_malformed";
            errResponse["status"] = "error";
            errResponse["message"] = "Malformed JSON request: " + parseError.errorString();
            sendResponse(errResponse);
        }
    }
    qDebug() << "Server CH(" << m_socketDescriptor << "): onReadyRead FINISHED. Remaining buffer size:" << m_buffer.size();
}


void ClientHandler::onSocketDisconnected() {
    qInfo() << "ClientHandler: Socket disconnected" << m_socketDescriptor;
    emit disconnectedFromClient(this);
    emit finished(); // Signal that this handler's work is done
}

void ClientHandler::onSocketError(QAbstractSocket::SocketError socketError) {
    qWarning() << "ClientHandler: Socket error on" << m_socketDescriptor << ":" << m_socket->errorString() << "(Code:" << socketError << ")";
    // emit disconnectedFromClient(this); // Server will handle removal
    // emit finished(); // Depending on error, may need to terminate
}

void ClientHandler::sendResponse(const QJsonObject& response) {
    if (m_socket && m_socket->isOpen() && m_socket->isWritable()) {
        // 将JSON对象转换为QByteArray
        QByteArray data = QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n"; // 使用 Compact 格式减少传输大小
        qint64 bytesWritten = m_socket->write(data);
        if (bytesWritten == -1) {
            qWarning() << "Server TX (AbsPath): Failed to write data to socket:" << m_socket->errorString();
        } else if (bytesWritten < data.size()) {
            qWarning() << "Server TX (AbsPath): Not all data written to socket. Wrote" << bytesWritten << "of" << data.size();
            // 这里可能需要处理未写完的数据，但对于简单场景，先忽略
        } else {
            qDebug() << "Server TX (AbsPath):" << data;
        }
        m_socket->flush(); // 确保数据被发送
    } else {
        qWarning() << "ClientHandler (AbsPath): Cannot send response, socket not valid, open, or writable.";
    }
}

void ClientHandler::processMessage(const QJsonObject& request) {
    qDebug() << "Server CH(" << m_socketDescriptor << "): processMessage CALLED with request:" << request;
    QString action = request["action"].toString();
    QJsonObject payload = request["payload"].toObject();
    QJsonObject responseDataSection; // 用于存放 handleXXX 返回的、将放入最终响应 "data" 字段的内容
    QString status = "success";
    QString message = "";

    qDebug() << "Server CH(" << m_socketDescriptor << "): Action:" << action << "Payload:" << payload;

    // 使用 try-catch 块来捕获可能的异常，这在文件操作中很重要
    try {
        if (action == "readFile") {
            qDebug() << "Server CH(" << m_socketDescriptor << "): Routing to handleReadFile.";
            responseDataSection = handleReadFile(payload);
        } else if (action == "writeFile") {
            qDebug() << "Server CH(" << m_socketDescriptor << "): Routing to handleWriteFile.";
            responseDataSection = handleWriteFile(payload);
        } else if (action == "fileExists") {
            qDebug() << "Server CH(" << m_socketDescriptor << "): Routing to handleFileExists.";
            responseDataSection = handleFileExists(payload);
        } else {
            qWarning() << "Server CH(" << m_socketDescriptor << "): Unknown file action:" << action;
            status = "error";
            message = "Unknown file action: " + action;
            // responseDataSection 保持为空或设置错误信息
            responseDataSection["message"] = message;
        }

        // 检查 handleXXX 是否内部标记了错误
        if (responseDataSection.contains("status") && responseDataSection["status"].toString() == "error") {
            status = "error";
            // 如果 handler 返回了错误，它的内容就是 message，而不是 data
            // 清除 responseDataSection，因为它是错误信息，不应放入最终的 "data" 字段
            // responseDataSection = QJsonObject(); // 或者确保不使用它作为data
        }

    } catch (const std::exception& e) {
        qCritical() << "Server CH(" << m_socketDescriptor << "): STANDARD EXCEPTION CAUGHT in processMessage for action" << action << ":" << e.what();
        status = "error";
        message = QString("Server دچار یک استثنای استاندارد شد: ") + e.what(); // "Server encountered a standard exception"
        // responseDataSection 保持为空或设置错误信息
        responseDataSection["message"] = message;
    } catch (...) {
        qCritical() << "Server CH(" << m_socketDescriptor << "): UNKNOWN EXCEPTION CAUGHT in processMessage for action" << action;
        status = "error";
        message = "سرور با یک استثنای ناشناخته مواجه شد."; // "Server encountered an unknown exception"
        // responseDataSection 保持为空或设置错误信息
        responseDataSection["message"] = message;
    }

    QJsonObject finalResponse;
    finalResponse["response_to_action"] = action;
    finalResponse["status"] = status;

    if (status == "success") {
        finalResponse["data"] = responseDataSection; // responseDataSection 就是要放入 "data" 的内容
    } else {
        finalResponse["message"] = message;
    }
    qDebug() << "Server CH(" << m_socketDescriptor << "): Sending final response:" << finalResponse;
    sendResponse(finalResponse);
    qDebug() << "Server CH(" << m_socketDescriptor << "): processMessage FINISHED.";
}

// --- 文件操作处理方法实现 ---
QJsonObject ClientHandler::handleReadFile(const QJsonObject& payload) {

    QString absoluteFilePath = payload["filePath"].toString();
    QJsonObject responseData; // 只包含数据或错误信息

    qDebug() << "Server CH(" << m_socketDescriptor << "): handleReadFile CALLED with filePath:" << absoluteFilePath;

    if (absoluteFilePath.isEmpty()) {
        responseData["status"] = "error";
        responseData["message"] = "File path is empty.";
        return responseData;
    }

    QFile file(absoluteFilePath);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray content = file.readAll();
        file.close();
        responseData["content"] = QString::fromUtf8(content); // 文件内容作为字符串
        // status 会在 processMessage 中被设置为 success (如果这里不设置error的话)
    } else {
        responseData["status"] = "error";
        responseData["message"] = "File not found or cannot be read: " + absoluteFilePath + ". Error: " + file.errorString();
    }
    return responseData; // 返回的是待放入 "data" 字段的内容，或者包含 "status" 和 "message" 的错误对象
}

QJsonObject ClientHandler::handleWriteFile(const QJsonObject& payload) {
    QString absoluteFilePath = payload["filePath"].toString();
    QString content = payload["content"].toString(); // 客户端传来的JSON字符串内容
    QJsonObject responseData;

    if (absoluteFilePath.isEmpty()) {
        responseData["status"] = "error";
        responseData["message"] = "File path is empty for writing.";
        return responseData;
    }

    QFile file(absoluteFilePath);
    // 确保目录存在 (对于绝对路径，这一步通常不是必须的，除非路径本身可能无效)
    // QFileInfo fileInfo(absoluteFilePath);
    // QDir dir = fileInfo.dir();
    // if (!dir.exists()) {
    //     if (!dir.mkpath(".")) {
    //         responseData["status"] = "error";
    //         responseData["message"] = "Failed to create directory for: " + absoluteFilePath;
    //         return responseData;
    //     }
    // }

    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) { // Truncate 会覆盖旧内容
        QTextStream out(&file);
        out << content;
        file.close();
        // status 默认为 success
    } else {
        responseData["status"] = "error";
        responseData["message"] = "Cannot write to file: " + absoluteFilePath + ". Error: " + file.errorString();
    }
    return responseData;
}

QJsonObject ClientHandler::handleFileExists(const QJsonObject& payload) {
    QString absoluteFilePath = payload["filePath"].toString();
    QJsonObject responseData;

    if (absoluteFilePath.isEmpty()) {
        responseData["status"] = "error";
        responseData["message"] = "File path is empty for existence check.";
        return responseData;
    }
    responseData["exists"] = QFile::exists(absoluteFilePath);
    // status 默认为 success
    return responseData;
}
