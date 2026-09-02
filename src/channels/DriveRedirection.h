#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>
#include <optional>
namespace openrdp {
struct RedirectedFolderConfig{QString name;QString canonicalPath;bool operator==(const RedirectedFolderConfig&)const=default;};
std::optional<RedirectedFolderConfig> validateRedirectedFolder(const QString& name,const QString& path,QString* error=nullptr);
bool uniqueFolderNames(const QVector<RedirectedFolderConfig>& folders,QString* error=nullptr);
QVector<QByteArray> driveArguments(const RedirectedFolderConfig& folder);
}
