#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>

class ChatConnection final : public QObject {
    Q_OBJECT
public:
    explicit ChatConnection(QObject *parent = nullptr);
    bool isConnected() const;
    void connectToHost(const QString &host, quint16 port, const QString &nickname);
    void disconnectFromHost();
    void sendMessage(const QString &text);
    void joinRoom(const QString &room);
    void changeNickname(const QString &nickname);
    void requestUsers();

signals:
    void connected();
    void disconnected();
    void statusChanged(const QString &status);
    void systemMessage(const QString &text);
    void chatMessage(const QString &nickname, const QString &time, const QString &text);
    void usersChanged(const QStringList &users);
    void roomChanged(const QString &room);
    void nicknameChanged(const QString &nickname);

private slots:
    void readAvailable();
    void socketError(QAbstractSocket::SocketError error);

private:
    void sendObject(const QJsonObject &object);
    void processLine(const QByteArray &line);

    QTcpSocket m_socket;
    QByteArray m_buffer;
    QString m_pendingNickname;
};
