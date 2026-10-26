#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace tcpchat {
inline constexpr quint16 DefaultPort = 5050;
inline constexpr qsizetype MaxLineBytes = 8192;

inline QByteArray encode(const QJsonObject &object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
}

inline QString cleanText(QString value, qsizetype maxLength = 256) {
    value = value.trimmed();
    value.remove(QChar::Null);
    if (value.size() > maxLength) value.truncate(maxLength);
    return value;
}
}
