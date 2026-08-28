#include "rdp/RdpSession.h"
#include "rdp/RdpContext.h"
#include "rdp/RdpInput.h"

#include <QMutexLocker>
#include <QUrl>
#include <freerdp/client.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/input.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/input.h>
#include <array>
#include <cstdarg>

namespace openrdp {
namespace {
RdpSession* session(freerdp* instance) {
    return instance && instance->context ? reinterpret_cast<OpenRdpContext*>(instance->context)->session : nullptr;
}
}

RdpSession::RdpSession(QObject* parent) : QObject(parent) {}
RdpSession::~RdpSession() { cleanup(); }

void RdpSession::setState(const SessionState state) { state_ = state; emit stateChanged(state); }

void RdpSession::connectToServer(const ConnectionSettings& settings)
{
    if (state_ != SessionState::Disconnected && state_ != SessionState::Failed) return;
    settings_ = settings;
    cancelRequested_ = false;
    setState(SessionState::Connecting);

    instance_ = freerdp_new();
    if (!instance_) {
        setState(SessionState::Failed);
        emit connectionError({RdpErrorType::InternalError, QStringLiteral("The RDP client could not be initialized."), {}, 0});
        return;
    }
    instance_->ContextSize = sizeof(OpenRdpContext);
    instance_->PreConnect = &RdpSession::preConnect;
    instance_->PostConnect = &RdpSession::postConnect;
    instance_->PostDisconnect = &RdpSession::postDisconnect;
    instance_->AuthenticateEx = &RdpSession::authenticate;
    instance_->VerifyCertificateEx = &RdpSession::verifyCertificate;
    instance_->GetAccessToken = &RdpSession::getAccessToken;
    if (!freerdp_context_new(instance_)) {
        emit connectionError({RdpErrorType::InternalError, QStringLiteral("The RDP context could not be created."), {}, 0});
        cleanup(); setState(SessionState::Failed); return;
    }
    reinterpret_cast<OpenRdpContext*>(instance_->context)->session = this;
    auto* s = instance_->context->settings;
    const QByteArray host = settings.hostname.toUtf8();
    const QByteArray user = settings.username.toUtf8();
    const QByteArray domain = settings.domain.toUtf8();
    const bool configured = freerdp_settings_set_string(s, FreeRDP_ServerHostname, host.constData()) &&
        freerdp_settings_set_uint32(s, FreeRDP_ServerPort, settings.port) &&
        freerdp_settings_set_string(s, FreeRDP_Username, user.constData()) &&
        freerdp_settings_set_string(s, FreeRDP_Domain, domain.constData()) &&
        freerdp_settings_set_uint32(s, FreeRDP_DesktopWidth, settings.desktopWidth) &&
        freerdp_settings_set_uint32(s, FreeRDP_DesktopHeight, settings.desktopHeight) &&
        freerdp_settings_set_uint32(s, FreeRDP_ColorDepth, 32) &&
        freerdp_settings_set_bool(s, FreeRDP_AadSecurity, settings.authenticationMode == AuthenticationMode::EntraWebAccount) &&
        freerdp_settings_set_bool(s, FreeRDP_NlaSecurity, settings.authenticationMode == AuthenticationMode::NlaPassword && settings.enableNla) &&
        freerdp_settings_set_bool(s, FreeRDP_TlsSecurity, settings.authenticationMode == AuthenticationMode::NlaPassword && settings.enableTls) &&
        freerdp_settings_set_bool(s, FreeRDP_RdpSecurity, false) &&
        freerdp_settings_set_bool(s, FreeRDP_SoftwareGdi, true);
    if (!configured || !freerdp_connect(instance_)) {
        const auto error = translateError(freerdp_get_last_error(instance_->context), settings.hostname, settings.port);
        cleanup(); setState(SessionState::Failed); emit connectionError(error); emit disconnected(); return;
    }
    setState(SessionState::Connected);
    emit connected({settings.desktopWidth, settings.desktopHeight});

    while (!cancelRequested_ && !freerdp_shall_disconnect_context(instance_->context)) {
        std::array<HANDLE, 64> handles{};
        const DWORD count = freerdp_get_event_handles(instance_->context, handles.data(), handles.size());
        if (count == 0 || count > handles.size()) break;
        const DWORD status = WaitForMultipleObjects(count, handles.data(), FALSE, 100);
        if (status == WAIT_FAILED || (status != WAIT_TIMEOUT && !freerdp_check_event_handles(instance_->context))) break;
        flushInput();
    }
    setState(SessionState::Disconnecting);
    if (instance_) freerdp_disconnect(instance_);
    cleanup(); setState(SessionState::Disconnected); emit disconnected();
}

void RdpSession::disconnect()
{
    cancelRequested_ = true;
    {
        QMutexLocker instanceLock(&instanceMutex_);
        if (instance_ && instance_->context) freerdp_abort_connect_context(instance_->context);
    }
    QMutexLocker lock(&responseMutex_);
    responseAccepted_ = false; responsePending_ = false; responseReady_.wakeAll();
}

void RdpSession::cleanup()
{
    QMutexLocker instanceLock(&instanceMutex_);
    if (!instance_) return;
    if (instance_->context) renderer_.shutdown(instance_->context);
    freerdp_context_free(instance_);
    freerdp_free(instance_);
    instance_ = nullptr;
}

BOOL RdpSession::preConnect(freerdp*) { return TRUE; }
BOOL RdpSession::postConnect(freerdp* instance)
{
    auto* self = session(instance);
    if (!self || !self->renderer_.initialize(instance->context)) return FALSE;
    instance->context->update->BeginPaint = &RdpSession::beginPaint;
    instance->context->update->EndPaint = &RdpSession::endPaint;
    return TRUE;
}
void RdpSession::postDisconnect(freerdp*) {}
BOOL RdpSession::beginPaint(rdpContext*) { return TRUE; }
BOOL RdpSession::endPaint(rdpContext* context)
{
    auto* self = session(context->instance);
    if (!self || !context->gdi) return FALSE;
    const QRect damage(0, 0, context->gdi->width, context->gdi->height);
    const QImage frame(context->gdi->primary_buffer, context->gdi->width, context->gdi->height,
        context->gdi->stride, QImage::Format_ARGB32);
    emit self->frameUpdated(frame.copy(), damage);
    return TRUE;
}

BOOL RdpSession::authenticate(freerdp* instance, char** username, char** password, char** domain, rdp_auth_reason)
{
    try { auto* self = session(instance); return self && self->awaitCredentials(username, password, domain); }
    catch (...) { return FALSE; }
}

bool RdpSession::awaitCredentials(char** username, char** password, char** domain)
{
    setState(SessionState::Authenticating);
    { QMutexLocker lock(&responseMutex_); responsePending_ = true; responseAccepted_ = false; }
    emit authenticationRequired(settings_.hostname, settings_.domain.isEmpty() ? settings_.username : settings_.domain + u'\\' + settings_.username);
    QMutexLocker lock(&responseMutex_);
    while (responsePending_ && !cancelRequested_) responseReady_.wait(&responseMutex_);
    if (!responseAccepted_ || cancelRequested_) return false;
    const QByteArray user = responseUsername_.toUtf8();
    const QByteArray pass = responsePassword_.toUtf8();
    QString parsedUser, parsedDomain; parseUsername(responseUsername_, parsedUser, parsedDomain);
    const QByteArray parsedUserBytes = parsedUser.toUtf8();
    const QByteArray domainBytes = parsedDomain.toUtf8();
    free(*username); free(*password); free(*domain);
    *username = _strdup(parsedUserBytes.constData());
    *password = _strdup(pass.constData());
    *domain = _strdup(domainBytes.constData());
    responsePassword_.fill(QChar(u'\0')); responsePassword_.clear();
    return *username && *password && *domain;
}

void RdpSession::provideCredentials(const QString& username, const QString& password, const bool accepted)
{
    QMutexLocker lock(&responseMutex_); responseUsername_ = username; responsePassword_ = password;
    responseAccepted_ = accepted; responsePending_ = false; responseReady_.wakeAll();
}

DWORD RdpSession::verifyCertificate(freerdp* instance, const char* host, UINT16 port,
    const char* commonName, const char* subject, const char* issuer, const char* fingerprint, DWORD flags)
{
    try {
        CertificateInfo info{QString::fromUtf8(host), port, QString::fromUtf8(commonName),
            QString::fromUtf8(subject), QString::fromUtf8(issuer), QString::fromUtf8(fingerprint),
            flags ? QStringLiteral("The certificate is untrusted, mismatched, expired, or otherwise invalid (flags 0x%1).").arg(flags, 0, 16) : QStringLiteral("The certificate is not trusted.")};
        auto* self = session(instance); return self ? self->awaitCertificate(info) : 0;
    } catch (...) { return 0; }
}

DWORD RdpSession::awaitCertificate(const CertificateInfo& info)
{
    setState(SessionState::CertificateVerification);
    { QMutexLocker lock(&responseMutex_); responsePending_ = true; responseAccepted_ = false; }
    emit certificateVerificationRequired(info);
    QMutexLocker lock(&responseMutex_);
    while (responsePending_ && !cancelRequested_) responseReady_.wait(&responseMutex_);
    return responseAccepted_ && !cancelRequested_ ? 2UL : 0UL;
}
void RdpSession::provideCertificateDecision(const bool accepted)
{ QMutexLocker lock(&responseMutex_); responseAccepted_ = accepted; responsePending_ = false; responseReady_.wakeAll(); }

BOOL RdpSession::getAccessToken(freerdp* instance, const AccessTokenType type, char** token,
    const size_t count, ...)
{
    try {
        if (!instance || !instance->context || !token || type != ACCESS_TOKEN_TYPE_AAD || count < 2)
            return FALSE;
        va_list args;
        va_start(args, count);
        const char* scope = va_arg(args, const char*);
        const char* reqCnf = va_arg(args, const char*);
        va_end(args);
        if (!scope || !reqCnf) return FALSE;

        auto* self = session(instance);
        if (!self) return FALSE;
        auto* freerdpSettings = instance->context->settings;
        const BOOL previous = freerdp_settings_get_bool(freerdpSettings, FreeRDP_UseCommonStdioCallbacks);
        if (!freerdp_settings_set_bool(freerdpSettings, FreeRDP_UseCommonStdioCallbacks, TRUE)) return FALSE;
        char* request = freerdp_client_get_aad_url(
            reinterpret_cast<rdpClientContext*>(instance->context),
            FREERDP_CLIENT_AAD_AUTH_REQUEST, scope);
        (void)freerdp_settings_set_bool(freerdpSettings, FreeRDP_UseCommonStdioCallbacks, previous);
        if (!request) return FALSE;
        const QString authorizationUrl = QString::fromUtf8(request);
        free(request);

        QString redirectUrl;
        if (!self->awaitWebAuthentication(authorizationUrl, redirectUrl)) return FALSE;
        const auto code = authorizationCodeFromRedirect(redirectUrl);
        if (!code) return FALSE;
        const QByteArray codeBytes = code->toUtf8();

        if (!freerdp_settings_set_bool(freerdpSettings, FreeRDP_UseCommonStdioCallbacks, TRUE)) return FALSE;
        char* tokenRequest = freerdp_client_get_aad_url(
            reinterpret_cast<rdpClientContext*>(instance->context),
            FREERDP_CLIENT_AAD_TOKEN_REQUEST, scope, codeBytes.constData(), reqCnf);
        (void)freerdp_settings_set_bool(freerdpSettings, FreeRDP_UseCommonStdioCallbacks, previous);
        if (!tokenRequest) return FALSE;
        const BOOL result = client_common_get_access_token(instance, tokenRequest, token);
        free(tokenRequest);
        return result && *token;
    } catch (...) { return FALSE; }
}

bool RdpSession::awaitWebAuthentication(const QString& authorizationUrl, QString& redirectUrl)
{
    setState(SessionState::Authenticating);
    { QMutexLocker lock(&responseMutex_); responsePending_ = true; responseAccepted_ = false; responseRedirectUrl_.clear(); }
    emit webAuthenticationRequired(authorizationUrl);
    QMutexLocker lock(&responseMutex_);
    while (responsePending_ && !cancelRequested_) responseReady_.wait(&responseMutex_);
    if (!responseAccepted_ || cancelRequested_) return false;
    redirectUrl = responseRedirectUrl_;
    responseRedirectUrl_.clear();
    return !redirectUrl.isEmpty();
}

void RdpSession::provideWebAuthenticationResult(const QString& redirectUrl, const bool accepted)
{
    QMutexLocker lock(&responseMutex_);
    responseRedirectUrl_ = redirectUrl;
    responseAccepted_ = accepted;
    responsePending_ = false;
    responseReady_.wakeAll();
}

QImage RdpSession::currentFrame() const
{
    if (!instance_ || !instance_->context || !instance_->context->gdi) return {};
    auto* gdi = instance_->context->gdi;
    return QImage(gdi->primary_buffer, gdi->width, gdi->height, gdi->stride, QImage::Format_ARGB32).copy();
}

void RdpSession::sendMouse(quint16 flags, quint16 x, quint16 y)
{ QMutexLocker lock(&inputMutex_); inputEvents_.append({false, flags, x, y, false, false}); }
void RdpSession::sendKey(quint32 virtualKey, bool down, bool repeat)
{
    if (virtualKey == 0) return;
    QMutexLocker lock(&inputMutex_); inputEvents_.append({true, virtualKey, 0, 0, down, repeat});
}
void RdpSession::flushInput()
{
    if (!instance_ || !instance_->context || !instance_->context->input) return;
    QVector<InputEvent> events;
    { QMutexLocker lock(&inputMutex_); events.swap(inputEvents_); }
    for (const auto& event : events) {
        if (event.key) {
            const UINT32 scan = virtualKeyToRdpScancode(event.first);
            freerdp_input_send_keyboard_event_ex(instance_->context->input, event.down, event.repeat, scan);
        } else {
            freerdp_input_send_mouse_event(instance_->context->input, static_cast<UINT16>(event.first), event.x, event.y);
        }
    }
}
} // namespace openrdp
