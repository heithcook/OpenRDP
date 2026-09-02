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
    const QJsonArray savedEntries=document.isArray()?document.array():document.object().value(QStringLiteral("entries")).toArray();

    QVector<ConnectionHistoryEntry> entries;
    for (const QJsonValue& value : savedEntries) {
        const QJsonObject object = value.toObject();
        ConnectionHistoryEntry entry;
        entry.server = object.value(QStringLiteral("server")).toString().trimmed();
        entry.username = object.value(QStringLiteral("username")).toString();
        entry.authenticationMode = object.value(QStringLiteral("webAccount")).toBool()
            ? AuthenticationMode::EntraWebAccount : AuthenticationMode::NlaPassword;
        entry.lastConnectedAt=QDateTime::fromString(object.value(QStringLiteral("lastConnectedAt")).toString(),Qt::ISODateWithMs);
        entry.profileId=object.value(QStringLiteral("profileId")).toString();
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
    ConnectionHistoryEntry recorded=entry;if(!recorded.lastConnectedAt.isValid())recorded.lastConnectedAt=QDateTime::currentDateTimeUtc();entries.prepend(recorded);
    if (entries.size() > maximumEntries) entries.resize(maximumEntries);

    return save(entries);
}

bool ConnectionHistory::remove(const ConnectionHistoryEntry& entry) const
{
    auto entries=load();entries.removeIf([&entry](const auto& existing){return existing.server.compare(entry.server,Qt::CaseInsensitive)==0&&existing.username==entry.username&&existing.authenticationMode==entry.authenticationMode;});return save(entries);
}

bool ConnectionHistory::clear() const
{
    if(!QFile::exists(filePath_))return true;
    return QFile::remove(filePath_);
}

bool ConnectionHistory::save(const QVector<ConnectionHistoryEntry>& entries) const
{
    const QFileInfo info(filePath_);
    if (!QDir().mkpath(info.absolutePath())) return false;
    QJsonArray array;
    for (const ConnectionHistoryEntry& saved : entries) {
        array.append(QJsonObject{{QStringLiteral("server"), saved.server},
            {QStringLiteral("username"), saved.username},
            {QStringLiteral("webAccount"), saved.authenticationMode == AuthenticationMode::EntraWebAccount},
            {QStringLiteral("lastConnectedAt"),saved.lastConnectedAt.toUTC().toString(Qt::ISODateWithMs)},
            {QStringLiteral("profileId"),saved.profileId}});
    }
    QSaveFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const QJsonObject root{{QStringLiteral("version"),1},{QStringLiteral("entries"),array}};
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) return false;
    return file.commit();
}

} // namespace openrdp
