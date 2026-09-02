#pragma once

#include "profiles/ConnectionProfile.h"

#include <QString>

#include <optional>

namespace openrdp {

class ProfileStore final {
public:
    explicit ProfileStore(QString directoryPath = {});

    QString directoryPath() const { return directoryPath_; }
    QStringList profileIds() const;
    std::optional<ConnectionProfile> load(const QString& id, QString* error = nullptr) const;
    bool save(ConnectionProfile& profile, QString* error = nullptr) const;
    bool remove(const QString& id, QString* error = nullptr) const;

private:
    QString directoryPath_;
    static bool validId(const QString& id);
};

} // namespace openrdp
