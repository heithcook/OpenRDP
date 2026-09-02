#include "rdp/RdpSession.h"
#include "rdp/RdpContext.h"
#include "rdp/RdpInput.h"
#include "channels/ClipboardText.h"

#include <QMutexLocker>
#include <QUrl>
#include <freerdp/addin.h>
#include <freerdp/client.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/client/disp.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/channels/disp.h>
#include <freerdp/channels/cliprdr.h>
#include <freerdp/event.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/input.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/input.h>
#include <winpr/collections.h>
#include <winpr/user.h>
#include <array>
#include <cstdarg>
#include <cstring>

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
    instance_->LoadChannels = &RdpSession::loadChannels;
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
        freerdp_settings_set_bool(s, FreeRDP_DynamicResolutionUpdate, settings.dynamicResolution) &&
        freerdp_settings_set_bool(s, FreeRDP_SupportDisplayControl, settings.dynamicResolution) &&
        freerdp_settings_set_bool(s, FreeRDP_RedirectClipboard, settings.clipboard) &&
        freerdp_settings_set_bool(s, FreeRDP_AudioPlayback, settings.audio.output == RemoteAudioMode::Local) &&
        freerdp_settings_set_bool(s, FreeRDP_RemoteConsoleAudio, settings.audio.output == RemoteAudioMode::Remote) &&
        freerdp_settings_set_bool(s, FreeRDP_AudioCapture, settings.audio.microphone) &&
        freerdp_settings_set_bool(s, FreeRDP_DeviceRedirection, !settings.folders.isEmpty()||!settings.printers.isEmpty()) &&
        freerdp_settings_set_bool(s, FreeRDP_RedirectDrives, false) &&
        freerdp_settings_set_bool(s, FreeRDP_RedirectHomeDrive, false) &&
        freerdp_settings_set_bool(s, FreeRDP_AadSecurity, settings.authenticationMode == AuthenticationMode::EntraWebAccount) &&
        freerdp_settings_set_bool(s, FreeRDP_NlaSecurity, settings.authenticationMode == AuthenticationMode::NlaPassword && settings.enableNla) &&
        freerdp_settings_set_bool(s, FreeRDP_TlsSecurity, settings.authenticationMode == AuthenticationMode::NlaPassword && settings.enableTls) &&
        freerdp_settings_set_bool(s, FreeRDP_RdpSecurity, false) &&
        freerdp_settings_set_bool(s, FreeRDP_SoftwareGdi, true);
    bool monitorsConfigured = true;
    if (settings.monitors.size() > 1) {
        const auto monitors = toFreeRdpMonitors(settings.monitors);
        monitorsConfigured = !monitors.isEmpty() &&
            freerdp_settings_set_bool(s, FreeRDP_UseMultimon, true) &&
            freerdp_settings_set_bool(s, FreeRDP_ForceMultimon, false) &&
            freerdp_settings_set_monitor_def_array_sorted(s, monitors.constData(), static_cast<size_t>(monitors.size()));
    }
    if (!configured || !monitorsConfigured || !freerdp_connect(instance_)) {
        const auto error = translateError(freerdp_get_last_error(instance_->context), settings.hostname, settings.port);
        cleanup(); setState(SessionState::Failed); emit connectionError(error); emit disconnected(); return;
    }
    if (settings.clipboard) {
        auto* clip = static_cast<CliprdrClientContext*>(freerdp_channels_get_static_channel_interface(
            instance_->context->channels, CLIPRDR_SVC_CHANNEL_NAME));
        if (clip) {
            bindClipboardContext(clip);
        }
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
        flushDisplayResize();
        flushClipboard();
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
    { QMutexLocker displayLock(&displayMutex_); displayControl_ = nullptr; displayControlActive_ = false; pendingDisplaySize_ = {}; }
    { QMutexLocker clipboardLock(&clipboardMutex_); clipboardContext_ = nullptr; localClipboardText_.clear(); clipboardDirty_=false; clipboardHandshakeStarted_=false; clipboardServerReady_=false; }
    if (instance_->context) renderer_.shutdown(instance_->context);
    freerdp_context_free(instance_);
    freerdp_free(instance_);
    instance_ = nullptr;
}

BOOL RdpSession::preConnect(freerdp* instance)
{
    if (!instance || !instance->context || !instance->context->settings || !instance->context->channels)
        return FALSE;
    // FreeRDP initializes its channel manager immediately before PreConnect.
    // Subscribe here so the registrations survive that initialization and are
    // present before any static or dynamic channel is loaded.
    if (PubSub_SubscribeChannelConnected(instance->context->pubSub, &RdpSession::channelConnected) < 0 ||
        PubSub_SubscribeChannelDisconnected(instance->context->pubSub, &RdpSession::channelDisconnected) < 0)
        return FALSE;
    // freerdp_new()/freerdp_context_new() do not install the client-common
    // static add-in provider. Without it, distro builds with channels linked
    // into libfreerdp-client (rather than separate plugin .so files) cannot
    // resolve cliprdr, rdpdr, rdpsnd, audin, or disp during pre-connect.
    if (freerdp_register_addin_provider(freerdp_channels_load_static_addin_entry, 0) != CHANNEL_RC_OK)
        return FALSE;
    if (freerdp_settings_get_bool(instance->context->settings, FreeRDP_DynamicResolutionUpdate)) {
        const char* channel[] = { DISP_CHANNEL_NAME };
        if (!freerdp_client_add_dynamic_channel(instance->context->settings, 1, channel)) return FALSE;
    }
    if (freerdp_settings_get_bool(instance->context->settings, FreeRDP_RedirectClipboard)) {
        const char* channel[] = { CLIPRDR_CHANNEL_NAME };
        if (!freerdp_client_add_static_channel(instance->context->settings, 1, channel)) return FALSE;
    }
    auto* self=session(instance);
    if(!self)return FALSE;
    const auto sound=rdpsndArguments(self->settings_.audio);
    if(!sound.isEmpty()){QVector<const char*> args;for(const auto& value:sound)args.append(value.constData());if(!freerdp_client_add_static_channel(instance->context->settings,static_cast<size_t>(args.size()),args.constData()))return FALSE;}
    const auto microphone=audinArguments(self->settings_.audio);
    if(!microphone.isEmpty()){QVector<const char*> args;for(const auto& value:microphone)args.append(value.constData());if(!freerdp_client_add_dynamic_channel(instance->context->settings,static_cast<size_t>(args.size()),args.constData()))return FALSE;}
    if(!uniqueFolderNames(self->settings_.folders))return FALSE;
    for(const auto& folder:self->settings_.folders){const auto values=driveArguments(folder);QVector<const char*> args;for(const auto& value:values)args.append(value.constData());if(!freerdp_client_add_device_channel(instance->context->settings,static_cast<size_t>(args.size()),args.constData()))return FALSE;}
    if(!uniquePrinters(self->settings_.printers))return FALSE;
    for(const auto& printer:self->settings_.printers){const auto values=printerArguments(printer);QVector<const char*> args;for(const auto& value:values)args.append(value.constData());if(!freerdp_client_add_device_channel(instance->context->settings,static_cast<size_t>(args.size()),args.constData()))return FALSE;}
    return TRUE;
}
BOOL RdpSession::loadChannels(freerdp* instance)
{
    if (!instance || !instance->context || !instance->context->settings) return FALSE;
    return freerdp_client_load_channels(instance);
}
BOOL RdpSession::postConnect(freerdp* instance)
{
    auto* self = session(instance);
    if (!self || !self->renderer_.initialize(instance->context)) return FALSE;
    instance->context->update->BeginPaint = &RdpSession::beginPaint;
    instance->context->update->EndPaint = &RdpSession::endPaint;
    instance->context->update->DesktopResize = &RdpSession::desktopResize;
    return TRUE;
}
void RdpSession::postDisconnect(freerdp*) {}
void RdpSession::channelConnected(void* context, const ChannelConnectedEventArgs* event)
{
    if (!context || !event || !event->name || !event->pInterface) return;
    auto* rdp = static_cast<rdpContext*>(context);
    auto* self = session(rdp->instance);
    if (!self) return;
    if (std::strcmp(event->name, CLIPRDR_CHANNEL_NAME) == 0) {
        self->bindClipboardContext(static_cast<CliprdrClientContext*>(event->pInterface));
        return;
    }
    if (std::strcmp(event->name, DISP_DVC_CHANNEL_NAME) != 0 && std::strcmp(event->name, DISP_CHANNEL_NAME) != 0) return;
    QMutexLocker lock(&self->displayMutex_);
    self->displayControl_ = static_cast<DispClientContext*>(event->pInterface);
    self->displayControl_->custom = self;
    self->displayControl_->DisplayControlCaps = &RdpSession::displayControlCaps;
}
void RdpSession::bindClipboardContext(CliprdrClientContext* clip)
{
    if (!clip) return;
    QMutexLocker lock(&clipboardMutex_);
    clipboardContext_=clip;
    clip->custom=this;
    clip->MonitorReady=&RdpSession::clipboardMonitorReady;
    clip->ServerCapabilities=&RdpSession::clipboardServerCapabilities;
    clip->ServerFormatList=&RdpSession::clipboardServerFormatList;
    clip->ServerFormatListResponse=&RdpSession::clipboardServerFormatListResponse;
    clip->ServerFormatDataRequest=&RdpSession::clipboardServerDataRequest;
    clip->ServerFormatDataResponse=&RdpSession::clipboardServerDataResponse;
}
void RdpSession::channelDisconnected(void* context, const ChannelDisconnectedEventArgs* event)
{
    if (!context || !event || !event->name) return;
    auto* rdp = static_cast<rdpContext*>(context);
    auto* self = session(rdp->instance);
    if (!self) return;
    if(std::strcmp(event->name,CLIPRDR_CHANNEL_NAME)==0){QMutexLocker lock(&self->clipboardMutex_);self->clipboardContext_=nullptr;return;}
    if (std::strcmp(event->name, DISP_DVC_CHANNEL_NAME) != 0 && std::strcmp(event->name, DISP_CHANNEL_NAME) != 0) return;
    QMutexLocker lock(&self->displayMutex_);
    self->displayControl_ = nullptr; self->displayControlActive_ = false;
}
UINT RdpSession::displayControlCaps(DispClientContext* context, UINT32, UINT32, UINT32)
{
    if (!context || !context->custom) return CHANNEL_RC_INVALID_INSTANCE;
    auto* self = static_cast<RdpSession*>(context->custom);
    QMutexLocker lock(&self->displayMutex_);
    self->displayControlActive_ = true;
    return CHANNEL_RC_OK;
}
UINT RdpSession::clipboardMonitorReady(CliprdrClientContext* context,const CLIPRDR_MONITOR_READY*)
{
    if(!context||!context->custom||!context->ClientCapabilities||!context->ClientFormatList)
        return CHANNEL_RC_INVALID_INSTANCE;
    auto* self=static_cast<RdpSession*>(context->custom);
    {
        QMutexLocker lock(&self->clipboardMutex_);
        if(self->clipboardHandshakeStarted_)return CHANNEL_RC_OK;
        self->clipboardHandshakeStarted_=true;
    }

    // Capabilities must precede the first format list. Windows may ignore a
    // client's advertised formats when this part of the CLIPRDR handshake is
    // omitted.
    CLIPRDR_GENERAL_CAPABILITY_SET general{};
    general.capabilitySetType=CB_CAPSTYPE_GENERAL;
    general.capabilitySetLength=CB_CAPSTYPE_GENERAL_LEN;
    general.version=CB_CAPS_VERSION_2;
    general.generalFlags=CB_USE_LONG_FORMAT_NAMES;
    CLIPRDR_CAPABILITIES capabilities{};
    capabilities.cCapabilitiesSets=1;
    capabilities.capabilitySets=reinterpret_cast<CLIPRDR_CAPABILITY_SET*>(&general);
    const UINT capabilityResult=context->ClientCapabilities(context,&capabilities);
    if(capabilityResult!=CHANNEL_RC_OK)return capabilityResult;

    CLIPRDR_FORMAT format{CF_UNICODETEXT,nullptr};CLIPRDR_FORMAT_LIST list{};list.common.msgType=CB_FORMAT_LIST;list.numFormats=1;list.formats=&format;
    const UINT formatResult=context->ClientFormatList(context,&list);
    return formatResult;
}
UINT RdpSession::clipboardServerCapabilities(CliprdrClientContext* context,const CLIPRDR_CAPABILITIES* capabilities)
{
    if(!context||!context->custom||!capabilities)return CHANNEL_RC_INVALID_INSTANCE;
    auto* self=static_cast<RdpSession*>(context->custom);
    {
        QMutexLocker lock(&self->clipboardMutex_);
        self->clipboardServerReady_=true;
        // The channel can connect after the server's Monitor Ready PDU has
        // already passed. Start the client half of the handshake here, once
        // the server capabilities are known, instead of advertising formats
        // prematurely from PostConnect.
        self->clipboardHandshakeStarted_=false;
    }
    return clipboardMonitorReady(context,nullptr);
}
UINT RdpSession::clipboardServerFormatList(CliprdrClientContext* context,const CLIPRDR_FORMAT_LIST* list)
{
    if(!context||!context->custom||!list)return CHANNEL_RC_INVALID_INSTANCE;
    CLIPRDR_FORMAT_LIST_RESPONSE response{};response.common.msgType=CB_FORMAT_LIST_RESPONSE;response.common.msgFlags=CB_RESPONSE_OK;
    UINT result=context->ClientFormatListResponse?context->ClientFormatListResponse(context,&response):CHANNEL_RC_OK;
    bool unicode=false;for(UINT32 i=0;i<list->numFormats;++i)if(list->formats[i].formatId==CF_UNICODETEXT){unicode=true;break;}
    if(unicode&&context->ClientFormatDataRequest){CLIPRDR_FORMAT_DATA_REQUEST request{};request.common.msgType=CB_FORMAT_DATA_REQUEST;request.requestedFormatId=CF_UNICODETEXT;result=context->ClientFormatDataRequest(context,&request);}
    return result;
}
UINT RdpSession::clipboardServerFormatListResponse(CliprdrClientContext* context,const CLIPRDR_FORMAT_LIST_RESPONSE* response)
{
    return context&&context->custom&&response?CHANNEL_RC_OK:CHANNEL_RC_INVALID_INSTANCE;
}
UINT RdpSession::clipboardServerDataRequest(CliprdrClientContext* context,const CLIPRDR_FORMAT_DATA_REQUEST* request)
{
    if(!context||!context->custom||!request||!context->ClientFormatDataResponse)return CHANNEL_RC_INVALID_INSTANCE;
    auto* self=static_cast<RdpSession*>(context->custom);QString localText;
    {QMutexLocker lock(&self->clipboardMutex_);localText=self->localClipboardText_;}
    CLIPRDR_FORMAT_DATA_RESPONSE response{};response.common.msgType=CB_FORMAT_DATA_RESPONSE;
    QByteArray data;if(request->requestedFormatId==CF_UNICODETEXT){data=encodeClipboardText(localText);response.common.msgFlags=CB_RESPONSE_OK;response.common.dataLen=static_cast<UINT32>(data.size());response.requestedFormatData=reinterpret_cast<const BYTE*>(data.constData());}else response.common.msgFlags=CB_RESPONSE_FAIL;
    return context->ClientFormatDataResponse(context,&response);
}
UINT RdpSession::clipboardServerDataResponse(CliprdrClientContext* context,const CLIPRDR_FORMAT_DATA_RESPONSE* response)
{
    if(!context||!context->custom||!response||!(response->common.msgFlags&CB_RESPONSE_OK)||
        (response->common.dataLen>0&&!response->requestedFormatData))return CHANNEL_RC_OK;
    const QByteArray data(reinterpret_cast<const char*>(response->requestedFormatData),static_cast<qsizetype>(response->common.dataLen));
    const auto text=decodeClipboardText(data);if(text)emit static_cast<RdpSession*>(context->custom)->remoteClipboardText(*text);return CHANNEL_RC_OK;
}
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

BOOL RdpSession::desktopResize(rdpContext* context)
{
    if (!context || !context->gdi || !context->settings) return FALSE;
    const UINT32 width = freerdp_settings_get_uint32(context->settings, FreeRDP_DesktopWidth);
    const UINT32 height = freerdp_settings_get_uint32(context->settings, FreeRDP_DesktopHeight);
    if (!gdi_resize(context->gdi, width, height)) return FALSE;
    auto* self = session(context->instance);
    if (self) emit self->frameUpdated(QImage(), QRect());
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
void RdpSession::requestDisplayResize(const QSize size)
{
    QMutexLocker lock(&displayMutex_);
    pendingDisplaySize_ = constrainedDisplaySize(size);
}
void RdpSession::setLocalClipboardText(QString text)
{
    QMutexLocker lock(&clipboardMutex_);localClipboardText_=std::move(text);clipboardDirty_=true;
}
void RdpSession::flushClipboard()
{
    CliprdrClientContext* context=nullptr;
    {QMutexLocker lock(&clipboardMutex_);if(!clipboardDirty_||!clipboardServerReady_||!clipboardContext_||!clipboardContext_->ClientFormatList)return;clipboardDirty_=false;context=clipboardContext_;}
    CLIPRDR_FORMAT format{CF_UNICODETEXT,nullptr};CLIPRDR_FORMAT_LIST list{};list.common.msgType=CB_FORMAT_LIST;list.numFormats=1;list.formats=&format;
    (void)context->ClientFormatList(context,&list);
}
void RdpSession::flushDisplayResize()
{
    QMutexLocker lock(&displayMutex_);
    if (!displayControlActive_ || !displayControl_ || !displayControl_->SendMonitorLayout || pendingDisplaySize_.isEmpty()) return;
    const QSize size = pendingDisplaySize_; pendingDisplaySize_ = {};
    DISPLAY_CONTROL_MONITOR_LAYOUT layout{};
    layout.Flags = DISPLAY_CONTROL_MONITOR_PRIMARY;
    layout.Width = static_cast<UINT32>(size.width()); layout.Height = static_cast<UINT32>(size.height());
    layout.PhysicalWidth = static_cast<UINT32>(qBound(10, qRound(size.width() / 96.0 * 25.4), 10000));
    layout.PhysicalHeight = static_cast<UINT32>(qBound(10, qRound(size.height() / 96.0 * 25.4), 10000));
    layout.Orientation = 0; layout.DesktopScaleFactor = 100; layout.DeviceScaleFactor = 100;
    (void)displayControl_->SendMonitorLayout(displayControl_, 1, &layout);
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
