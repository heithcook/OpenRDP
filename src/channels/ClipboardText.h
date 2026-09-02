#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

namespace openrdp {
QByteArray encodeClipboardText(const QString& text);
std::optional<QString> decodeClipboardText(const QByteArray& data);
}
