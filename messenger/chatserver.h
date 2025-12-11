#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>

// Структура для хранения информации о клиенте
struct ClientInfo {
    QString name;
    QString room;
};

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void handleCommand(QTcpSocket *clientSocket, const QString &text);
    void joinRoom(QTcpSocket *clientSocket, const QString &roomName);
    void broadcastToRoom(const QString &roomName, const QString &message, QTcpSocket *excludeClient = nullptr);
    void sendToClient(QTcpSocket *clientSocket, const QString &message);
    QStringList getRoomList();
    QStringList getUsersInRoom(const QString &roomName);

    QTcpServer *m_server;
    // Теперь карта хранит сокет и структуру с информацией о клиенте
    QMap<QTcpSocket*, ClientInfo> m_clients;
};

#endif // CHATSERVER_H
