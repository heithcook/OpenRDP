#include "channels/PrinterRedirection.h"
#include <QRegularExpression>
#include <QSet>
#include <cups/cups.h>
namespace openrdp {
QVector<PrinterInfo> discoverCupsPrinters(){cups_dest_t* destinations=nullptr;const int count=cupsGetDests(&destinations);QVector<PrinterInfo> result;result.reserve(count);for(int i=0;i<count;++i){const auto& dest=destinations[i];const QString base=QString::fromUtf8(dest.name);const QString instance=dest.instance?QString::fromUtf8(dest.instance):QString();const QString name=instance.isEmpty()?base:base+u'/'+instance;const QString display=instance.isEmpty()?base:QStringLiteral("%1 / %2").arg(base,instance);const auto valid=validatePrinter({name,display,dest.is_default!=0});if(valid)result.append(*valid);}cupsFreeDests(count,destinations);return result;}
std::optional<PrinterInfo> validatePrinter(const PrinterInfo& printer,QString* error){const QString name=printer.name.trimmed();if(name.isEmpty()||name.size()>255||name.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f,]")))){if(error)*error=QStringLiteral("The CUPS printer name is not safe for redirection.");return std::nullopt;}PrinterInfo result=printer;result.name=name;if(result.displayName.trimmed().isEmpty())result.displayName=name;return result;}
bool uniquePrinters(const QVector<PrinterInfo>& printers,QString* error){QSet<QString> names;for(const auto& printer:printers){const QString key=printer.name.toCaseFolded();if(names.contains(key)){if(error)*error=QStringLiteral("Each redirected printer must be unique.");return false;}names.insert(key);}return true;}
QVector<QByteArray> printerArguments(const PrinterInfo& printer)
{
    QVector<QByteArray> args{QByteArrayLiteral("printer"),printer.name.toUtf8()};
    if(printer.isDefault){
        // FreeRDP interprets a present-but-empty third argument as an
        // explicitly blank Windows driver name. Use its CUPS fallback driver
        // when a fourth positional "default" flag is required.
        args.append(QByteArrayLiteral("MS Publisher Imagesetter"));
        args.append(QByteArrayLiteral("default"));
    }
    return args;
}
}
