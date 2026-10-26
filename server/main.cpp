#include "chat_server.hpp"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QHostAddress>
#include <QTextStream>
#include <cstdio>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("TCPChat Server"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("TCPChat C++ server"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption portOption({QStringLiteral("p"), QStringLiteral("port")},
                                  QStringLiteral("Port to listen on (default: 5050)."),
                                  QStringLiteral("port"), QStringLiteral("5050"));
    parser.addOption(portOption);
    parser.process(app);

    bool ok = false;
    const uint parsedPort = parser.value(portOption).toUInt(&ok);
    if (!ok || parsedPort == 0 || parsedPort > 65535) {
        QTextStream(stderr) << "Invalid port. Choose a number from 1 to 65535.\n";
        return 2;
    }

    ChatServer server;
    if (!server.listen(static_cast<quint16>(parsedPort))) {
        QTextStream(stderr) << "Could not listen on port " << parsedPort << ". Is it already in use?\n";
        return 1;
    }
    QTextStream(stdout) << "TCPChat server listening on port " << parsedPort << "\n";
    QTextStream(stdout) << "Press Ctrl+C to stop the server.\n";
    return app.exec();
}
