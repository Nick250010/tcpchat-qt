#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>

class ChatServer final : public QObject {
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);
    bool listen(quint16 port);

private slots:
    void acceptConnections();
    void readClient();
    void disconnectClient();

private:
    struct ClientState {
        QByteArray buffer;
        QString nickname;
        QString room = QStringLiteral("lobby");
    };

    void processLine(QTcpSocket *socket, const QByteArray &line);
    void sendJson(QTcpSocket *socket, const QJsonObject &object);
    void sendSystem(QTcpSocket *socket, const QString &text);
    void broadcastRoom(const QString &room, const QJsonObject &object, QTcpSocket *except = nullptr);
    void sendUsers(const QString &room);
    void removeClient(QTcpSocket *socket);
    QString uniqueNickname(QString desired, QTcpSocket *except) const;

    QTcpServer m_server;
    QHash<QTcpSocket *, ClientState> m_clients;
};
