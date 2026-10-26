#include "chat_connection.hpp"
#include "shared/protocol.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ChatConnection::ChatConnection(QObject *parent) : QObject(parent) {
    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        emit statusChanged(QStringLiteral("Connected; signing in…"));
        sendObject({{QStringLiteral("type"), QStringLiteral("hello")},
                    {QStringLiteral("nick"), m_pendingNickname}});
        emit connected();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this] {
        m_buffer.clear();
        emit disconnected();
        emit statusChanged(QStringLiteral("Disconnected"));
    });
    connect(&m_socket, &QTcpSocket::readyRead, this, &ChatConnection::readAvailable);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &ChatConnection::socketError);
}

bool ChatConnection::isConnected() const { return m_socket.state() == QAbstractSocket::ConnectedState; }

void ChatConnection::connectToHost(const QString &host, quint16 port, const QString &nickname) {
    if (m_socket.state() != QAbstractSocket::UnconnectedState) m_socket.abort();
    m_buffer.clear();
    m_pendingNickname = nickname.trimmed();
    emit statusChanged(QStringLiteral("Connecting to %1:%2…").arg(host).arg(port));
    m_socket.connectToHost(host, port);
}

void ChatConnection::disconnectFromHost() { m_socket.disconnectFromHost(); }

void ChatConnection::sendMessage(const QString &text) {
    const QString cleaned = text.trimmed();
    if (!cleaned.isEmpty()) sendObject({{QStringLiteral("type"), QStringLiteral("message")}, {QStringLiteral("text"), cleaned}});
}

void ChatConnection::joinRoom(const QString &room) {
    sendObject({{QStringLiteral("type"), QStringLiteral("join")}, {QStringLiteral("room"), room.trimmed()}});
}

void ChatConnection::changeNickname(const QString &nickname) {
    sendObject({{QStringLiteral("type"), QStringLiteral("nick")}, {QStringLiteral("nick"), nickname.trimmed()}});
}

void ChatConnection::requestUsers() { sendObject({{QStringLiteral("type"), QStringLiteral("who")}}); }

void ChatConnection::sendObject(const QJsonObject &object) {
    if (isConnected()) m_socket.write(tcpchat::encode(object));
}

void ChatConnection::readAvailable() {
    m_buffer += m_socket.readAll();
    if (m_buffer.size() > tcpchat::MaxLineBytes && !m_buffer.contains('\n')) {
        emit systemMessage(QStringLiteral("Server response too long; closing connection."));
        m_socket.abort();
        return;
    }
    while (true) {
        const qsizetype newline = m_buffer.indexOf('\n');
        if (newline < 0) break;
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);
        if (!line.isEmpty()) processLine(line);
    }
}

void ChatConnection::processLine(const QByteArray &line) {
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        emit systemMessage(QStringLiteral("Received an invalid message from server."));
        return;
    }
    const QJsonObject object = document.object();
    const QString type = object.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("system")) {
        emit systemMessage(object.value(QStringLiteral("text")).toString());
    } else if (type == QStringLiteral("message")) {
        emit chatMessage(object.value(QStringLiteral("nick")).toString(),
                         object.value(QStringLiteral("time")).toString(),
                         object.value(QStringLiteral("text")).toString());
    } else if (type == QStringLiteral("users")) {
        QStringList users;
        for (const QJsonValue &value : object.value(QStringLiteral("users")).toArray()) users.append(value.toString());
        emit usersChanged(users);
    } else if (type == QStringLiteral("room")) {
        emit roomChanged(object.value(QStringLiteral("room")).toString());
    } else if (type == QStringLiteral("welcome")) {
        emit nicknameChanged(object.value(QStringLiteral("nick")).toString());
        emit roomChanged(object.value(QStringLiteral("room")).toString());
    }
}

void ChatConnection::socketError(QAbstractSocket::SocketError) {
    emit statusChanged(m_socket.errorString());
}
