#include "startmenu.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

const char *kStyle = R"QSS(
#MenuRoot {
    background-color: #14211a;
}
QLabel#Title {
    color: #7ede7e;
    font-size: 32px;
    font-weight: bold;
}
QLabel#Subtitle {
    color: #6c8f76;
    font-size: 13px;
}
QLabel#FieldLabel {
    color: #a8d8b4;
    font-size: 14px;
}
QLabel#Signature {
    color: #86c79a;
    font-size: 12px;
    letter-spacing: 2px;
}
QPushButton {
    background-color: #2f6b3f;
    color: #eaffea;
    border: 2px solid #4f9c63;
    border-radius: 8px;
    padding: 10px 18px;
    font-size: 16px;
    min-width: 190px;
}
QPushButton:hover {
    background-color: #3d8a52;
    border-color: #7ede7e;
}
QPushButton:pressed {
    background-color: #24512f;
}
QPushButton:disabled {
    background-color: #26332a;
    color: #5d7a66;
    border-color: #35473b;
}
QLineEdit {
    background-color: #0e1712;
    color: #eaffea;
    border: 2px solid #3d8a52;
    border-radius: 6px;
    padding: 6px 10px;
    font-size: 14px;
    min-width: 180px;
}
QLineEdit:disabled {
    color: #5d7a66;
    border-color: #35473b;
}
)QSS";

const char *kStatusNormal = "color: #a8d8b4; font-size: 13px;";
const char *kStatusError  = "color: #ff8a8a; font-size: 13px;";

const char *kDefaultHint = "\xe6\x8f\x90\xe7\xa4\xba\xef\xbc\x9a\xe5\x85\x88\xe5\x9c\xa8\xe6\x9c\xac\xe6\x9c\xba\xe6\x88\x96\xe5\x8f\xa6\xe4\xb8\x80\xe5\x8f\xb0\xe6\x9c\xba\xe5\x99\xa8\xe4\xb8\x8a\xe8\xbf\x90\xe8\xa1\x8c Server \xe7\xa8\x8b\xe5\xba\x8f";

//署名：出现在菜单每一页的底部
QLabel *makeSignature()
{
    QLabel *label = new QLabel(QStringLiteral("Murasakiki3"));

    label->setObjectName(QStringLiteral("Signature"));
    label->setAlignment(Qt::AlignCenter);
    label->setToolTip(QStringLiteral("测试：Murasakiki3"));

    return label;
}

} // namespace

StartMenu::StartMenu(QWidget *parent)
    : QWidget(parent)
    , m_pages(nullptr)
    , m_hostEdit(nullptr)
    , m_portEdit(nullptr)
    , m_connectButton(nullptr)
    , m_backButton(nullptr)
    , m_statusLabel(nullptr)
{
    setObjectName(QStringLiteral("MenuRoot"));

    //QWidget 子类必须打开这个属性，样式表里的背景色才会生效
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setStyleSheet(QString::fromUtf8(kStyle));

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(buildMainPage());
    m_pages->addWidget(buildConnectPage());

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->addWidget(m_pages);

    showMainPage();
}

QWidget *StartMenu::buildMainPage()
{
    QWidget *page = new QWidget;

    QLabel *title = new QLabel(QStringLiteral("贪 吃 蛇"));
    title->setObjectName(QStringLiteral("Title"));
    title->setAlignment(Qt::AlignCenter);

    QLabel *subtitle = new QLabel(QStringLiteral("S N A K E   ·   Qt 5"));
    subtitle->setObjectName(QStringLiteral("Subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    QPushButton *btnSingle = new QPushButton(QStringLiteral("单 人 游 玩"));
    QPushButton *btnMulti = new QPushButton(QStringLiteral("联 机 对 战"));
    QPushButton *btnQuit = new QPushButton(QStringLiteral("退 出 游 戏"));

    connect(btnSingle, &QPushButton::clicked, this, &StartMenu::onSingleClicked);
    connect(btnMulti, &QPushButton::clicked, this, &StartMenu::onMultiClicked);
    connect(btnQuit, &QPushButton::clicked, this, &StartMenu::quitRequested);

    QLabel *hint = new QLabel(QStringLiteral("方向键 ↑ ↓ ← → 控制移动     游戏中按 Esc 返回菜单"));
    hint->setObjectName(QStringLiteral("Subtitle"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addStretch(1);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(34);
    layout->addWidget(btnSingle, 0, Qt::AlignCenter);
    layout->addSpacing(12);
    layout->addWidget(btnMulti, 0, Qt::AlignCenter);
    layout->addSpacing(12);
    layout->addWidget(btnQuit, 0, Qt::AlignCenter);
    layout->addSpacing(26);
    layout->addWidget(hint);
    layout->addSpacing(18);
    layout->addWidget(makeSignature());
    layout->addStretch(1);

    return page;
}

QWidget *StartMenu::buildConnectPage()
{
    QWidget *page = new QWidget;

    QLabel *title = new QLabel(QStringLiteral("联 机 对 战"));
    title->setObjectName(QStringLiteral("Title"));
    title->setAlignment(Qt::AlignCenter);

    QLabel *hostLabel = new QLabel(QStringLiteral("服务器地址"));
    hostLabel->setObjectName(QStringLiteral("FieldLabel"));

    QLabel *portLabel = new QLabel(QStringLiteral("端口"));
    portLabel->setObjectName(QStringLiteral("FieldLabel"));

    m_hostEdit = new QLineEdit(QStringLiteral("127.0.0.1"));
    m_hostEdit->setPlaceholderText(QStringLiteral("例如 192.168.1.10"));

    m_portEdit = new QLineEdit(QStringLiteral("8888"));
    m_portEdit->setValidator(new QIntValidator(1, 65535, m_portEdit));
    m_portEdit->setMaxLength(5);

    m_connectButton = new QPushButton(QStringLiteral("连 接 并 开 始"));
    m_backButton = new QPushButton(QStringLiteral("返 回"));

    connect(m_connectButton, &QPushButton::clicked, this, &StartMenu::onConnectClicked);
    connect(m_backButton, &QPushButton::clicked, this, &StartMenu::onBackClicked);
    connect(m_hostEdit, &QLineEdit::returnPressed, this, &StartMenu::onConnectClicked);
    connect(m_portEdit, &QLineEdit::returnPressed, this, &StartMenu::onConnectClicked);

    m_statusLabel = new QLabel(QString::fromUtf8(kDefaultHint));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QString::fromUtf8(kStatusNormal));

    QFormLayout *form = new QFormLayout;
    form->addRow(hostLabel, m_hostEdit);
    form->addRow(portLabel, m_portEdit);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignCenter);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(m_backButton);
    buttons->addSpacing(10);
    buttons->addWidget(m_connectButton);
    buttons->addStretch(1);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addStretch(1);
    layout->addWidget(title);
    layout->addSpacing(30);
    layout->addLayout(form);
    layout->addSpacing(20);
    layout->addLayout(buttons);
    layout->addSpacing(22);
    layout->addWidget(m_statusLabel);
    layout->addSpacing(16);
    layout->addWidget(makeSignature());
    layout->addStretch(1);

    return page;
}

void StartMenu::showMainPage()
{
    if (m_statusLabel)
    {
        m_statusLabel->setText(QString::fromUtf8(kDefaultHint));
        m_statusLabel->setStyleSheet(QString::fromUtf8(kStatusNormal));
    }

    setBusy(false);

    if (m_pages)
        m_pages->setCurrentIndex(0);
}

void StartMenu::setStatus(const QString &text, bool isError)
{
    if (!m_statusLabel)
        return;

    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString::fromUtf8(isError ? kStatusError : kStatusNormal));
}

void StartMenu::setBusy(bool busy)
{
    if (m_connectButton)
        m_connectButton->setEnabled(!busy);

    if (m_backButton)
        m_backButton->setEnabled(!busy);

    if (m_hostEdit)
        m_hostEdit->setEnabled(!busy);

    if (m_portEdit)
        m_portEdit->setEnabled(!busy);
}

void StartMenu::onSingleClicked()
{
    emit singlePlayerRequested();
}

void StartMenu::onMultiClicked()
{
    setStatus(QString::fromUtf8(kDefaultHint));
    setBusy(false);

    if (m_pages)
        m_pages->setCurrentIndex(1);

    if (m_hostEdit)
        m_hostEdit->setFocus();
}

void StartMenu::onBackClicked()
{
    showMainPage();
}

void StartMenu::onConnectClicked()
{
    const QString host = m_hostEdit->text().trimmed();

    bool ok = false;
    const int port = m_portEdit->text().toInt(&ok);

    if (host.isEmpty())
    {
        setStatus(QStringLiteral("请输入服务器地址"), true);
        m_hostEdit->setFocus();
        return;
    }

    if (!ok || port < 1 || port > 65535)
    {
        setStatus(QStringLiteral("端口号无效，请输入 1 - 65535"), true);
        m_portEdit->setFocus();
        return;
    }

    setBusy(true);
    setStatus(QStringLiteral("正在连接 %1:%2 ...").arg(host).arg(port));

    emit multiplayerConnectRequested(host, static_cast<quint16>(port));
}
