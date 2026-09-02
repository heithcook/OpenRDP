#pragma once

#include "profiles/ConnectionProfile.h"
#include "profiles/RdpFile.h"

#include <QVector>

namespace openrdp {

enum class SensitiveResource {
    Clipboard,
    Microphone,
    LocalDrives,
    Printers
};

struct ResourceRedirectionRequest {
    SensitiveResource resource;
    QString sourceProperty;
    QString detail;
    bool operator==(const ResourceRedirectionRequest&) const = default;
};

struct RdpImportResult {
    ConnectionProfile profile;
    RdpFile source;
    QVector<ResourceRedirectionRequest> resourceRequests;
    QStringList unsupportedKeys;
    bool discardedPasswordProperty = false;
};

class RdpProfileMapper final {
public:
    RdpImportResult importFile(const RdpFile& file) const;
    RdpFile exportFile(const ConnectionProfile& profile, const RdpFile& preserved = {}) const;
    static bool isPasswordProperty(const QString& name);
};

} // namespace openrdp
