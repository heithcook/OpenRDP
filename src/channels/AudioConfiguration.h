#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>
namespace openrdp {
enum class RemoteAudioMode{Local,Remote,Disabled};
struct AudioConfiguration{RemoteAudioMode output=RemoteAudioMode::Local;bool microphone=false;QString outputDevice;QString inputDevice;};
QVector<QByteArray> rdpsndArguments(const AudioConfiguration& configuration);
QVector<QByteArray> audinArguments(const AudioConfiguration& configuration);
}
