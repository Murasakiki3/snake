#include "myserver.h"
#include <QDebug>

MyServer::MyServer()
{
    playerCount = 0;
    //监听8888端口
    if(listen(QHostAddress::Any, 8888))
    {
        qDebug() << "服务器启动成功";
    }
    else
    {
        qDebug() << "服务器启动失败";
    }
}


void MyServer::onReadyRead()
{

    QTcpSocket *socket =
        qobject_cast<QTcpSocket*>(sender());


    if(!socket)
        return;



    QByteArray data =
        socket->readAll();



    qDebug()
        <<"收到玩家数据:"
        <<data;



    QString str(data);


    QStringList list =
        str.split(",");



    if(list.size()<2)
    {
        return;
    }



    int id =
        list[0].toInt();



    int length =
        list[1].toInt();



    QVector<QRect> snake;



    int index=2;



    for(int i=0;i<length;i++)
    {


        if(index+1 >= list.size())
            return;


        int x =
            list[index++].toInt();


        int y =
            list[index++].toInt();



        snake.push_back(
            QRect(x,y,20,20)
            );

    }



    //保存玩家蛇

    players[socket].snake = snake;



    qDebug()
        <<"更新玩家:"
        <<id;



    //发送所有玩家蛇数据

    QString allData;


    for(auto it = players.begin();
         it != players.end();
         it++)
    {

        Player p = it.value();


        allData += QString::number(p.id);


        allData += ",";


        allData += QString::number(p.snake.size());



        for(QRect r:p.snake)
        {

            allData += ",";

            allData += QString::number(r.x());


            allData += ",";

            allData += QString::number(r.y());

        }


        allData += ";";

    }



    //发送给所有客户端

    for(QTcpSocket *client:clients)
    {

        client->write(
            allData.toUtf8()
            );

    }

}

// 客户端断开连接的槽函数
void MyServer::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if(socket)
    {
        Player p = players.value(socket);
        int id = p.id;

        // 清理所有容器里的残留数据
        clients.removeOne(socket);
        players.remove(socket);
        playerIDs.remove(socket);

        // 广播玩家退出消息
        QString msg = QString("QUIT,%1").arg(id);
        for(QTcpSocket *client : clients)
        {
            client->write(msg.toUtf8());
        }

        socket->deleteLater();
        qDebug() << "玩家退出，ID:" << id;
    }
}

void MyServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    qDebug() << "一个玩家连接";
    clients.append(socket);

    connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    int newId = playerCount;
    socket->write(QString::number(newId).toUtf8());
    qDebug() << "玩家ID:" << newId;
    Player p;


    p.id = newId;



    //初始化蛇长度5

    for(int i=0;i<5;i++)
    {

        p.snake.push_back(
            QRect(
                200,
                140+i*20,
                20,
                20
                )
            );

    }


    players[socket]=p;
   playerIDs[socket] = newId;
    playerCount++;
}
