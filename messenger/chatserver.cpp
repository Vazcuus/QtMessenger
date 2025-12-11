#include "chatserver.h"
#include <QDebug>
#include <QCoreApplication>
#include <QStringList>

Server::Server(QObject *parent) : QObject(parent)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, 1234)) {
        qDebug() << "Server could not start!";
        qApp->quit();
    } else {
        qDebug() << "Server started on port 1234. Waiting for connections...";
    }
}

void Server::onNewConnection()
{
    QTcpSocket *clientSocket = m_server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onDisconnected);

    m_clients.insert(clientSocket, ClientInfo()); // Вставляем пустую информацию
    qDebug() << "New client connecting from:" << clientSocket->peerAddress().toString();
    sendToClient(clientSocket, "Welcome! Please enter your name:");
}

void Server::onReadyRead()
{
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    QString text = QString::fromUtf8(senderSocket->readAll().trimmed());
    ClientInfo &clientInfo = m_clients[senderSocket]; // Получаем ссылку для удобства

    if (clientInfo.name.isEmpty()) {
        // 1. Первое сообщение - это имя
        clientInfo.name = text;
        QString welcomeMessage = QString("Hello, %1! ").arg(text);
        QString roomList = getRoomList().join(", ");
        if (roomList.isEmpty()) {
            welcomeMessage += "No rooms active. Enter a name to create one:";
        } else {
            welcomeMessage += QString("Active rooms: [%1].\nEnter a room name to join or create one:").arg(roomList);
        }
        sendToClient(senderSocket, welcomeMessage);
    } else if (clientInfo.room.isEmpty()) {
        // 2. Второе сообщение - это комната
        joinRoom(senderSocket, text);
    } else {
        // 3. Все последующие сообщения - команды или чат
        if (text.startsWith('/')) {
            handleCommand(senderSocket, text);
        } else {
            QString message = QString("%1: %2").arg(clientInfo.name, text);
            broadcastToRoom(clientInfo.room, message, senderSocket);
        }
    }
}

void Server::onDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    ClientInfo clientInfo = m_clients.value(clientSocket);
    m_clients.remove(clientSocket);
    clientSocket->deleteLater();

    if (!clientInfo.name.isEmpty() && !clientInfo.room.isEmpty()) {
        QString leaveMessage = QString("[%1 has left the room]").arg(clientInfo.name);
        qDebug() << "Room" << clientInfo.room << ":" << leaveMessage;
        broadcastToRoom(clientInfo.room, leaveMessage);
    }
}

void Server::joinRoom(QTcpSocket *clientSocket, const QString &roomName)
{
    ClientInfo &clientInfo = m_clients[clientSocket];
    QString oldRoom = clientInfo.room;

    // Если пользователь уже был в комнате, уведомить ее участников
    if (!oldRoom.isEmpty()) {
        broadcastToRoom(oldRoom, QString("[%1 has left the room]").arg(clientInfo.name));
    }

    clientInfo.room = roomName;

    QString joinMessage = QString("[%1 has joined the room]").arg(clientInfo.name);
    broadcastToRoom(roomName, joinMessage);
    qDebug() << clientInfo.name << "joined room" << roomName;

    sendToClient(clientSocket, QString("You have joined room '%1'.").arg(roomName));
    QString usersInRoom = getUsersInRoom(roomName).join(", ");
    sendToClient(clientSocket, QString("Users in this room: %1").arg(usersInRoom));
}

void Server::handleCommand(QTcpSocket *clientSocket, const QString &text)
{
    QStringList parts = text.split(' ');
    const QString command = parts.first();
    ClientInfo &clientInfo = m_clients[clientSocket];

    if (command == "/join" && parts.size() > 1) {
        joinRoom(clientSocket, parts.at(1));
    } else if (command == "/list") {
        QString roomList = getRoomList().join(", ");
        if (roomList.isEmpty()) {
            sendToClient(clientSocket, "No active rooms.");
        } else {
            sendToClient(clientSocket, QString("Active rooms: [%1]").arg(roomList));
        }
    } else if (command == "/who") {
        QString usersInRoom = getUsersInRoom(clientInfo.room).join(", ");
        sendToClient(clientSocket, QString("Users in this room: %1").arg(usersInRoom));
    } else {
        sendToClient(clientSocket, QString("Unknown command: %1").arg(command));
    }
}

void Server::broadcastToRoom(const QString &roomName, const QString &message, QTcpSocket *excludeClient)
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.key() != excludeClient && it.value().room == roomName) {
            sendToClient(it.key(), message);
        }
    }
}

void Server::sendToClient(QTcpSocket *clientSocket, const QString &message)
{
    clientSocket->write((message + "\n").toUtf8());
}

QStringList Server::getRoomList()
{
    QSet<QString> rooms;
    for (const ClientInfo &info : m_clients.values()) {
        if (!info.room.isEmpty()) {
            rooms.insert(info.room);
        }
    }
    return rooms.values();
}

QStringList Server::getUsersInRoom(const QString &roomName)
{
    QStringList users;
    for (const ClientInfo &info : m_clients.values()) {
        if (info.room == roomName) {
            users << info.name;
        }
    }
    return users;
}
