#ifndef SNAKE_H
#define SNAKE_H

#include <QMainWindow>
#include <QTcpSocket>
#include <qpainter.h>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class Snake; }
QT_END_NAMESPACE

class StartMenu;

class Snake : public QMainWindow
{
    Q_OBJECT

public:
    Snake(QWidget *parent = nullptr);
    ~Snake();
    //游戏界面
    void paintEvent(QPaintEvent *event);
    //初始化蛇
    void InitSnake();
    QRect CreateFood();//产生食物
    void IsEat();//判断是否吃到了食物
    void IsHit();//判断是否撞到了自己
    void IsWin();//判断是否赢得了游戏


private slots://定义槽函数的方式
    //蛇的移动
    void Snake_update();
    //服务器接受数据
    void receiveData();

    //菜单选择
    void onSinglePlayerRequested();
    void onMultiplayerConnectRequested(const QString &host, quint16 port);
    //网络状态
    void onSocketConnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketDisconnected();


private:
    //当前处于哪个界面
    enum GameMode
    {
        ModeMenu,   //在主菜单
        ModeSingle, //单人游玩
        ModeMulti   //联机对战
    };

    //一局游戏的启动/结束/回到菜单
    void startGame(GameMode mode);
    void showMenuOverlay();
    void restartGame();
    //处理服务端发来的一条完整消息
    void processLine(const QString &line);
    //断开并销毁网络连接
    void teardownSocket();

    Ui::Snake *ui;
    StartMenu *m_menu;

    GameMode m_mode;
    bool m_connecting;//正在联机连接中（还没拿到ID）

    bool getID;//是否获取ID
    int playerID;//用户ID
    QTcpSocket *socket;//用于网络的链接
    QByteArray m_buffer;//网络粘包缓冲
    QString m_pendingHost;//待连接地址
    quint16 m_pendingPort;//待连接端口

    QTimer *timer;//计时器事件，只要设置了这个，每隔一段时间会去执行指定的东西
    int nDirection;//蛇的移动方向
    bool blsRun;//用于控制是否开始的变量
    bool blsOver;//用于控制是否结束的变量
    void keyPressEvent(QKeyEvent *key);//键盘事件
    QVector<QRect> vSnakeRect; //n个小方块组成的vector，代表蛇
    QRect SnakeHead;//蛇头
    QRect OtherSnakeHead;
    QMap<int,QVector<QRect>> OtherPlayers;//其他玩家的蛇
    QString Display;//显示的内容
    QRect Food;//用来表示食物的变量
    int Score;//得分
    int Speed;//蛇的移动速度
};
#endif // SNAKE_H
