#include "app/Application.h"
#include "gui/MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>

namespace openrdp {

int runApplication(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("OpenRDP"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Native Qt client using embedded FreeRDP"));
    parser.addHelpOption();
    QCommandLineOption server({QStringLiteral("v")}, QStringLiteral("Remote computer"),
        QStringLiteral("server"));
    QCommandLineOption user({QStringLiteral("u")}, QStringLiteral("User name"),
        QStringLiteral("username"));
    parser.addOption(server);
    parser.addOption(user);
    parser.addPositionalArgument(QStringLiteral("server"), QStringLiteral("Remote computer"),
        QStringLiteral("[server]"));
    parser.process(app);

    QString host = parser.value(server);
    QString rdpFile;
    if (host.isEmpty() && !parser.positionalArguments().isEmpty()) {
        const QString positional = parser.positionalArguments().first();
        if (positional.endsWith(QStringLiteral(".rdp"), Qt::CaseInsensitive)) rdpFile = positional;
        else host = positional;
    }
    MainWindow window(host, parser.value(user), rdpFile);
    window.show();
    return app.exec();
}

} // namespace openrdp
