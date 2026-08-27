#pragma once

#include "rdp/RdpError.h"
#include "rdp/RdpRenderer.h"
#include "rdp/RdpSettings.h"

#include <QImage>
#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QRect>
#include <QSet>
#include <QVector>
#include <QWaitCondition>
#include <atomic>
#include <memory>
#include <freerdp/freerdp.h>

namespace openrdp {

enum class SessionState { Disconnected, Connecting, Authenticating, CertificateVerification,
    Connected, Disconnecting, Failed };

struct CertificateInfo {
    QString server;
    quint16 port = 3389;
    QString commonName;
    QString subject;
    QString issuer;
    QString fingerprint;
    QString reason;
};

class RdpSession final : public QObject {
    Q_OBJECT
public:
    explicit RdpSession(QObject* parent = nullptr);
    ~RdpSession() override;
    QImage currentFrame() const;

public slots:
    void connectToServer(const ConnectionSettings& settings);
    void disconnect();
    void provideCredentials(const QString& username, const QString& password, bool accepted);
    void provideCertificateDecision(bool accepted);
    void sendMouse(quint16 flags, quint16 x, quint16 y);
    void sendKey(quint32 virtualKey, bool down, bool repeat);

signals:
    void stateChanged(SessionState state);
    void connected(QSize desktopSize);
    void disconnected();
    void authenticationRequired(QString server, QString username);
    void certificateVerificationRequired(CertificateInfo certificate);
    void frameUpdated(QImage frame, QRect damage);
    void connectionError(RdpError error);

private:
    static BOOL preConnect(freerdp* instance);
    static BOOL postConnect(freerdp* instance);
    static void postDisconnect(freerdp* instance);
    static BOOL authenticate(freerdp* instance, char** username, char** password, char** domain, rdp_auth_reason reason);
    static DWORD verifyCertificate(freerdp* instance, const char* host, UINT16 port,
        const char* commonName, const char* subject, const char* issuer, const char* fingerprint,
        DWORD flags);
    static BOOL beginPaint(rdpContext* context);
    static BOOL endPaint(rdpContext* context);
    bool awaitCredentials(char** username, char** password, char** domain);
    DWORD awaitCertificate(const CertificateInfo& info);
    void setState(SessionState state);
    void cleanup();
    void flushInput();

    ConnectionSettings settings_;
    freerdp* instance_ = nullptr;
    RdpRenderer renderer_;
    std::atomic_bool cancelRequested_ = false;
    SessionState state_ = SessionState::Disconnected;
    mutable QMutex responseMutex_;
    QWaitCondition responseReady_;
    bool responsePending_ = false;
    bool responseAccepted_ = false;
    QString responseUsername_;
    QString responsePassword_;
    struct InputEvent { bool key; quint32 first; quint16 x; quint16 y; bool down; bool repeat; };
    QMutex inputMutex_;
    QVector<InputEvent> inputEvents_;
    QMutex instanceMutex_;
};
} // namespace openrdp

Q_DECLARE_METATYPE(openrdp::ConnectionSettings)
Q_DECLARE_METATYPE(openrdp::SessionState)
Q_DECLARE_METATYPE(openrdp::CertificateInfo)
