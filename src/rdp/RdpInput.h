#pragma once

#include <QPoint>
#include <QSize>
#include <Qt>
#include <cstdint>

namespace openrdp {
QPoint scaleMousePosition(const QPoint& local, const QSize& localSize, const QSize& remoteSize);
std::uint32_t qtKeyToVirtualKey(Qt::Key key);
std::uint32_t virtualKeyToRdpScancode(std::uint32_t virtualKey);
} // namespace openrdp
