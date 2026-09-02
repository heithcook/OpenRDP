#include "channels/ClipboardText.h"

namespace openrdp {
QByteArray encodeClipboardText(const QString& input)
{
    QString text=input;text.replace(QStringLiteral("\r\n"),QStringLiteral("\n"));text.replace(u'\r',u'\n');text.replace(u'\n',QStringLiteral("\r\n"));
    QByteArray data;data.reserve((text.size()+1)*2);
    for(QChar character:text){const quint16 value=character.unicode();data.append(static_cast<char>(value&0xff));data.append(static_cast<char>(value>>8));}
    data.append('\0');data.append('\0');return data;
}
std::optional<QString> decodeClipboardText(const QByteArray& data)
{
    constexpr qsizetype maximum=16*1024*1024;
    if(data.size()<2||data.size()>maximum||(data.size()%2)!=0)return std::nullopt;
    QString text;text.reserve(data.size()/2);
    for(qsizetype offset=0;offset<data.size();offset+=2){
        const quint16 value=static_cast<unsigned char>(data[offset])|(static_cast<quint16>(static_cast<unsigned char>(data[offset+1]))<<8);
        if(value==0)break;
        text.append(QChar(value));
    }
    text.replace(QStringLiteral("\r\n"),QStringLiteral("\n"));text.replace(u'\r',u'\n');return text;
}
}
