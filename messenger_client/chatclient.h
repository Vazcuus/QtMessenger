#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QSocketNotifier>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onUserInput();

private:
    QTcpSocket *m_socket;
    QSocketNotifier *m_notifier;
    bool m_isNameSet = false;
};

#endif // CLIENT_H
