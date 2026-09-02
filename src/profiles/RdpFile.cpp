#include "profiles/RdpFile.h"

#include <QRegularExpression>
#include <QtEndian>

#include <limits>

namespace openrdp {
namespace {

std::optional<QString> decode(const QByteArray& data, QString& error)
{
    if (data.size() > RdpFileParser::maximumFileSize) {
        error = QStringLiteral("The RDP file exceeds the 1 MiB safety limit.");
        return std::nullopt;
    }
    if (data.contains('\0') && !(data.size() >= 2 &&
            static_cast<unsigned char>(data[0]) == 0xFF &&
            static_cast<unsigned char>(data[1]) == 0xFE)) {
        error = QStringLiteral("The RDP file contains an unexpected null byte.");
        return std::nullopt;
    }
    if (data.size() >= 2 && static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xFE) {
        if ((data.size() - 2) % 2 != 0) {
            error = QStringLiteral("The UTF-16 RDP file has an incomplete code unit.");
            return std::nullopt;
        }
        QString text;
        text.reserve((data.size() - 2) / 2);
        for (qsizetype offset = 2; offset < data.size(); offset += 2) {
            const auto low = static_cast<unsigned char>(data[offset]);
            const auto high = static_cast<unsigned char>(data[offset + 1]);
            const char16_t codeUnit = static_cast<char16_t>(low | (high << 8));
            if (codeUnit == 0) {
                error = QStringLiteral("The RDP file contains an unexpected null character.");
                return std::nullopt;
            }
            text.append(QChar(codeUnit));
        }
        return text;
    }

    const QString text = QString::fromUtf8(data.constData(), data.size());
    if (text.toUtf8() != data) {
        error = QStringLiteral("The RDP file is neither valid UTF-8 nor UTF-16LE.");
        return std::nullopt;
    }
    return text.startsWith(QChar::ByteOrderMark) ? text.mid(1) : text;
}

bool validInteger(const QString& value)
{
    if (value.isEmpty()) return false;
    bool ok = false;
    (void)value.toLongLong(&ok, 10);
    return ok;
}

} // namespace

std::optional<RdpProperty> RdpFile::lastProperty(const QString& name) const
{
    for (auto iterator = properties.crbegin(); iterator != properties.crend(); ++iterator) {
        if (iterator->name.compare(name, Qt::CaseInsensitive) == 0) return *iterator;
    }
    return std::nullopt;
}

RdpFileParseResult RdpFileParser::parse(const QByteArray& data) const
{
    RdpFileParseResult result;
    const auto decoded = decode(data, result.error);
    if (!decoded) return result;

    const QStringList lines = decoded->split(QRegularExpression(QStringLiteral("\\r\\n|\\n|\\r")));
    qsizetype lineNumber = 0;
    for (const QString& line : lines) {
        ++lineNumber;
        if (line.isEmpty()) continue;
        if (line.size() > maximumLineLength) {
            result.error = QStringLiteral("An RDP property exceeds the 64 KiB line limit.");
            result.errorLine = lineNumber;
            return result;
        }
        const qsizetype firstColon = line.indexOf(u':');
        const qsizetype secondColon = firstColon < 0 ? -1 : line.indexOf(u':', firstColon + 1);
        if (firstColon <= 0 || secondColon != firstColon + 2) {
            result.error = QStringLiteral("Malformed RDP property.");
            result.errorLine = lineNumber;
            return result;
        }
        const QString name = line.left(firstColon).trimmed();
        const QChar type = line.at(firstColon + 1).toLower();
        const QString value = line.mid(secondColon + 1);
        if (name.isEmpty() || name.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f:]")))) {
            result.error = QStringLiteral("Invalid RDP property name.");
            result.errorLine = lineNumber;
            return result;
        }
        if (type == u'i' && !validInteger(value)) {
            result.error = QStringLiteral("Invalid integer RDP property.");
            result.errorLine = lineNumber;
            return result;
        }
        result.file.properties.append({name, type, value});
    }
    return result;
}

QByteArray RdpFileWriter::write(const RdpFile& file) const
{
    QString text;
    for (const RdpProperty& property : file.properties) {
        if (property.name.isEmpty() || property.name.contains(u':') || property.type.isNull()) continue;
        text += property.name + u':' + property.type + u':' + property.value + QStringLiteral("\r\n");
    }

    QByteArray bytes;
    bytes.reserve(2 + text.size() * 2);
    bytes.append(char(0xFF));
    bytes.append(char(0xFE));
    for (const QChar character : text) {
        const quint16 value = character.unicode();
        bytes.append(static_cast<char>(value & 0xFF));
        bytes.append(static_cast<char>(value >> 8));
    }
    return bytes;
}

} // namespace openrdp
