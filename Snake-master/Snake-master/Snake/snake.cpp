#include "snake.h"
#include "ui_snake.h"
#include "startmenu.h"
#include<QPainter>
#include<QTimer>
#include<QKeyEvent>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QDebug>

namespace {

//从 "3"、"3;" 这类文本里取出第一个整数。
//直接 toInt() 遇到分号会失败并返回 0，曾经导致所有玩家的 playerID 都是 0。
int firstInt(const QString &text)
{
    int begin = -1;

    for(int i=0;i<text.size();i++)
    {
        const QChar c = text.at(i);

        if(c.isDigit())
        {
            if(begin<0)
                begin = i;
        }
        else if(begin>=0)
        {
            break;
        }
    }

    if(begin<0)
        return 0;

    int end = begin;

    while(end<text.size() && text.at(end).isDigit())
        end++;

    return text.mid(begin, end-begin).toInt();
}

} // namespace

Snake::Snake(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Snake)
    , m_menu(nullptr)
    , m_mode(ModeMenu)
    , m_connecting(false)
    , getID(false)
    , playerID(0)
    , socket(nullptr)
    , m_pendingHost(QStringLiteral("127.0.0.1"))
    , m_pendingPort(8888)
    , timer(nullptr)
    , nDirection(2)
    , blsRun(false)
    , blsOver(false)
    , Score(0)
    , Speed(500)
{
    ui->setupUi(this); //加载ui

    this->setWindowTitle(QStringLiteral("贪吃蛇"));
    this->setFocusPolicy(Qt::StrongFocus);
    this->setGeometry(QRect(1000,300,560,580));//设置窗体位置

    //计时器只创建一次，整局游戏复用（避免重开时反复 new/connect 造成泄漏）
    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),SLOT(Snake_update()));

    //开始菜单作为中央部件的子控件，用布局铺满整个客户区
    m_menu = new StartMenu(ui->centralwidget);
    QVBoxLayout *menuLayout = new QVBoxLayout(ui->centralwidget);
    menuLayout->setContentsMargins(0,0,0,0);
    menuLayout->addWidget(m_menu);

    connect(m_menu,&StartMenu::singlePlayerRequested,
            this,&Snake::onSinglePlayerRequested);
    connect(m_menu,&StartMenu::multiplayerConnectRequested,
            this,&Snake::onMultiplayerConnectRequested);
    connect(m_menu,&StartMenu::quitRequested,this,&Snake::close);

    qDebug()<<"Snake启动，进入主菜单";

    //启动时不连接服务器、不开始游戏，等玩家在菜单里选择
    showMenuOverlay();
}


Snake::~Snake()
{
    if(timer)
        timer->stop();

    delete ui; //结束ui 释放内存
}


//实现游戏界面，所有在游戏界面中显示的都要在这个函数中实现，这一函数在第一次启动程序和调用update的时候会被执行
void Snake::paintEvent(QPaintEvent *event){ //所有的绘图都要在paintEvent函数里面进行
    Q_UNUSED(event)

    //在主菜单时游戏区域不绘制，全部交给 StartMenu
    if(m_mode==ModeMenu)
    {
        QPainter menuPainter(this);
        menuPainter.fillRect(rect(),QColor(20,33,26));
        return;
    }

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

    //底部状态提示
    QFont font3("Microsoft YaHei",10);
    painter.setFont(font3);
    if(blsOver)
    {
        painter.setPen(QColor(255,220,80));
        painter.drawText(QRect(20,536,254,24),
                         Qt::AlignLeft|Qt::AlignVCenter,
                         QStringLiteral("按 R 重新开始   按 Esc 返回主菜单"));
    }
    else if(m_mode==ModeMulti)
    {
        painter.setPen(QColor(60,120,70));
        painter.drawText(QRect(20,536,254,24),
                         Qt::AlignLeft|Qt::AlignVCenter,
                         QStringLiteral("联机中  玩家ID %1  在线 %2 人")
                             .arg(playerID)
                             .arg(OtherPlayers.size()));
    }

    //游戏停止，通过让计时器停止来结束
    if(blsOver)
        timer->stop();
}




//用来说明代表蛇的小方块应该画在什么地方
void Snake::InitSnake(){
    //第一次进来的时候显示“游戏开始”
    Display = (m_mode==ModeMulti) ? QStringLiteral("联机开始！")
                                  : QStringLiteral("游戏开始！");
    blsRun=true;//游戏开始了，令成true
    blsOver=false;//游戏没有结束
    nDirection=2;//默认刚开始蛇的移动方向是向下
    Food=CreateFood();//在游戏开始的时候产生食物
    Score=0;//初始化得分=0
    Speed=500;//重置速度（吃到食物会变快，重开时必须还原）

    //矩形框
    vSnakeRect.clear();
    vSnakeRect.resize(5);//蛇的长度
    //用for循环实现蛇
    for(int i=0;i<vSnakeRect.size();i++){
        QRect rect(200, 140 + 20*i, 20, 20);
        vSnakeRect[vSnakeRect.size()-1-i]=rect;
    }

    timer->start(Speed);//设定计时器的间隔时间为500ms，用speed表示更统一
}


void Snake::keyPressEvent(QKeyEvent *event)
{
    //Esc 从游戏中返回主菜单
    if(event->key()==Qt::Key_Escape)
    {
        if(m_mode!=ModeMenu)
            showMenuOverlay();

        return;
    }

    //R 在游戏结束后重开一局
    if(event->key()==Qt::Key_R)
    {
        if(m_mode!=ModeMenu && blsOver)
            restartGame();

        return;
    }

    //菜单里或已经结束时，方向键不生效
    if(m_mode==ModeMenu || blsOver)
        return;

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


    //单人模式不联网，只有联机模式才发送整条蛇数据
    if(m_mode==ModeMulti
        && socket
        && socket->state()==QAbstractSocket::ConnectedState)
    {
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

        //';' 表示一条玩家记录结束，服务端按这个切分
        data += ";";

        socket->write(data.toUtf8());
    }



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
    if(!socket)
        return;

    m_buffer += socket->readAll();

    //服务端每发完一帧完整的世界状态就补一个 '\n'，用它作为帧结束标记。
    //这样即使 TCP 把多帧粘在一起、或者把一帧拆成几次送达，都能正确解析。
    int newlineIndex;

    while((newlineIndex = m_buffer.indexOf('\n')) != -1)
    {
        QByteArray line = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex+1);

        if(!line.isEmpty())
            processLine(QString::fromUtf8(line));
    }
}


//处理服务端发来的一条完整消息
void Snake::processLine(const QString &line)
{
    //1) 握手：服务端分配玩家ID, 格式 "ID,<编号>"
    if(line.startsWith(QLatin1String("ID,")))
    {
        playerID = firstInt(line.mid(3));
        getID = true;

        qDebug()<<"获得玩家ID:"<<playerID;

        //等待期间玩家可能已经按 Esc 回菜单了，这里再确认一次
        if(m_connecting)
        {
            m_connecting = false;
            startGame(ModeMulti);
        }

        return;
    }

    //2) 玩家退出, 格式 "QUIT,<编号>"
    if(line.startsWith(QLatin1String("QUIT,")))
    {
        QStringList list = line.split(",");

        if(list.size()>=2)
            OtherPlayers.remove(firstInt(list[1]));

        update();

        return;
    }

    //3) 一帧完整的世界状态："id,长度,x,y,x,y,...;id,长度,x,y,...;"

    OtherPlayers.clear();

    QStringList playerList = line.split(';', Qt::SkipEmptyParts);

    for(QString playerData : playerList)
    {
        QStringList list = playerData.split(",");

        if(list.size()<2)
            continue;

        int id = firstInt(list[0]);

        int length = firstInt(list[1]);

        //长度非法或数据不完整就跳过，避免越界
        if(length<=0 || list.size() < 2 + length*2)
            continue;

        QVector<QRect> snake;

        int index=2;

        for(int i=0;i<length;i++)
        {
            int x = list[index++].toInt();

            int y = list[index++].toInt();

            snake.push_back(QRect(x,y,20,20));
        }

        OtherPlayers[id]=snake;
    }

    update();
}


//———————————— 菜单与游戏流程 ————————————

void Snake::onSinglePlayerRequested()
{
    qDebug()<<"选择单人游玩";

    teardownSocket();

    startGame(ModeSingle);
}


void Snake::onMultiplayerConnectRequested(const QString &host, quint16 port)
{
    qDebug()<<"选择联机对战:"<<host<<port;

    m_pendingHost = host;
    m_pendingPort = port;

    teardownSocket();

    m_buffer.clear();
    getID = false;
    playerID = 0;
    m_connecting = true;

    socket = new QTcpSocket(this);

    connect(socket,&QTcpSocket::readyRead,this,&Snake::receiveData);
    connect(socket,&QTcpSocket::connected,this,&Snake::onSocketConnected);
    connect(socket,&QAbstractSocket::errorOccurred,this,&Snake::onSocketError);
    connect(socket,&QTcpSocket::disconnected,this,&Snake::onSocketDisconnected);

    socket->connectToHost(host,port);
}


void Snake::onSocketConnected()
{
    qDebug()<<"已连接服务器，等待分配ID";

    if(m_menu && m_menu->isVisible())
        m_menu->setStatus(QStringLiteral("已连接，等待服务器分配 ID ..."));

    //握手超时保护：连上了但服务端一直不给ID（例如跑的是旧版本服务端）
    QTimer::singleShot(5000,this,[this]()
    {
        if(!m_connecting || getID)
            return;

        qDebug()<<"等待ID超时";

        if(m_menu && m_menu->isVisible())
        {
            m_menu->setStatus(QStringLiteral("已连接但未收到 ID，请确认服务端已更新到最新版本"),true);
            m_menu->setBusy(false);
        }

        teardownSocket();
        m_connecting = false;
    });
}


void Snake::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)

    const QString msg = socket ? socket->errorString() : QStringLiteral("未知错误");

    qDebug()<<"网络错误:"<<msg;

    if(m_mode==ModeMenu)
    {
        //还在菜单里，展示错误让玩家重试
        if(m_menu && m_menu->isVisible())
        {
            m_menu->setStatus(QStringLiteral("连接失败：%1").arg(msg),true);
            m_menu->setBusy(false);
        }

        teardownSocket();
        m_connecting = false;
    }
    else
    {
        //游戏中掉线
        Display = QStringLiteral("连接已断开");
        blsOver = true;
        timer->stop();
        update();
    }
}


void Snake::onSocketDisconnected()
{
    qDebug()<<"与服务器断开连接";

    if(m_mode==ModeMulti && !blsOver)
    {
        Display = QStringLiteral("连接已断开");
        blsOver = true;
        timer->stop();
        update();
    }
}


void Snake::startGame(GameMode mode)
{
    m_mode = mode;
    blsRun = false;   //交给 paintEvent 触发 InitSnake()
    blsOver = false;
    nDirection = 2;
    Speed = 500;
    Score = 0;
    Display.clear();
    vSnakeRect.clear();
    OtherPlayers.clear();

    if(!m_buffer.isEmpty())
        m_buffer.clear();

    if(m_menu)
        m_menu->hide();

    setFocus();
    activateWindow();
    update();
}


void Snake::showMenuOverlay()
{
    m_mode = ModeMenu;
    blsRun = false;
    blsOver = false;
    nDirection = 2;
    Speed = 500;
    Score = 0;
    Display.clear();
    vSnakeRect.clear();
    OtherPlayers.clear();

    if(timer)
        timer->stop();

    teardownSocket();
    m_connecting = false;

    if(m_menu)
    {
        m_menu->showMainPage();
        m_menu->show();
        m_menu->raise();
        m_menu->setFocus();
    }

    update();
}


void Snake::restartGame()
{
    if(m_mode==ModeMenu)
    {
        showMenuOverlay();
        return;
    }

    blsRun = false;
    blsOver = false;
    nDirection = 2;
    Speed = 500;
    Score = 0;
    Display.clear();
    vSnakeRect.clear();

    timer->stop();

    setFocus();
    update();
}


void Snake::teardownSocket()
{
    if(socket)
    {
        //先断开所有信号，避免在回调里把自己删掉
        socket->disconnect(this);
        socket->abort();
        socket->deleteLater();
        socket = nullptr;
    }

    m_buffer.clear();
    getID = false;
}
