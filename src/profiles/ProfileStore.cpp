#include "profiles/ProfileStore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

namespace openrdp {
namespace {

QString defaultDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/openrdp/profiles");
}

QString authenticationName(AuthenticationMode mode)
{
    return mode == AuthenticationMode::EntraWebAccount
        ? QStringLiteral("entra-web-account") : QStringLiteral("nla");
}

QJsonObject toJson(const ConnectionProfile& profile)
{
    QJsonArray folders;
    for (const auto& folder : profile.resources.folders) {
        folders.append(QJsonObject{{QStringLiteral("enabled"), folder.enabled},
            {QStringLiteral("remoteName"), folder.remoteName},
            {QStringLiteral("localPath"), folder.localPath}});
    }
    QJsonArray monitors;
    for (int monitor : profile.display.selectedMonitors) monitors.append(monitor);
    QJsonArray printerNames;
    for (const auto& name : profile.resources.printerNames) printerNames.append(name);
    return QJsonObject{
        {QStringLiteral("version"), ConnectionProfile::currentSchemaVersion},
        {QStringLiteral("id"), profile.id}, {QStringLiteral("name"), profile.name},
        {QStringLiteral("server"), profile.server}, {QStringLiteral("username"), profile.username},
        {QStringLiteral("authentication"), authenticationName(profile.authenticationMode)},
        {QStringLiteral("favorite"), profile.favorite},
        {QStringLiteral("lastConnectedAt"), profile.lastConnectedAt.toUTC().toString(Qt::ISODateWithMs)},
        {QStringLiteral("display"), QJsonObject{
            {QStringLiteral("mode"), profile.display.mode == DisplayMode::MultipleMonitors ? QStringLiteral("multiple") : QStringLiteral("single")},
            {QStringLiteral("resizeBehavior"), profile.display.resizeBehavior == ResizeBehavior::Dynamic ? QStringLiteral("dynamic") : profile.display.resizeBehavior == ResizeBehavior::ScaleToFit ? QStringLiteral("scale") : QStringLiteral("fixed")},
            {QStringLiteral("fullScreen"), profile.display.fullScreen},
            {QStringLiteral("width"), static_cast<qint64>(profile.display.fixedWidth)},
            {QStringLiteral("height"), static_cast<qint64>(profile.display.fixedHeight)},
            {QStringLiteral("selectedMonitors"), monitors}}},
        {QStringLiteral("resources"), QJsonObject{
            {QStringLiteral("clipboard"), profile.resources.clipboard == ClipboardSharing::Bidirectional ? QStringLiteral("bidirectional") : QStringLiteral("disabled")},
            {QStringLiteral("audioPlayback"), profile.resources.audioPlayback == AudioPlayback::Local ? QStringLiteral("local") : profile.resources.audioPlayback == AudioPlayback::Remote ? QStringLiteral("remote") : QStringLiteral("disabled")},
            {QStringLiteral("audioOutputDevice"), profile.resources.audioOutputDevice},
            {QStringLiteral("audioInputDevice"), profile.resources.audioInputDevice},
            {QStringLiteral("microphone"), profile.resources.microphone},
            {QStringLiteral("printers"), profile.resources.printers},
            {QStringLiteral("printerNames"), printerNames},
            {QStringLiteral("folders"), folders}}}
    };
}

ConnectionProfile fromJson(const QJsonObject& root)
{
    ConnectionProfile profile;
    const int version = root.value(QStringLiteral("version")).toInt(0);
    profile.version = ConnectionProfile::currentSchemaVersion;
    profile.id = root.value(QStringLiteral("id")).toString();
    profile.name = root.value(QStringLiteral("name")).toString();
    profile.server = root.value(version == 0 ? QStringLiteral("host") : QStringLiteral("server")).toString();
    profile.username = root.value(version == 0 ? QStringLiteral("user") : QStringLiteral("username")).toString();
    if ((version == 0 && root.value(QStringLiteral("webAccount")).toBool()) ||
        root.value(QStringLiteral("authentication")).toString() == QStringLiteral("entra-web-account"))
        profile.authenticationMode = AuthenticationMode::EntraWebAccount;
    profile.favorite = root.value(QStringLiteral("favorite")).toBool();
    profile.lastConnectedAt = QDateTime::fromString(root.value(QStringLiteral("lastConnectedAt")).toString(), Qt::ISODateWithMs);

    const QJsonObject display = root.value(QStringLiteral("display")).toObject();
    profile.display.mode = display.value(QStringLiteral("mode")).toString() == QStringLiteral("multiple")
        ? DisplayMode::MultipleMonitors : DisplayMode::SingleMonitor;
    const QString resize = display.value(QStringLiteral("resizeBehavior")).toString();
    profile.display.resizeBehavior = resize == QStringLiteral("scale") ? ResizeBehavior::ScaleToFit :
        resize == QStringLiteral("fixed") ? ResizeBehavior::Fixed : ResizeBehavior::Dynamic;
    profile.display.fullScreen = display.value(QStringLiteral("fullScreen")).toBool();
    const int width = display.value(QStringLiteral("width")).toInt(1920);
    const int height = display.value(QStringLiteral("height")).toInt(1080);
    if (width > 0) profile.display.fixedWidth = static_cast<quint32>(width);
    if (height > 0) profile.display.fixedHeight = static_cast<quint32>(height);
    for (const auto monitor : display.value(QStringLiteral("selectedMonitors")).toArray())
        if (monitor.toInt(-1) >= 0) profile.display.selectedMonitors.append(monitor.toInt());

    const QJsonObject resources = root.value(QStringLiteral("resources")).toObject();
    profile.resources.clipboard = resources.value(QStringLiteral("clipboard")).toString(QStringLiteral("bidirectional")) == QStringLiteral("disabled")
        ? ClipboardSharing::Disabled : ClipboardSharing::Bidirectional;
    const QString audio = resources.value(QStringLiteral("audioPlayback")).toString(QStringLiteral("local"));
    profile.resources.audioPlayback = audio == QStringLiteral("remote") ? AudioPlayback::Remote :
        audio == QStringLiteral("disabled") ? AudioPlayback::Disabled : AudioPlayback::Local;
    profile.resources.audioOutputDevice = resources.value(QStringLiteral("audioOutputDevice")).toString();
    profile.resources.audioInputDevice = resources.value(QStringLiteral("audioInputDevice")).toString();
    profile.resources.microphone = resources.value(QStringLiteral("microphone")).toBool();
    profile.resources.printers = resources.value(QStringLiteral("printers")).toBool();
    for (const auto value : resources.value(QStringLiteral("printerNames")).toArray()) {
        const QString name = value.toString(); if (!name.isEmpty()) profile.resources.printerNames.append(name);
    }
    for (const auto value : resources.value(QStringLiteral("folders")).toArray()) {
        const QJsonObject folder = value.toObject();
        const QString path = folder.value(QStringLiteral("localPath")).toString();
        if (!path.isEmpty()) profile.resources.folders.append({folder.value(QStringLiteral("enabled")).toBool(true),
            folder.value(QStringLiteral("remoteName")).toString(), path});
    }
    return profile;
}

} // namespace

ProfileStore::ProfileStore(QString directoryPath)
    : directoryPath_(directoryPath.isEmpty() ? defaultDirectory() : std::move(directoryPath))
{
}

bool ProfileStore::validId(const QString& id)
{
    static const QRegularExpression expression(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$"));
    return expression.match(id).hasMatch();
}

QStringList ProfileStore::profileIds() const
{
    QDir directory(directoryPath_);
    QStringList ids;
    for (const QString& file : directory.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name))
        ids.append(file.chopped(5));
    return ids;
}

std::optional<ConnectionProfile> ProfileStore::load(const QString& id, QString* error) const
{
    if (!validId(id)) { if (error) *error = QStringLiteral("Invalid profile identifier."); return std::nullopt; }
    QFile file(QDir(directoryPath_).filePath(id + QStringLiteral(".json")));
    if (!file.open(QIODevice::ReadOnly)) { if (error) *error = file.errorString(); return std::nullopt; }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("Invalid profile JSON: %1").arg(parseError.errorString());
        return std::nullopt;
    }
    const int version = document.object().value(QStringLiteral("version")).toInt(0);
    if (version < 0 || version > ConnectionProfile::currentSchemaVersion) {
        if (error) *error = QStringLiteral("Unsupported profile schema version %1.").arg(version);
        return std::nullopt;
    }
    ConnectionProfile profile = fromJson(document.object());
    profile.id = id;
    return profile;
}

bool ProfileStore::save(ConnectionProfile& profile, QString* error) const
{
    if (profile.id.isEmpty()) profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!validId(profile.id)) { if (error) *error = QStringLiteral("Invalid profile identifier."); return false; }
    if (profile.server.trimmed().isEmpty()) { if (error) *error = QStringLiteral("A profile requires a server."); return false; }
    if (!QDir().mkpath(directoryPath_)) { if (error) *error = QStringLiteral("Could not create the profile directory."); return false; }
    profile.version = ConnectionProfile::currentSchemaVersion;
    QSaveFile file(QDir(directoryPath_).filePath(profile.id + QStringLiteral(".json")));
    if (!file.open(QIODevice::WriteOnly)) { if (error) *error = file.errorString(); return false; }
    if (file.write(QJsonDocument(toJson(profile)).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool ProfileStore::remove(const QString& id, QString* error) const
{
    if (!validId(id)) { if (error) *error = QStringLiteral("Invalid profile identifier."); return false; }
    QFile file(QDir(directoryPath_).filePath(id + QStringLiteral(".json")));
    if (!file.exists()) return true;
    if (!file.remove()) { if (error) *error = file.errorString(); return false; }
    return true;
}

} // namespace openrdp
