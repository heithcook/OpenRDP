#pragma once

#include "rdp/RdpSettings.h"
#include <QString>
#include <QVector>

namespace openrdp {

struct ConnectionHistoryEntry {
    QString server;
    QString username;
    AuthenticationMode authenticationMode = AuthenticationMode::NlaPassword;
    bool operator==(const ConnectionHistoryEntry&) const = default;
};

class ConnectionHistory final {
public:
    explicit ConnectionHistory(QString filePath = {});
    QVector<ConnectionHistoryEntry> load() const;
    bool record(const ConnectionHistoryEntry& entry) const;
    QString filePath() const { return filePath_; }

private:
    QString filePath_;
};

} // namespace openrdp
