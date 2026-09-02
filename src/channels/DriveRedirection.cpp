#include "channels/DriveRedirection.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
namespace openrdp {
std::optional<RedirectedFolderConfig> validateRedirectedFolder(const QString& inputName,const QString& path,QString* error){
    const QString name=inputName.trimmed();static const QRegularExpression allowed(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,31}$"));
    if(!allowed.match(name).hasMatch()||name==QStringLiteral(".")||name==QStringLiteral("..")){if(error)*error=QStringLiteral("Use a unique 1–32 character name containing letters, numbers, dots, underscores, or hyphens.");return std::nullopt;}
    QFileInfo info(path);if(!info.exists()||!info.isDir()||!info.isReadable()||!info.isAbsolute()){if(error)*error=QStringLiteral("Select an existing readable absolute folder.");return std::nullopt;}
    if(info.isSymLink()){if(error)*error=QStringLiteral("A symbolic link cannot be used as a redirected folder root.");return std::nullopt;}
    const QString canonical=info.canonicalFilePath();if(canonical.isEmpty()||canonical==QStringLiteral("/")){if(error)*error=QStringLiteral("The entire filesystem cannot be redirected.");return std::nullopt;}
    return RedirectedFolderConfig{name,QDir::cleanPath(canonical)};
}
bool uniqueFolderNames(const QVector<RedirectedFolderConfig>& folders,QString* error){QSet<QString> names;for(const auto& folder:folders){const QString key=folder.name.toCaseFolded();if(names.contains(key)){if(error)*error=QStringLiteral("Redirected folder names must be unique.");return false;}names.insert(key);}return true;}
QVector<QByteArray> driveArguments(const RedirectedFolderConfig& folder){return {QByteArrayLiteral("drive"),folder.name.toUtf8(),folder.canonicalPath.toUtf8()};}
}
