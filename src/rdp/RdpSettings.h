#pragma once

#include <QString>
#include <QtGlobal>
#include <optional>

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
    AuthenticationMode authenticationMode = AuthenticationMode::NlaPassword;
};

struct ParsedServer {
    QString hostname;
    quint16 port = 3389;
};

std::optional<ParsedServer> parseServer(const QString& value, QString* error = nullptr);
void parseUsername(const QString& value, QString& username, QString& domain);
std::optional<QString> authorizationCodeFromRedirect(const QString& redirectUrl);

} // namespace openrdp
