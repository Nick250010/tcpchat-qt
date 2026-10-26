#include "chat_window.hpp"

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

ChatWindow::ChatWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("TCPChat"));
    resize(1000, 680);
    setMinimumSize(760, 500);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto *connectionBox = new QWidget(central);
    auto *connectionLayout = new QHBoxLayout(connectionBox);
    connectionLayout->setContentsMargins(0, 0, 0, 0);
    m_host = new QLineEdit(QStringLiteral("127.0.0.1"), connectionBox);
    m_host->setPlaceholderText(QStringLiteral("Server address"));
    m_host->setMinimumWidth(150);
    m_port = new QLineEdit(QStringLiteral("5050"), connectionBox);
    m_port->setMaximumWidth(80);
    m_nickname = new QLineEdit(QStringLiteral("Alex"), connectionBox);
    m_nickname->setPlaceholderText(QStringLiteral("Nickname"));
    m_connectButton = new QPushButton(QStringLiteral("Connect"), connectionBox);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Server"), connectionBox));
    connectionLayout->addWidget(m_host, 2);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Port"), connectionBox));
    connectionLayout->addWidget(m_port);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Nickname"), connectionBox));
    connectionLayout->addWidget(m_nickname, 1);
    connectionLayout->addWidget(m_connectButton);
    root->addWidget(connectionBox);

    auto *body = new QWidget(central);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(12);

    auto *chatPanel = new QWidget(body);
    auto *chatLayout = new QVBoxLayout(chatPanel);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    m_roomLabel = new QLabel(QStringLiteral("#lobby"), chatPanel);
    m_roomLabel->setObjectName(QStringLiteral("roomTitle"));
    chatLayout->addWidget(m_roomLabel);
    m_transcript = new QTextEdit(chatPanel);
    m_transcript->setReadOnly(true);
    m_transcript->setAcceptRichText(false);
    m_transcript->setPlaceholderText(QStringLiteral("Messages will appear here when you connect."));
    chatLayout->addWidget(m_transcript, 1);
    auto *sendRow = new QWidget(chatPanel);
    auto *sendLayout = new QHBoxLayout(sendRow);
    sendLayout->setContentsMargins(0, 0, 0, 0);
    m_message = new QLineEdit(sendRow);
    m_message->setPlaceholderText(QStringLiteral("Write a message…"));
    m_sendButton = new QPushButton(QStringLiteral("Send"), sendRow);
    sendLayout->addWidget(m_message, 1);
    sendLayout->addWidget(m_sendButton);
    chatLayout->addWidget(sendRow);
    bodyLayout->addWidget(chatPanel, 1);

    auto *side = new QWidget(body);
    side->setMaximumWidth(230);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    auto *roomTitle = new QLabel(QStringLiteral("ROOMS"), side);
    roomTitle->setObjectName(QStringLiteral("sectionTitle"));
    sideLayout->addWidget(roomTitle);
    auto *roomRow = new QWidget(side);
    auto *roomLayout = new QHBoxLayout(roomRow);
    roomLayout->setContentsMargins(0, 0, 0, 0);
    m_room = new QLineEdit(QStringLiteral("lobby"), roomRow);
    m_room->setPlaceholderText(QStringLiteral("Room name"));
    m_joinButton = new QPushButton(QStringLiteral("Join"), roomRow);
    roomLayout->addWidget(m_room, 1);
    roomLayout->addWidget(m_joinButton);
    sideLayout->addWidget(roomRow);
    auto *usersTitle = new QLabel(QStringLiteral("PEOPLE IN ROOM"), side);
    usersTitle->setObjectName(QStringLiteral("sectionTitle"));
    sideLayout->addWidget(usersTitle);
    m_users = new QListWidget(side);
    sideLayout->addWidget(m_users, 1);
    auto *nickTitle = new QLabel(QStringLiteral("CHANGE NICKNAME"), side);
    nickTitle->setObjectName(QStringLiteral("sectionTitle"));
    sideLayout->addWidget(nickTitle);
    m_newNickname = new QLineEdit(side);
    m_newNickname->setPlaceholderText(QStringLiteral("New nickname"));
    m_nickButton = new QPushButton(QStringLiteral("Update nickname"), side);
    sideLayout->addWidget(m_newNickname);
    sideLayout->addWidget(m_nickButton);
    bodyLayout->addWidget(side);
    root->addWidget(body, 1);

    m_status = new QLabel(QStringLiteral("Disconnected"), central);
    m_status->setObjectName(QStringLiteral("status"));
    root->addWidget(m_status);
    setCentralWidget(central);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background: #111827; color: #e5e7eb; font-size: 13px; }"
        "QLineEdit, QTextEdit, QListWidget { background: #1f2937; border: 1px solid #374151; border-radius: 7px; padding: 8px; selection-background-color: #2563eb; }"
        "QPushButton { background: #2563eb; border: none; border-radius: 7px; padding: 9px 14px; font-weight: 600; }"
        "QPushButton:hover { background: #1d4ed8; } QPushButton:disabled { background: #374151; color: #9ca3af; }"
        "QLabel#roomTitle { font-size: 19px; font-weight: 700; padding: 4px 0; }"
        "QLabel#sectionTitle { color: #93c5fd; font-size: 11px; font-weight: 700; margin-top: 8px; }"
        "QLabel#status { color: #9ca3af; padding-top: 4px; }"
    ));

    connect(m_connectButton, &QPushButton::clicked, this, &ChatWindow::toggleConnection);
    connect(m_sendButton, &QPushButton::clicked, this, &ChatWindow::sendCurrentMessage);
    connect(m_message, &QLineEdit::returnPressed, this, &ChatWindow::sendCurrentMessage);
    connect(m_joinButton, &QPushButton::clicked, this, &ChatWindow::joinSelectedRoom);
    connect(m_room, &QLineEdit::returnPressed, this, &ChatWindow::joinSelectedRoom);
    connect(m_nickButton, &QPushButton::clicked, this, &ChatWindow::changeNickname);
    connect(&m_connection, &ChatConnection::statusChanged, m_status, &QLabel::setText);
    connect(&m_connection, &ChatConnection::systemMessage, this, &ChatWindow::appendSystem);
    connect(&m_connection, &ChatConnection::chatMessage, this, &ChatWindow::appendChat);
    connect(&m_connection, &ChatConnection::usersChanged, this, [this](const QStringList &users) {
        m_users->clear();
        m_users->addItems(users);
    });
    connect(&m_connection, &ChatConnection::roomChanged, this, [this](const QString &room) {
        m_room->setText(room);
        m_roomLabel->setText(QStringLiteral("#%1").arg(room));
    });
    connect(&m_connection, &ChatConnection::nicknameChanged, this, [this](const QString &nick) {
        m_nickname->setText(nick);
        m_newNickname->clear();
    });
    connect(&m_connection, &ChatConnection::connected, this, [this] { setConnectedControls(true); });
    connect(&m_connection, &ChatConnection::disconnected, this, [this] { setConnectedControls(false); });
    setConnectedControls(false);
}

void ChatWindow::toggleConnection() {
    if (m_connection.isConnected()) {
        m_connection.disconnectFromHost();
        return;
    }
    bool ok = false;
    const uint port = m_port->text().toUInt(&ok);
    if (!ok || port == 0 || port > 65535) {
        QMessageBox::warning(this, QStringLiteral("Invalid port"), QStringLiteral("Enter a port from 1 to 65535."));
        return;
    }
    if (m_nickname->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Nickname required"), QStringLiteral("Enter a nickname first."));
        return;
    }
    m_connection.connectToHost(m_host->text().trimmed(), static_cast<quint16>(port), m_nickname->text());
}

void ChatWindow::sendCurrentMessage() {
    if (!m_connection.isConnected()) return;
    const QString text = m_message->text().trimmed();
    if (text.isEmpty()) return;
    m_connection.sendMessage(text);
    m_message->clear();
}

void ChatWindow::joinSelectedRoom() {
    if (m_connection.isConnected() && !m_room->text().trimmed().isEmpty()) m_connection.joinRoom(m_room->text());
}

void ChatWindow::changeNickname() {
    const QString nick = m_newNickname->text().trimmed();
    if (m_connection.isConnected() && !nick.isEmpty()) m_connection.changeNickname(nick);
}

void ChatWindow::appendSystem(const QString &text) {
    m_transcript->append(QStringLiteral("<span style='color:#93c5fd'>[%1] %2</span>")
                         .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm")), text.toHtmlEscaped()));
}

void ChatWindow::appendChat(const QString &nickname, const QString &time, const QString &text) {
    m_transcript->append(QStringLiteral("<span style='color:#9ca3af'>%1</span> <b style='color:#bfdbfe'>%2</b>: %3")
                         .arg(time.toHtmlEscaped(), nickname.toHtmlEscaped(), text.toHtmlEscaped()));
}

void ChatWindow::setConnectedControls(bool connected) {
    m_connectButton->setText(connected ? QStringLiteral("Disconnect") : QStringLiteral("Connect"));
    m_host->setEnabled(!connected);
    m_port->setEnabled(!connected);
    m_nickname->setEnabled(!connected);
    m_sendButton->setEnabled(connected);
    m_message->setEnabled(connected);
    m_joinButton->setEnabled(connected);
    m_room->setEnabled(connected);
    m_nickButton->setEnabled(connected);
    m_newNickname->setEnabled(connected);
    if (connected) m_connection.requestUsers();
}
