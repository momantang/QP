#include "player.h"
#include <QtWidgets/QApplication>
#include  <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QFile>
#include <QUrl>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Player Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt MultiDedia Player Example");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url","The Url(s) to open.");
    parser.process(app);
    Player player;
    if (!parser.positionalArguments().isEmpty()&& player.isPlayerAvailable())
    {
	    QList<QUrl> urls;
        for (auto &a:parser.positionalArguments())
        {
	        urls.append(QUrl::fromUserInput(a,QDir::currentPath()));
        }
        player.addToPlaylist(urls);
    }
    player.show();
    return app.exec();
}
