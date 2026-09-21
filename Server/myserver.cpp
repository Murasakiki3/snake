#include "myserver.h"
#include <QDebug>

//协议说明（客户端 Snake 必须与之匹配）
//  服务端 -> 客户端
//    连接建立:  "ID,<编号>\n"                        给新玩家分配ID（单独一帧，不带 ';'）
//    每帧广播:  "<id>,<长度>,<x>,<y>,...;...;\n"     所有玩家的蛇，末尾 '\n' 是一帧的结束标记
//    玩家退出:  "QUIT,<编号>\n"
//  客户端 -> 服务端
//    每帧一次:  "<id>,<长度>,<x>,<y>,...;"           一条记录以 ';' 结束

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



    //累积到该连接自己的缓冲里，按 ';' 切出一条条完整记录。
    //这样即使一次 write 被拆成多次到达、或多帧粘在一起，也不会解析错位。
    QByteArray &buffer = clientBuffers[socket];

    buffer += socket->readAll();



    int index;

    while((index = buffer.indexOf(';')) != -1)
    {

        QByteArray record = buffer.left(index);

        buffer.remove(0, index+1);


        if(record.isEmpty())
            continue;


        processRecord(socket, QString::fromUtf8(record));

    }

}


void MyServer::processRecord(QTcpSocket *socket, const QString &record)
{

    qDebug()
        <<"收到玩家数据:"
        <<record;



    QStringList list =
        record.split(",");



    if(list.size()<2)
    {
        return;
    }



    int id =
        list[0].toInt();



    int length =
        list[1].toInt();



    //长度非法或数据不完整就丢弃，避免越界
    if(length<=0 || list.size() < 2 + length*2)
    {
        return;
    }



    QVector<QRect> snake;



    int index=2;



    for(int i=0;i<length;i++)
    {

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

    players[socket].id = id;



    qDebug()
        <<"更新玩家:"
        <<id;



    broadcastWorld();

}


void MyServer::broadcastWorld()
{

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



    //一帧的结束标记，客户端靠它判断“这一帧收全了”
    allData += "\n";



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
        clientBuffers.remove(socket);

        // 广播玩家退出消息
        QString msg = QString("QUIT,%1\n").arg(id);
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

    clientBuffers[socket].clear();

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

    // 握手：带明确前缀和结束符，客户端据此认出自己的ID。
    // 旧版本这里直接发一个裸数字，客户端无法与玩家数据区分，会导致 playerID 永远为 0。
    // 注意这里不要加 ';'，handshake 本身就是一帧，加 ';' 会让客户端解析出非法整数。
    socket->write(QString("ID,%1\n").arg(newId).toUtf8());
    qDebug() << "玩家ID:" << newId;
}
