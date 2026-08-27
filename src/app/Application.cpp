#include "app/Application.h"
#include "gui/MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <freerdp/freerdp.h>
namespace openrdp {
int runApplication(int argc,char** argv){QApplication app(argc,argv);QCoreApplication::setApplicationName(QStringLiteral("OpenRDP"));QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));QCommandLineParser p;p.setApplicationDescription(QStringLiteral("Native Qt client using embedded FreeRDP"));p.addHelpOption();p.addVersionOption();QCommandLineOption server({QStringLiteral("v")},QStringLiteral("Remote computer"),QStringLiteral("server"));QCommandLineOption user({QStringLiteral("u")},QStringLiteral("User name"),QStringLiteral("username"));p.addOption(server);p.addOption(user);p.addPositionalArgument(QStringLiteral("server"),QStringLiteral("Remote computer"),QStringLiteral("[server]"));p.process(app);QString host=p.value(server);if(host.isEmpty()&&!p.positionalArguments().isEmpty())host=p.positionalArguments().first();MainWindow window(host,p.value(user));window.show();return app.exec();}
}
