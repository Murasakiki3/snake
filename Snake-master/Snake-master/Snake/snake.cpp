#include "snake.h"
#include "ui_snake.h"
#include<QPainter>
#include<QTimer>
#include<QKeyEvent>
#include <QRandomGenerator>
#include <QDebug>
Snake::Snake(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Snake),
      blsRun(false), //初始：游戏未开始
      Speed(500),//500ms刷新一次
      getID(false),
      playerID(0)
{
    ui->setupUi(this); //加载ui
    qDebug()<<"Snake启动";
    this->setFocusPolicy(Qt::StrongFocus);
    socket = new QTcpSocket(this);
    socket->connectToHost("127.0.0.1",8888 );
    connect(socket,&QTcpSocket::readyRead,this,&Snake::receiveData);
    qDebug()<<"连接服务器成功";
    this->setGeometry(QRect(1000,300,560,580));//设置窗体位置：Qrect表示矩形框，这个矩形框放在距离屏幕左侧600单位，距离屏幕上方300单位，长290，高310.
}


Snake::~Snake()
{
    delete ui; //结束ui 释放内存
}


//实现游戏界面，所有在游戏界面中显示的都要在这个函数中实现，这一函数在第一次启动程序和调用update的时候会被执行
void Snake::paintEvent(QPaintEvent *event){ //所有的绘图都要在paintEvent函数里面进行
    QPainter painter(this);  //创建画家 在窗口绘图
    if(!blsRun)//只在第一次运行的时候初始化蛇
        InitSnake();    //在游戏界面中出现蛇

    //画游戏的背景，这里先设置外墙的墙体
    painter.setPen(Qt::black);//外边框用黑色的画笔去画
    painter.setBrush(Qt::gray);//设置填充的颜色为灰色
    painter.drawRect(15,15,260,260);//绘制矩形，以外部的框体为基准，要学会换算四个边界的坐标
    painter.setPen(Qt::black);//同理，再设置内墙的墙体
    painter.setBrush(Qt::black);
    painter.drawRect(20,20,500,500);//说明墙的宽度是5个单位
    painter.drawPixmap(20,20,500,500,QPixmap(":/new/img/img/bg.jpg"));

    //画格子，用循环实现
    painter.setPen(Qt::blue);
    for (int i=2;i<=25;i++) {
        painter.drawLine(20, 20+i*20, 520, 20+i*20);//画横线
        painter.drawLine(20+i*20, 20, 20+i*20, 520);//画竖线
    }

    //显示游戏开始和结束
    QFont font1("Courier",24);//设置字体，颜色
    painter.setFont(font1);
    painter.setPen(Qt::red);
    painter.setBrush(Qt::red);
    painter.drawText(80,300,Display); //将display的内容显示在80，300的位置上

    //显示分数
    QFont font2("Courier",15);
    painter.setFont(font2);
    painter.setPen(Qt::blue);
    painter.setBrush(Qt::red);
    painter.drawText(280,550,"得分：");
    painter.drawText(380,550,QString::number(Score));//把score转换成字符串并显示出来

    //画蛇
    painter.setPen(Qt::black);//设置蛇的边界颜色

    //画其他玩家蛇
   painter.setBrush(Qt::red);

    for(auto it = OtherPlayers.begin();
         it != OtherPlayers.end();
         it++)
    {


        //如果是自己的ID，跳过
        qDebug()
            <<"我的ID:"
            <<playerID
            <<"当前ID:"
            <<it.key();
        if(it.key()==playerID)
        {
            continue;
        }



        QVector<QRect> snake =
            it.value();



        for(QRect rect : snake)
        {

            painter.drawRect(rect);

        }

    }
    painter.setBrush(Qt::green);//设置蛇的颜色
    //painter.drawRect(vSnakeRect);//让蛇出现
    if(!vSnakeRect.isEmpty())
    {

        painter.drawRects(
            &vSnakeRect[0],
            vSnakeRect.size()
            );

    }

    //在界面中画出食物
    //painter.drawRect(Food);
    painter.drawPixmap(Food,QPixmap(":/new/img/img/Apple.png"));

    //游戏停止，通过让计时器停止来结束
    if(blsOver)
        timer->stop();
}




//用来说明代表蛇的小方块应该画在什么地方
void Snake::InitSnake(){
    //第一次进来的时候显示“游戏开始”
    Display="游戏开始！";
    blsRun=true;//游戏开始了，令成true
    blsOver=false;//游戏没有结束
    nDirection=2;//默认刚开始蛇的移动方向是向下
    Food=CreateFood();//在游戏开始的时候产生食物
    Score=0;//初始化得分=0


    //矩形框
    //QRect rect(100,70,10,10);//蛇距离左，上边界各100，70单位
    //vSnakeRect=rect;//用这个小方块初始化蛇
    vSnakeRect.resize(5);//蛇的长度
    //用for循环实现蛇
    for(int i=0;i<vSnakeRect.size();i++){
        QRect rect(200, 140 + 20*i, 20, 20);
        vSnakeRect[vSnakeRect.size()-1-i]=rect;
    }
    //对计时器的设定
    timer=new QTimer(this);//设定计时器
    timer->start(Speed);//设定计时器的间隔时间为500ms，用speed表示更统一
    connect(timer,SIGNAL(timeout()),SLOT(Snake_update()));//SIGNAL:信号 SLOT：槽,对信号和槽的连接
}


void Snake::keyPressEvent(QKeyEvent *event)
{

    switch(event->key())
    {

    case Qt::Key_Up:

        nDirection=1;

        break;


    case Qt::Key_Down:

        nDirection=2;

        break;


    case Qt::Key_Left:

        nDirection=3;

        break;


    case Qt::Key_Right:

        nDirection=4;

        break;


    default:

        break;

    }

}
//更新蛇
void Snake::Snake_update(){
    //在蛇出来之后，就不显示“游戏开始”这一提示语句了
    Display="";
    SnakeHead=vSnakeRect.first();//获取蛇头，让蛇头等于vector中的第一个元素
    IsEat();//判断是否吃到食物
    IsHit();//判断是否撞到自己
    IsWin();//判断是否赢了


    //非蛇头的蛇身的移动
    for(int j=0;j<vSnakeRect.size()-1;j++){
        vSnakeRect[vSnakeRect.size()-1-j]=vSnakeRect[vSnakeRect.size()-2-j];
    }
    //根据蛇头的朝向来更新蛇的位置
    switch (nDirection) {
    case 1:
        SnakeHead.setTop(SnakeHead.top()-20);//如果是往上移动，让蛇的上面的坐标减10
        SnakeHead.setBottom(SnakeHead.bottom()-20);//同时蛇的底部也向上移动10
        break;
    //向其他三个方向移动同理
    case 2:
        SnakeHead.setTop(SnakeHead.top()+20);
        SnakeHead.setBottom(SnakeHead.bottom()+20);
        break;
    case 3:
        SnakeHead.setLeft(SnakeHead.left()-20);
        SnakeHead.setRight(SnakeHead.right()-20);
        break;
    case 4:
        SnakeHead.setLeft(SnakeHead.left()+20);
        SnakeHead.setRight(SnakeHead.right()+20);
        break;
    default:;//默认情况下就什么都不做
    }
    vSnakeRect[0]=SnakeHead;


    //发送整条蛇数据

    QString data;

    data += QString::number(playerID);

    data += ",";

    data += QString::number(vSnakeRect.size());


    for(int i=0;i<vSnakeRect.size();i++)
    {

        data += ",";
        data += QString::number(vSnakeRect[i].x());

        data += ",";
        data += QString::number(vSnakeRect[i].y());

    }


    socket->write(data.toUtf8());



    //如果蛇头离开了墙壁的范围，即蛇头撞墙，则游戏结束
    if(SnakeHead.left()<20||SnakeHead.right()>520||SnakeHead.top()<20||SnakeHead.bottom()>520){
        blsOver = true;
        Display = "游戏结束";
    }

    update();
}


//产生食物
QRect Snake::CreateFood(){
    int x,y;
    //随机产生食物，使用到随机函数
    //x和y是0到25之间的随机的整数
    x=QRandomGenerator::global()->generate()%25;
    y=QRandomGenerator::global()->generate()%25;
    QRect rect(20 + x*20, 20 + y*20, 20, 20);
    return rect;
}


//判断是否吃到了食物
void Snake::IsEat(){
    //吃到食物，蛇变长(注意思考怎样才叫做蛇变长一节)其实是下一个刷新的时刻让蛇身变长了，得分加10
    if(SnakeHead==Food){
        SnakeHead=Food;//如果吃到了食物，就将食物所在位置变成新的蛇头
        vSnakeRect.push_back(vSnakeRect.last());//把数组中最后一个元素再重复地添加到数组中，就相当于蛇多了一节
        Food = CreateFood();//再产生新的食物
        Score=Score+10;
        Speed=qMax(50,Speed-60);;//如果吃的够多速度会变成负数所以设置最低50ms
        timer->stop();//停止之前的计时器
        timer->start(Speed);//把更新之后的计时器启动

    }
}


//判断是否撞到自己
void Snake::IsHit(){
    //用for循环遍历蛇身，0是蛇头，因此从1开始
    for(int i=1;i<vSnakeRect.size();i++){
        //如果撞上了
        if(SnakeHead==vSnakeRect[i]){
            Display="游戏结束";
            blsOver=true;
            update();//刷新界面
        }
    }
}


//判断是否游戏胜利
void Snake::IsWin(){
    //设置如果达到了100分则游戏胜利
    if(Score==100){
        Display="恭喜你赢了";
        blsOver=true;
        update();
    }
}


void Snake::receiveData()
{

    QByteArray data =
        socket->readAll();



    QString str(data);

    if(str.startsWith("QUIT"))
    {

        QStringList list =
            str.split(",");


        int id =
            list[1].toInt();


        OtherPlayers.remove(id);


        update();


        return;

    }




    //按照玩家分割

    QStringList playerList =
        str.split(";");



    //清除上一帧数据

    OtherPlayers.clear();



    for(QString playerData : playerList)
    {


        //防止最后一个空数据

        if(playerData.isEmpty())
            continue;



        QStringList list =
            playerData.split(",");



        if(list.size()<2)
            continue;



        //玩家ID

        int id =
            list[0].toInt();



        //蛇长度

        int length =
            list[1].toInt();



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

        OtherPlayers[id]=snake;



    }


    update();
}