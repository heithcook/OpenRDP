#include "profiles/RdpProfileMapper.h"

#include <QSet>

#include <algorithm>

namespace openrdp {
namespace {

std::optional<int> integer(const RdpFile& file, const QString& name)
{
    const auto property = file.lastProperty(name);
    if (!property || property->type.toLower() != u'i') return std::nullopt;
    bool ok = false;
    const int value = property->value.toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}

QString string(const RdpFile& file, const QString& name)
{
    const auto property = file.lastProperty(name);
    return property && property->type.toLower() == u's' ? property->value : QString();
}

void request(QVector<ResourceRedirectionRequest>& requests, SensitiveResource resource,
    const QString& property, const QString& detail = {})
{
    if (std::none_of(requests.cbegin(), requests.cend(), [resource](const auto& existing) {
            return existing.resource == resource;
        })) {
        requests.append({resource, property, detail});
    }
}

void replace(RdpFile& file, const QString& name, QChar type, const QString& value)
{
    file.properties.removeIf([&name](const RdpProperty& property) {
        return property.name.compare(name, Qt::CaseInsensitive) == 0;
    });
    file.properties.append({name, type, value});
}

QString boolValue(bool value) { return value ? QStringLiteral("1") : QStringLiteral("0"); }

const QSet<QString>& supportedKeys()
{
    static const QSet<QString> keys{
        QStringLiteral("full address"), QStringLiteral("username"), QStringLiteral("domain"),
        QStringLiteral("screen mode id"), QStringLiteral("desktopwidth"),
        QStringLiteral("desktopheight"), QStringLiteral("session bpp"),
        QStringLiteral("use multimon"), QStringLiteral("selectedmonitors"),
        QStringLiteral("smart sizing"), QStringLiteral("dynamic resolution"),
        QStringLiteral("audiomode"), QStringLiteral("audiocapturemode"),
        QStringLiteral("redirectclipboard"), QStringLiteral("redirectprinters"),
        QStringLiteral("redirectdrives"),
        QStringLiteral("drivestoredirect"), QStringLiteral("prompt for credentials"),
        QStringLiteral("promptcredentialonce"), QStringLiteral("authentication level"),
        QStringLiteral("enablecredsspsupport"), QStringLiteral("networkautodetect"),
        QStringLiteral("bandwidthautodetect"), QStringLiteral("compression"),
        QStringLiteral("keyboardhook"), QStringLiteral("connection type"),
        QStringLiteral("disable wallpaper"), QStringLiteral("allow font smoothing"),
        QStringLiteral("allow desktop composition"), QStringLiteral("disable full window drag"),
        QStringLiteral("disable menu anims"), QStringLiteral("disable themes"),
        QStringLiteral("bitmapcachepersistenable")
    };
    return keys;
}

} // namespace

bool RdpProfileMapper::isPasswordProperty(const QString& name)
{
    const QString normalized = name.trimmed().toLower();
    return normalized == QStringLiteral("password") || normalized.startsWith(QStringLiteral("password "));
}

RdpImportResult RdpProfileMapper::importFile(const RdpFile& file) const
{
    RdpImportResult result;
    result.source = file;
    result.source.properties.removeIf([&result](const RdpProperty& property) {
        if (!isPasswordProperty(property.name)) return false;
        result.discardedPasswordProperty = true;
        return true;
    });

    result.profile.server = string(file, QStringLiteral("full address")).trimmed();
    const QString username = string(file, QStringLiteral("username"));
    const QString domain = string(file, QStringLiteral("domain"));
    result.profile.username = domain.isEmpty() || username.contains(u'\\') || username.contains(u'@')
        ? username : domain + u'\\' + username;

    if (const auto value = integer(file, QStringLiteral("screen mode id")))
        result.profile.display.fullScreen = *value == 2;
    if (const auto value = integer(file, QStringLiteral("desktopwidth")); value && *value > 0)
        result.profile.display.fixedWidth = static_cast<quint32>(*value);
    if (const auto value = integer(file, QStringLiteral("desktopheight")); value && *value > 0)
        result.profile.display.fixedHeight = static_cast<quint32>(*value);
    if (integer(file, QStringLiteral("use multimon")).value_or(0) != 0)
        result.profile.display.mode = DisplayMode::MultipleMonitors;

    const QString monitors = string(file, QStringLiteral("selectedmonitors"));
    for (const QString& item : monitors.split(u',', Qt::SkipEmptyParts)) {
        bool ok = false;
        const int monitor = item.trimmed().toInt(&ok);
        if (ok && monitor >= 0 && !result.profile.display.selectedMonitors.contains(monitor))
            result.profile.display.selectedMonitors.append(monitor);
    }
    if (integer(file, QStringLiteral("dynamic resolution")).value_or(0) != 0)
        result.profile.display.resizeBehavior = ResizeBehavior::Dynamic;
    else if (integer(file, QStringLiteral("smart sizing")).value_or(0) != 0)
        result.profile.display.resizeBehavior = ResizeBehavior::ScaleToFit;
    else
        result.profile.display.resizeBehavior = ResizeBehavior::Fixed;

    switch (integer(file, QStringLiteral("audiomode")).value_or(0)) {
    case 1: result.profile.resources.audioPlayback = AudioPlayback::Remote; break;
    case 2: result.profile.resources.audioPlayback = AudioPlayback::Disabled; break;
    default: result.profile.resources.audioPlayback = AudioPlayback::Local; break;
    }

    // Sensitive settings are requests, never grants. The profile remains inactive
    // until the import security review applies the user's choices.
    result.profile.resources.clipboard = ClipboardSharing::Disabled;
    if (integer(file, QStringLiteral("redirectclipboard")).value_or(0) != 0)
        request(result.resourceRequests, SensitiveResource::Clipboard,
            QStringLiteral("redirectclipboard"));
    if (integer(file, QStringLiteral("audiocapturemode")).value_or(0) != 0)
        request(result.resourceRequests, SensitiveResource::Microphone,
            QStringLiteral("audiocapturemode"));
    if (integer(file, QStringLiteral("redirectprinters")).value_or(0) != 0)
        request(result.resourceRequests, SensitiveResource::Printers,
            QStringLiteral("redirectprinters"));
    const QString drives = string(file, QStringLiteral("drivestoredirect"));
    if (integer(file, QStringLiteral("redirectdrives")).value_or(0) != 0 || !drives.isEmpty())
        request(result.resourceRequests, SensitiveResource::LocalDrives,
            drives.isEmpty() ? QStringLiteral("redirectdrives") : QStringLiteral("drivestoredirect"),
            drives);

    for (const RdpProperty& property : file.properties) {
        const QString key = property.name.trimmed().toLower();
        if (!supportedKeys().contains(key) && !isPasswordProperty(key) &&
            !result.unsupportedKeys.contains(property.name, Qt::CaseInsensitive))
            result.unsupportedKeys.append(property.name);
    }
    return result;
}

RdpFile RdpProfileMapper::exportFile(const ConnectionProfile& profile, const RdpFile& preserved) const
{
    RdpFile file = preserved;
    file.properties.removeIf([](const RdpProperty& property) { return isPasswordProperty(property.name); });
    replace(file, QStringLiteral("full address"), u's', profile.server);
    replace(file, QStringLiteral("username"), u's', profile.username);
    replace(file, QStringLiteral("screen mode id"), u'i',
        profile.display.fullScreen ? QStringLiteral("2") : QStringLiteral("1"));
    replace(file, QStringLiteral("desktopwidth"), u'i', QString::number(profile.display.fixedWidth));
    replace(file, QStringLiteral("desktopheight"), u'i', QString::number(profile.display.fixedHeight));
    replace(file, QStringLiteral("use multimon"), u'i', boolValue(profile.display.mode == DisplayMode::MultipleMonitors));
    QStringList monitors;
    for (int monitor : profile.display.selectedMonitors) monitors.append(QString::number(monitor));
    replace(file, QStringLiteral("selectedmonitors"), u's', monitors.join(u','));
    replace(file, QStringLiteral("smart sizing"), u'i', boolValue(profile.display.resizeBehavior == ResizeBehavior::ScaleToFit));
    replace(file, QStringLiteral("dynamic resolution"), u'i', boolValue(profile.display.resizeBehavior == ResizeBehavior::Dynamic));
    const int audioMode = profile.resources.audioPlayback == AudioPlayback::Local ? 0 :
        profile.resources.audioPlayback == AudioPlayback::Remote ? 1 : 2;
    replace(file, QStringLiteral("audiomode"), u'i', QString::number(audioMode));
    replace(file, QStringLiteral("audiocapturemode"), u'i', boolValue(profile.resources.microphone));
    replace(file, QStringLiteral("redirectclipboard"), u'i',
        boolValue(profile.resources.clipboard == ClipboardSharing::Bidirectional));
    replace(file, QStringLiteral("redirectprinters"), u'i', boolValue(profile.resources.printers));
    const bool hasEnabledFolder = std::any_of(profile.resources.folders.cbegin(),
        profile.resources.folders.cend(), [](const RedirectedFolder& folder) { return folder.enabled; });
    replace(file, QStringLiteral("redirectdrives"), u'i', boolValue(hasEnabledFolder));
    return file;
}

} // namespace openrdp
