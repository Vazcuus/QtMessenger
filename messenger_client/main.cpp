#include <QCoreApplication>
#include "chatclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Client client;
    return a.exec();
}
