#pragma once

#include <QString>
#include <QMetaType>
#include <cstdint>

namespace openrdp {
enum class RdpErrorType { DnsFailure, ConnectionRefused, ConnectionTimeout, TlsFailure,
    CertificateFailure, AuthenticationFailure, ProtocolFailure, ServerDisconnected,
    NetworkFailure, InternalError };

struct RdpError {
    RdpErrorType type = RdpErrorType::InternalError;
    QString userMessage;
    QString technicalDetails;
    std::uint32_t nativeErrorCode = 0;
};

RdpError translateError(std::uint32_t code, const QString& server, quint16 port);
} // namespace openrdp

Q_DECLARE_METATYPE(openrdp::RdpError)
