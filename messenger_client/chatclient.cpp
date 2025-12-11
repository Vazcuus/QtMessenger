#include "chatclient.h"
#include <QDebug>
#include <QTextStream>
#include <QCoreApplication>
#include <unistd.h> // for STDIN_FILENO

Client::Client(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);

    qDebug() << "Connecting to server...";
    m_socket->connectToHost("127.0.0.1", 1234);

    m_notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &Client::onUserInput);
}

void Client::onConnected()
{
    qDebug() << "Connected to server.";
}

void Client::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QTextStream(stdout) << data;
}

void Client::onDisconnected()
{
    qDebug() << "Disconnected from server.";
    qApp->quit();
}

void Client::onUserInput()
{
    QTextStream qin(stdin);
    QString line = qin.readLine();
    if (!line.isNull()) {
        m_socket->write(line.toUtf8());
        if(!m_isNameSet) {
            m_isNameSet = true; // Предполагаем, что первое отправленное сообщение - это имя
            QTextStream(stdout) << "You can now start sending messages.\n";
        } else {
            m_socket->write("\n"); // Добавляем перенос строки для последующих сообщений
        }
    }
}
