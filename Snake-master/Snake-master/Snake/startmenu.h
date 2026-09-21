#ifndef STARTMENU_H
#define STARTMENU_H

#include <QWidget>

class QStackedWidget;
class QLineEdit;
class QLabel;
class QPushButton;

//开始菜单：单人游玩 / 联机对战 / 退出
class StartMenu : public QWidget
{
    Q_OBJECT

public:
    explicit StartMenu(QWidget *parent = nullptr);

    //回到第一页（模式选择）
    void showMainPage();

    //显示一行状态文字，isError 为真时显示为红色
    void setStatus(const QString &text, bool isError = false);

    //连接过程中禁用输入，避免重复点击
    void setBusy(bool busy);

signals:
    void singlePlayerRequested();
    void multiplayerConnectRequested(const QString &host, quint16 port);
    void quitRequested();

private slots:
    void onSingleClicked();
    void onMultiClicked();
    void onConnectClicked();
    void onBackClicked();

private:
    QWidget *buildMainPage();
    QWidget *buildConnectPage();

    QStackedWidget *m_pages;
    QLineEdit *m_hostEdit;
    QLineEdit *m_portEdit;
    QPushButton *m_connectButton;
    QPushButton *m_backButton;
    QLabel *m_statusLabel;
};

#endif // STARTMENU_H
