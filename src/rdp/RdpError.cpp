#include "rdp/RdpError.h"

#include <freerdp/error.h>
#include <freerdp/freerdp.h>

namespace openrdp {
RdpError translateError(const std::uint32_t code, const QString& server, const quint16 port)
{
    RdpError result;
    result.nativeErrorCode = code;
    const char* name = freerdp_get_last_error_name(code);
    const char* detail = freerdp_get_last_error_string(code);
    result.technicalDetails = QStringLiteral("%1 (%2)\nServer: %3:%4")
        .arg(QString::fromUtf8(name ? name : "unknown"), QString::fromUtf8(detail ? detail : "unknown"),
             server, QString::number(port));
    switch (code) {
    case FREERDP_ERROR_AUTHENTICATION_FAILED:
    case FREERDP_ERROR_CONNECT_LOGON_FAILURE:
        result.type = RdpErrorType::AuthenticationFailure;
        result.userMessage = QStringLiteral("Authentication failed. The username or password was not accepted by the remote computer.");
        break;
    case FREERDP_ERROR_TLS_CONNECT_FAILED:
        result.type = RdpErrorType::TlsFailure;
        result.userMessage = QStringLiteral("A secure connection could not be established with the remote computer.");
        break;
    case FREERDP_ERROR_CONNECT_CANCELLED:
        result.type = RdpErrorType::NetworkFailure;
        result.userMessage = QStringLiteral("The connection was cancelled.");
        break;
    default:
        result.type = RdpErrorType::ProtocolFailure;
        result.userMessage = QStringLiteral("The remote desktop connection could not be established.");
        break;
    }
    return result;
}
} // namespace openrdp
