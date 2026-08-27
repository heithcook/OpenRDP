#include "rdp/RdpSettings.h"

#include <QUrl>

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

} // namespace openrdp
