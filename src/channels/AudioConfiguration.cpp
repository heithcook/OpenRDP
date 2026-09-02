#include "channels/AudioConfiguration.h"
namespace openrdp {
namespace {QByteArray deviceArgument(const QString& device){return QByteArrayLiteral("dev:")+device.toUtf8();}}
QVector<QByteArray> rdpsndArguments(const AudioConfiguration& configuration){
    if(configuration.output!=RemoteAudioMode::Local)return {};
    QVector<QByteArray> result{QByteArrayLiteral("rdpsnd"),QByteArrayLiteral("sys:pulse")};
    if(!configuration.outputDevice.trimmed().isEmpty())result.append(deviceArgument(configuration.outputDevice.trimmed()));
    return result;
}
QVector<QByteArray> audinArguments(const AudioConfiguration& configuration){
    if(!configuration.microphone)return {};
    QVector<QByteArray> result{QByteArrayLiteral("audin"),QByteArrayLiteral("sys:pulse")};
    if(!configuration.inputDevice.trimmed().isEmpty())result.append(deviceArgument(configuration.inputDevice.trimmed()));
    return result;
}
}
