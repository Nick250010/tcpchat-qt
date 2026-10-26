#include "chat_server.hpp"
#include "shared/protocol.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QHostAddress>
#include <QRegularExpression>

ChatServer::ChatServer(QObject *parent) : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection, this, &ChatServer::acceptConnections);
}

bool ChatServer::listen(quint16 port) {
    return m_server.listen(QHostAddress::Any, port);
}

void ChatServer::acceptConnections() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        if (!socket) continue;
        m_clients.insert(socket, ClientState{});
        connect(socket, &QTcpSocket::readyRead, this, &ChatServer::readClient);
        connect(socket, &QTcpSocket::disconnected, this, &ChatServer::disconnectClient);
        qInfo().noquote() << "Client connected:" << socket->peerAddress().toString();
        sendSystem(socket, QStringLiteral("Connected. Choose a nickname to join the chat."));
    }
}

void ChatServer::readClient() {
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket || !m_clients.contains(socket)) return;
    ClientState &state = m_clients[socket];
    state.buffer += socket->readAll();
    if (state.buffer.size() > tcpchat::MaxLineBytes && !state.buffer.contains('\n')) {
        sendSystem(socket, QStringLiteral("Input line too long; disconnecting."));
        socket->disconnectFromHost();
        return;
    }
    while (true) {
        const qsizetype newline = state.buffer.indexOf('\n');
        if (newline < 0) break;
        if (newline > tcpchat::MaxLineBytes) {
            socket->disconnectFromHost();
            return;
        }
        const QByteArray line = state.buffer.left(newline).trimmed();
        state.buffer.remove(0, newline + 1);
        if (!line.isEmpty()) processLine(socket, line);
        if (!m_clients.contains(socket)) return;
    }
}

void ChatServer::processLine(QTcpSocket *socket, const QByteArray &line) {
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        sendSystem(socket, QStringLiteral("Invalid message format."));
        return;
    }
    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();
    ClientState &state = m_clients[socket];

    if (type == QStringLiteral("hello")) {
        const QString desired = tcpchat::cleanText(obj.value(QStringLiteral("nick")).toString(), 24);
        if (desired.isEmpty() || !QRegularExpression(QStringLiteral("^[\\p{L}\\p{N}_-]+$")).match(desired).hasMatch()) {
            sendSystem(socket, QStringLiteral("Nickname must be 1-24 letters, digits, _ or - characters."));
            return;
        }
        const QString oldNick = state.nickname;
        state.nickname = uniqueNickname(desired, socket);
        sendJson(socket, {{QStringLiteral("type"), QStringLiteral("welcome")},
                          {QStringLiteral("nick"), state.nickname},
                          {QStringLiteral("room"), state.room}});
        sendSystem(socket, QStringLiteral("Welcome, %1! You are in #%2.").arg(state.nickname, state.room));
        if (!oldNick.isEmpty()) broadcastRoom(state.room, {{QStringLiteral("type"), QStringLiteral("system")},
                                                            {QStringLiteral("text"), oldNick + QStringLiteral(" is now known as ") + state.nickname}});
        else broadcastRoom(state.room, {{QStringLiteral("type"), QStringLiteral("system")},
                                        {QStringLiteral("text"), state.nickname + QStringLiteral(" joined the chat.")}}, socket);
        sendUsers(state.room);
        return;
    }

    if (state.nickname.isEmpty()) {
        sendSystem(socket, QStringLiteral("Send a nickname first."));
        return;
    }

    if (type == QStringLiteral("message")) {
        const QString text = tcpchat::cleanText(obj.value(QStringLiteral("text")).toString(), 2000);
        if (text.isEmpty()) return;
        broadcastRoom(state.room, {{QStringLiteral("type"), QStringLiteral("message")},
                                   {QStringLiteral("nick"), state.nickname},
                                   {QStringLiteral("room"), state.room},
                                   {QStringLiteral("text"), text},
                                   {QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"))}});
    } else if (type == QStringLiteral("join")) {
        const QString room = tcpchat::cleanText(obj.value(QStringLiteral("room")).toString(), 32);
        if (room.isEmpty() || !QRegularExpression(QStringLiteral("^[\\p{L}\\p{N}_-]+$")).match(room).hasMatch()) {
            sendSystem(socket, QStringLiteral("Room names may contain letters, digits, _ and -."));
            return;
        }
        const QString previous = state.room;
        if (previous == room) return;
        broadcastRoom(previous, {{QStringLiteral("type"), QStringLiteral("system")},
                                 {QStringLiteral("text"), state.nickname + QStringLiteral(" left the room.")}}, socket);
        state.room = room;
        sendJson(socket, {{QStringLiteral("type"), QStringLiteral("room")}, {QStringLiteral("room"), room}});
        sendSystem(socket, QStringLiteral("You joined #%1.").arg(room));
        broadcastRoom(room, {{QStringLiteral("type"), QStringLiteral("system")},
                             {QStringLiteral("text"), state.nickname + QStringLiteral(" joined the room.")}}, socket);
        sendUsers(previous);
        sendUsers(room);
    } else if (type == QStringLiteral("who")) {
        sendUsers(state.room);
    } else if (type == QStringLiteral("nick")) {
        const QString desired = tcpchat::cleanText(obj.value(QStringLiteral("nick")).toString(), 24);
        if (desired.isEmpty() || !QRegularExpression(QStringLiteral("^[\\p{L}\\p{N}_-]+$")).match(desired).hasMatch()) {
            sendSystem(socket, QStringLiteral("Invalid nickname."));
            return;
        }
        const QString old = state.nickname;
        state.nickname = uniqueNickname(desired, socket);
        sendJson(socket, {{QStringLiteral("type"), QStringLiteral("welcome")}, {QStringLiteral("nick"), state.nickname}, {QStringLiteral("room"), state.room}});
        broadcastRoom(state.room, {{QStringLiteral("type"), QStringLiteral("system")},
                                   {QStringLiteral("text"), old + QStringLiteral(" is now ") + state.nickname}}, nullptr);
    } else {
        sendSystem(socket, QStringLiteral("Unknown command."));
    }
}

void ChatServer::sendJson(QTcpSocket *socket, const QJsonObject &object) {
    if (socket && socket->state() == QAbstractSocket::ConnectedState) socket->write(tcpchat::encode(object));
}

void ChatServer::sendSystem(QTcpSocket *socket, const QString &text) {
    sendJson(socket, {{QStringLiteral("type"), QStringLiteral("system")}, {QStringLiteral("text"), text}});
}

void ChatServer::broadcastRoom(const QString &room, const QJsonObject &object, QTcpSocket *except) {
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        if (it.key() != except && it->room == room && !it->nickname.isEmpty()) sendJson(it.key(), object);
    }
}

void ChatServer::sendUsers(const QString &room) {
    QJsonArray users;
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        if (it->room == room && !it->nickname.isEmpty()) users.append(it->nickname);
    }
    broadcastRoom(room, {{QStringLiteral("type"), QStringLiteral("users")}, {QStringLiteral("users"), users}});
}

void ChatServer::removeClient(QTcpSocket *socket) {
    const auto it = m_clients.find(socket);
    if (it == m_clients.end()) return;
    const QString nick = it->nickname;
    const QString room = it->room;
    m_clients.erase(it);
    if (!nick.isEmpty()) {
        broadcastRoom(room, {{QStringLiteral("type"), QStringLiteral("system")},
                             {QStringLiteral("text"), nick + QStringLiteral(" disconnected.")}});
        sendUsers(room);
        qInfo().noquote() << nick << "disconnected";
    }
    socket->deleteLater();
}

void ChatServer::disconnectClient() {
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket) removeClient(socket);
}

QString ChatServer::uniqueNickname(QString desired, QTcpSocket *except) const {
    auto exists = [this, except](const QString &candidate) {
        for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it)
            if (it.key() != except && it->nickname.compare(candidate, Qt::CaseInsensitive) == 0) return true;
        return false;
    };
    if (!exists(desired)) return desired;
    for (int i = 2; i < 10000; ++i) {
        const QString candidate = desired + QString::number(i);
        if (!exists(candidate)) return candidate;
    }
    return desired + QString::number(QDateTime::currentMSecsSinceEpoch() % 100000);
}
