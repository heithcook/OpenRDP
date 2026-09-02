#include "rdp/RdpSettings.h"

#include <QUrl>
#include <QUrlQuery>

namespace openrdp {

std::optional<ParsedServer> parseServer(const QString& input, QString* error)
{
    const QString value = input.trimmed();
    if (value.isEmpty()) {
        if (error) *error = QStringLiteral("Enter a computer name or IP address.");
        return std::nullopt;
    }

    const QUrl url(QStringLiteral("rdp://") + value, QUrl::StrictMode);
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
        !url.path().isEmpty()) {
        if (error) *error = QStringLiteral("The computer address is not valid.");
        return std::nullopt;
    }
    const int port = url.port(3389);
    if (port < 1 || port > 65535) {
        if (error) *error = QStringLiteral("The port must be between 1 and 65535.");
        return std::nullopt;
    }
    return ParsedServer{url.host(), static_cast<quint16>(port)};
}

void parseUsername(const QString& value, QString& username, QString& domain)
{
    const qsizetype slash = value.indexOf(u'\\');
    if (slash > 0) {
        domain = value.left(slash);
        username = value.mid(slash + 1);
    } else {
        domain.clear();
        username = value;
    }
}

std::optional<QString> authorizationCodeFromRedirect(const QString& redirectUrl)
{
    const QUrl url(redirectUrl, QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != QStringLiteral("https")) return std::nullopt;
    if (url.host().compare(QStringLiteral("login.microsoftonline.com"), Qt::CaseInsensitive) != 0 ||
        url.path() != QStringLiteral("/common/oauth2/nativeclient")) return std::nullopt;
    const QString code = QUrlQuery(url).queryItemValue(QStringLiteral("code"), QUrl::FullyDecoded);
    if (code.isEmpty()) return std::nullopt;
    return code;
}

QSize constrainedDisplaySize(const QSize& requested)
{
    return {qBound(200, requested.width(), 8192), qBound(200, requested.height(), 8192)};
}

} // namespace openrdp
