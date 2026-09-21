#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QVector>
#include <QRect>

struct Player
{
    int id;

    QVector<QRect> snake;

};


class MyServer : public QTcpServer
{
    Q_OBJECT

public:

    MyServer();

protected:

    void incomingConnection(qintptr socketDescriptor);

private slots:
    void clientDisconnected();
    void onReadyRead();

private:

    QList<QTcpSocket*> clients;

    QMap<QTcpSocket*,int> playerIDs;

    int playerCount;

    QMap<QTcpSocket*,Player> players;

};


#endif // MYSERVER_H