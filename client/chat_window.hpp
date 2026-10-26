#pragma once

#include "chat_connection.hpp"

#include <QMainWindow>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTextEdit;

class ChatWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit ChatWindow(QWidget *parent = nullptr);

private slots:
    void toggleConnection();
    void sendCurrentMessage();
    void joinSelectedRoom();
    void changeNickname();
    void appendSystem(const QString &text);

private:
    void appendChat(const QString &nickname, const QString &time, const QString &text);
    void setConnectedControls(bool connected);

    ChatConnection m_connection;
    QLineEdit *m_host = nullptr;
    QLineEdit *m_port = nullptr;
    QLineEdit *m_nickname = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_roomLabel = nullptr;
    QTextEdit *m_transcript = nullptr;
    QLineEdit *m_message = nullptr;
    QLineEdit *m_room = nullptr;
    QLineEdit *m_newNickname = nullptr;
    QListWidget *m_users = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_joinButton = nullptr;
    QPushButton *m_nickButton = nullptr;
};
