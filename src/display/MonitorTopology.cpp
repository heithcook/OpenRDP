#include "display/MonitorTopology.h"

#include <QSet>

namespace openrdp {

bool validMonitorTopology(const QVector<MonitorInfo>& monitors, QString* error)
{
    if (monitors.isEmpty() || monitors.size() > 16) {
        if (error) *error = QStringLiteral("Select between one and sixteen monitors.");
        return false;
    }
    int primaryCount = 0;
    QSet<QString> ids;
    for (const auto& monitor : monitors) {
        if (monitor.id.isEmpty() || ids.contains(monitor.id) || monitor.pixelGeometry.width() < 200 ||
            monitor.pixelGeometry.height() < 200 || monitor.pixelGeometry.width() > 8192 ||
            monitor.pixelGeometry.height() > 8192 ||
            (monitor.orientationDegrees != 0 && monitor.orientationDegrees != 90 &&
             monitor.orientationDegrees != 180 && monitor.orientationDegrees != 270)) {
            if (error) *error = QStringLiteral("The selected monitor topology is invalid.");
            return false;
        }
        ids.insert(monitor.id);
        if (monitor.primary) ++primaryCount;
    }
    if (primaryCount != 1) {
        if (error) *error = QStringLiteral("Exactly one selected monitor must be primary.");
        return false;
    }
    return true;
}

QVector<rdpMonitor> toFreeRdpMonitors(const QVector<MonitorInfo>& monitors)
{
    QVector<rdpMonitor> result;
    if (!validMonitorTopology(monitors)) return result;
    result.reserve(monitors.size());
    for (qsizetype index = 0; index < monitors.size(); ++index) {
        const auto& source = monitors.at(index);
        rdpMonitor target{};
        target.x = source.pixelGeometry.x(); target.y = source.pixelGeometry.y();
        target.width = source.pixelGeometry.width(); target.height = source.pixelGeometry.height();
        target.is_primary = source.primary ? 1U : 0U; target.orig_screen = static_cast<UINT32>(index);
        target.attributes.physicalWidth = static_cast<UINT32>(qBound(10, source.physicalMillimeters.width(), 10000));
        target.attributes.physicalHeight = static_cast<UINT32>(qBound(10, source.physicalMillimeters.height(), 10000));
        target.attributes.orientation = static_cast<UINT32>(source.orientationDegrees);
        target.attributes.desktopScaleFactor = static_cast<UINT32>(qBound(100, qRound(source.scaleFactor * 100.0), 500));
        target.attributes.deviceScaleFactor = target.attributes.desktopScaleFactor >= 180 ? 180U :
            target.attributes.desktopScaleFactor >= 140 ? 140U : 100U;
        result.append(target);
    }
    return result;
}

} // namespace openrdp
