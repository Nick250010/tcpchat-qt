#include "chat_window.hpp"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TCPChat"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("TCPChat Project"));
    ChatWindow window;
    window.show();
    return app.exec();
}
