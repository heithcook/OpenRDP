#pragma once

#include <QRect>
#include <QString>
#include <QVector>
#include <freerdp/settings_types.h>

namespace openrdp {

struct MonitorInfo {
    QString id;
    QRect pixelGeometry;
    QSize physicalMillimeters;
    qreal scaleFactor = 1.0;
    int orientationDegrees = 0;
    bool primary = false;
};

QVector<rdpMonitor> toFreeRdpMonitors(const QVector<MonitorInfo>& monitors);
bool validMonitorTopology(const QVector<MonitorInfo>& monitors, QString* error = nullptr);

} // namespace openrdp
