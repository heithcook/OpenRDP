#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>
#include <optional>
namespace openrdp {
struct PrinterInfo{QString name;QString displayName;bool isDefault=false;bool operator==(const PrinterInfo&)const=default;};
QVector<PrinterInfo> discoverCupsPrinters();
std::optional<PrinterInfo> validatePrinter(const PrinterInfo& printer,QString* error=nullptr);
bool uniquePrinters(const QVector<PrinterInfo>& printers,QString* error=nullptr);
QVector<QByteArray> printerArguments(const PrinterInfo& printer);
}
