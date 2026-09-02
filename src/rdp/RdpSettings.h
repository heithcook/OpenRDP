#pragma once

#include <QString>
#include <QSize>
#include <QVector>
#include <QtGlobal>
#include <optional>
#include "display/MonitorTopology.h"
#include "channels/AudioConfiguration.h"
#include "channels/DriveRedirection.h"
#include "channels/PrinterRedirection.h"

namespace openrdp {

enum class AuthenticationMode { NlaPassword, EntraWebAccount };

struct ConnectionSettings {
    QString hostname;
    quint16 port = 3389;
    QString username;
    QString domain;
    int desktopWidth = 1920;
    int desktopHeight = 1080;
    bool enableNla = true;
    bool enableTls = true;
    bool dynamicResolution = true;
    bool clipboard = true;
    AudioConfiguration audio;
    QVector<RedirectedFolderConfig> folders;
    QVector<PrinterInfo> printers;
    QVector<MonitorInfo> monitors;
    AuthenticationMode authenticationMode = AuthenticationMode::NlaPassword;
};

struct ParsedServer {
    QString hostname;
    quint16 port = 3389;
};

std::optional<ParsedServer> parseServer(const QString& value, QString* error = nullptr);
void parseUsername(const QString& value, QString& username, QString& domain);
std::optional<QString> authorizationCodeFromRedirect(const QString& redirectUrl);
QSize constrainedDisplaySize(const QSize& requested);

} // namespace openrdp
