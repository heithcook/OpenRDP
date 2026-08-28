#include "app/ConnectionHistory.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace openrdp {
namespace {
constexpr qsizetype maximumEntries = 20;

QString defaultHistoryPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/openrdp/history.json");
}
}

ConnectionHistory::ConnectionHistory(QString filePath)
    : filePath_(filePath.isEmpty() ? defaultHistoryPath() : std::move(filePath))
{
}

QVector<ConnectionHistoryEntry> ConnectionHistory::load() const
{
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) return {};

    QVector<ConnectionHistoryEntry> entries;
    for (const QJsonValue& value : document.array()) {
        const QJsonObject object = value.toObject();
        ConnectionHistoryEntry entry;
        entry.server = object.value(QStringLiteral("server")).toString().trimmed();
        entry.username = object.value(QStringLiteral("username")).toString();
        entry.authenticationMode = object.value(QStringLiteral("webAccount")).toBool()
            ? AuthenticationMode::EntraWebAccount : AuthenticationMode::NlaPassword;
        if (!entry.server.isEmpty()) entries.append(std::move(entry));
        if (entries.size() == maximumEntries) break;
    }
    return entries;
}

bool ConnectionHistory::record(const ConnectionHistoryEntry& entry) const
{
    if (entry.server.trimmed().isEmpty()) return false;
    QVector<ConnectionHistoryEntry> entries = load();
    entries.removeIf([&entry](const ConnectionHistoryEntry& existing) {
        return existing.server.compare(entry.server, Qt::CaseInsensitive) == 0 &&
            existing.username == entry.username &&
            existing.authenticationMode == entry.authenticationMode;
    });
    entries.prepend(entry);
    if (entries.size() > maximumEntries) entries.resize(maximumEntries);

    const QFileInfo info(filePath_);
    if (!QDir().mkpath(info.absolutePath())) return false;
    QJsonArray array;
    for (const ConnectionHistoryEntry& saved : entries) {
        array.append(QJsonObject{{QStringLiteral("server"), saved.server},
            {QStringLiteral("username"), saved.username},
            {QStringLiteral("webAccount"), saved.authenticationMode == AuthenticationMode::EntraWebAccount}});
    }
    QSaveFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly)) return false;
    if (file.write(QJsonDocument(array).toJson(QJsonDocument::Indented)) < 0) return false;
    return file.commit();
}

} // namespace openrdp
