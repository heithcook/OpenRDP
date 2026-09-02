#pragma once

#include "rdp/RdpSettings.h"
#include <QString>
#include <QDateTime>
#include <QVector>
#include <utility>

namespace openrdp {

struct ConnectionHistoryEntry {
    ConnectionHistoryEntry() = default;
    ConnectionHistoryEntry(QString serverValue, QString usernameValue, AuthenticationMode mode,
        QDateTime connectedAt = {}, QString associatedProfile = {})
        : server(std::move(serverValue)), username(std::move(usernameValue)), authenticationMode(mode),
          lastConnectedAt(std::move(connectedAt)), profileId(std::move(associatedProfile)) {}
    QString server;
    QString username;
    AuthenticationMode authenticationMode = AuthenticationMode::NlaPassword;
    QDateTime lastConnectedAt;
    QString profileId;
    bool operator==(const ConnectionHistoryEntry& other) const {
        return server==other.server&&username==other.username&&authenticationMode==other.authenticationMode&&profileId==other.profileId;
    }
};

class ConnectionHistory final {
public:
    explicit ConnectionHistory(QString filePath = {});
    QVector<ConnectionHistoryEntry> load() const;
    bool record(const ConnectionHistoryEntry& entry) const;
    bool remove(const ConnectionHistoryEntry& entry) const;
    bool clear() const;
    QString filePath() const { return filePath_; }

private:
    QString filePath_;
    bool save(const QVector<ConnectionHistoryEntry>& entries) const;
};

} // namespace openrdp
