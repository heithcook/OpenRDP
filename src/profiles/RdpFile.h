#pragma once

#include <QByteArray>
#include <QChar>
#include <QString>
#include <QVector>

#include <optional>

namespace openrdp {

struct RdpProperty {
    QString name;
    QChar type;
    QString value;
    bool operator==(const RdpProperty&) const = default;
};

struct RdpFile {
    QVector<RdpProperty> properties;

    std::optional<RdpProperty> lastProperty(const QString& name) const;
};

struct RdpFileParseResult {
    RdpFile file;
    QString error;
    qsizetype errorLine = 0;

    explicit operator bool() const { return error.isEmpty(); }
};

class RdpFileParser final {
public:
    static constexpr qsizetype maximumFileSize = 1024 * 1024;
    static constexpr qsizetype maximumLineLength = 64 * 1024;

    RdpFileParseResult parse(const QByteArray& data) const;
};

class RdpFileWriter final {
public:
    QByteArray write(const RdpFile& file) const;
};

} // namespace openrdp
