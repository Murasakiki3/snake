#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QVector>
#include <QRect>
#include <QByteArray>

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

    //解析一条玩家记录并转发
    void processRecord(QTcpSocket *socket, const QString &record);

    //把当前所有玩家的蛇广播给所有客户端
    void broadcastWorld();

    QList<QTcpSocket*> clients;

    QMap<QTcpSocket*,int> playerIDs;

    int playerCount;

    QMap<QTcpSocket*,Player> players;

    //每个连接各留一份粘包缓冲，按 ';' 切分玩家记录
    QMap<QTcpSocket*,QByteArray> clientBuffers;

};


#endif // MYSERVER_H
