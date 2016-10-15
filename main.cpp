#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QMessageBox>

#include <QTcpServer>
#include <QTcpSocket>

#include <QStringList>
#include <QProcess>

#include <iostream>
#include <string>

const uint PORT = 8899;
const std::string VSCODE_BIN = R"(C:\Program Files (x86)\Microsoft VS Code\Code.exe)";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSystemTrayIcon icon{QIcon(":/code.ico")};
    QMenu* menu = new QMenu();
    menu->addAction("Exit", [&a]() { a.exit(); });
    icon.setContextMenu(menu);
    icon.show();

    QTcpServer tcpServer{};
    if ( !tcpServer.listen(QHostAddress::LocalHost, PORT) ) {
        std::cout << "Can't start server!" << std::endl;
        QMessageBox::critical(nullptr,
                              "VSCode Server",
                              "Unable to start the server: " + tcpServer.errorString());
        return 1;
    }

    QObject::connect(&tcpServer, &QTcpServer::newConnection, [&tcpServer](){
        QTcpSocket *clientConnection = tcpServer.nextPendingConnection();
        QObject::connect(clientConnection, &QAbstractSocket::disconnected,
                         clientConnection, &QObject::deleteLater);

        clientConnection->waitForReadyRead();
        std::string fname = clientConnection->readLine(2048);
        fname.erase(std::remove_if(fname.begin(), fname.end(), [](char c) -> bool {
            return strchr("\r\n\t", c) != NULL;
        }), fname.end());
        std::cout << "Request to open '" << fname << "'" << std::endl;

        QProcess builder{};
        builder.start(QString::fromStdString(VSCODE_BIN), QStringList() << QString::fromStdString(fname));
        builder.waitForStarted() && builder.waitForFinished();

        clientConnection->disconnectFromHost();
    });

    return a.exec();
}
