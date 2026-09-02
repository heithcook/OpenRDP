#pragma once

#include "rdp/RdpSettings.h"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

namespace openrdp {

enum class CapabilityState {
    NotRequested,
    Requested,
    LocallyUnavailable,
    ClientBlocked,
    ServerBlocked,
    Unsupported,
    Negotiated,
    Active
};

struct RdpCapability {
    bool requested = false;
    bool availableLocally = false;
    bool allowedByClient = false;
    bool negotiatedWithServer = false;
    bool active = false;
    CapabilityState state = CapabilityState::NotRequested;
    QString detail;
};

struct RdpCapabilities {
    RdpCapability clipboard;
    RdpCapability audioOutput;
    RdpCapability audioInput;
    RdpCapability driveRedirection;
    RdpCapability printers;
    RdpCapability dynamicResolution;
    RdpCapability multiMonitor;
};

enum class ClipboardSharing { Disabled, Bidirectional };
enum class AudioPlayback { Local, Remote, Disabled };
enum class ResizeBehavior { Dynamic, ScaleToFit, Fixed };
enum class DisplayMode { SingleMonitor, MultipleMonitors };

struct RedirectedFolder {
    bool enabled = true;
    QString remoteName;
    QString localPath;
};

struct ResourceSettings {
    ClipboardSharing clipboard = ClipboardSharing::Bidirectional;
    AudioPlayback audioPlayback = AudioPlayback::Local;
    QString audioOutputDevice;
    QString audioInputDevice;
    bool microphone = false;
    bool printers = false;
    QStringList printerNames;
    QVector<RedirectedFolder> folders;
};

struct DisplaySettings {
    DisplayMode mode = DisplayMode::SingleMonitor;
    ResizeBehavior resizeBehavior = ResizeBehavior::Dynamic;
    bool fullScreen = false;
    quint32 fixedWidth = 1920;
    quint32 fixedHeight = 1080;
    QVector<int> selectedMonitors;
};

struct ConnectionProfile {
    static constexpr int currentSchemaVersion = 1;

    int version = currentSchemaVersion;
    QString id;
    QString name;
    QString server;
    QString username;
    AuthenticationMode authenticationMode = AuthenticationMode::NlaPassword;
    DisplaySettings display;
    ResourceSettings resources;
    QDateTime lastConnectedAt;
    bool favorite = false;
};

} // namespace openrdp
