#pragma once

#include <QString>
#include <QtGlobal>
#include <optional>

namespace openrdp {

struct ConnectionSettings {
    QString hostname;
    quint16 port = 3389;
    QString username;
    QString domain;
    int desktopWidth = 1920;
    int desktopHeight = 1080;
    bool enableNla = true;
    bool enableTls = true;
};

struct ParsedServer {
    QString hostname;
    quint16 port = 3389;
};

std::optional<ParsedServer> parseServer(const QString& value, QString* error = nullptr);
void parseUsername(const QString& value, QString& username, QString& domain);

} // namespace openrdp
