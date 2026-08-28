#include "gui/MainWindow.h"
#include "gui/CertificateDialog.h"
#include "gui/CredentialDialog.h"
#include "gui/RdpDisplayWidget.h"
#include "rdp/RdpSettings.h"
#include <QCloseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>
namespace openrdp {
MainWindow::MainWindow(const QString& initialServer,const QString& initialUser,QWidget* parent):QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("OpenRDP")); resize(1100,700);
    pages_=new QStackedWidget(this); setCentralWidget(pages_);
    auto* page=new QWidget; auto* layout=new QVBoxLayout(page); auto* form=new QFormLayout;
    savedConnections_=new QComboBox; savedConnections_->addItem(QStringLiteral("Select a previous session…"));
    computer_=new QLineEdit(initialServer); username_=new QLineEdit(initialUser); connect_=new QPushButton(QStringLiteral("Connect"));
    webAccount_=new QCheckBox(QStringLiteral("Use a web account to sign in to the remote computer"));
    form->addRow(QStringLiteral("Previous session:"),savedConnections_); form->addRow(QStringLiteral("Computer:"),computer_); form->addRow(QStringLiteral("User name:"),username_);
    layout->addLayout(form); layout->addWidget(webAccount_); layout->addWidget(connect_,0,Qt::AlignRight); layout->addStretch(); pages_->addWidget(page);
    display_=new RdpDisplayWidget; pages_->addWidget(display_);
    auto* disconnectAction=menuBar()->addAction(QStringLiteral("Disconnect")); disconnectAction->setEnabled(false);
    session_=new RdpSession; session_->moveToThread(&worker_); worker_.start();
    connect(connect_,&QPushButton::clicked,this,&MainWindow::connectClicked);
    connect(savedConnections_,qOverload<int>(&QComboBox::activated),this,&MainWindow::selectSavedConnection);
    connect(disconnectAction,&QAction::triggered,this,[this]{emit stopSession();});
    connect(this,&MainWindow::startSession,session_,&RdpSession::connectToServer,Qt::QueuedConnection);
    connect(this,&MainWindow::stopSession,session_,&RdpSession::disconnect,Qt::DirectConnection);
    connect(this,&MainWindow::credentialsProvided,session_,&RdpSession::provideCredentials,Qt::DirectConnection);
    connect(this,&MainWindow::certificateDecision,session_,&RdpSession::provideCertificateDecision,Qt::DirectConnection);
    connect(this,&MainWindow::webAuthenticationProvided,session_,&RdpSession::provideWebAuthenticationResult,Qt::DirectConnection);
    connect(session_,&RdpSession::authenticationRequired,this,&MainWindow::showCredentials);
    connect(session_,&RdpSession::certificateVerificationRequired,this,&MainWindow::showCertificate);
    connect(session_,&RdpSession::webAuthenticationRequired,this,&MainWindow::showWebAuthentication);
    connect(session_,&RdpSession::connectionError,this,&MainWindow::showError);
    connect(session_,&RdpSession::connected,this,[this,disconnectAction](QSize){
        if (pendingHistoryEntry_) {history_.record(*pendingHistoryEntry_);reloadSavedConnections();}
        pendingHistoryEntry_.reset();pages_->setCurrentWidget(display_);disconnectAction->setEnabled(true);
    });
    connect(session_,&RdpSession::disconnected,this,[this,disconnectAction]{disconnectAction->setEnabled(false);sessionEnded();});
    connect(session_,&RdpSession::frameUpdated,this,[this](const QImage& frame,QRect){display_->setFrame(frame);});
    connect(display_,&RdpDisplayWidget::mouseInput,session_,&RdpSession::sendMouse,Qt::DirectConnection);
    connect(display_,&RdpDisplayWidget::keyInput,session_,&RdpSession::sendKey,Qt::DirectConnection);
    reloadSavedConnections();
}
MainWindow::~MainWindow(){emit stopSession();worker_.quit();worker_.wait(5000);delete session_;}
void MainWindow::connectClicked(){QString error;auto parsed=parseServer(computer_->text(),&error);if(!parsed){QMessageBox::warning(this,QStringLiteral("OpenRDP"),error);return;} ConnectionSettings s; s.hostname=parsed->hostname;s.port=parsed->port;parseUsername(username_->text(),s.username,s.domain);s.authenticationMode=webAccount_->isChecked()?AuthenticationMode::EntraWebAccount:AuthenticationMode::NlaPassword;pendingHistoryEntry_=ConnectionHistoryEntry{computer_->text().trimmed(),username_->text(),s.authenticationMode};connect_->setEnabled(false);emit startSession(s);}
void MainWindow::reloadSavedConnections(){
    savedEntries_=history_.load();savedConnections_->clear();savedConnections_->addItem(QStringLiteral("Select a previous session…"));
    for(const auto& entry:savedEntries_)savedConnections_->addItem(entry.username.isEmpty()?entry.server:QStringLiteral("%1 — %2").arg(entry.server,entry.username));
}
void MainWindow::selectSavedConnection(const int index){
    if(index<=0||index>savedEntries_.size())return;
    const auto& entry=savedEntries_.at(index-1);
    computer_->setText(entry.server);username_->setText(entry.username);webAccount_->setChecked(entry.authenticationMode==AuthenticationMode::EntraWebAccount);
}
void MainWindow::showCredentials(QString server,QString user){CredentialDialog d(server,user,this);const bool ok=d.exec()==QDialog::Accepted;emit credentialsProvided(d.username(),ok?d.password():QString(),ok);}
void MainWindow::showCertificate(CertificateInfo info){CertificateDialog d(info,this);emit certificateDecision(d.exec()==QDialog::Accepted);}
void MainWindow::showWebAuthentication(QString authorizationUrl){
    QDialog dialog(this); dialog.setWindowTitle(QStringLiteral("Web Account Sign-In"));
    dialog.resize(520,180);
    auto* layout=new QVBoxLayout(&dialog);
    auto* instructions=new QLabel(QStringLiteral("Complete Microsoft sign-in in the private Chromium window. Phone/passkey QR authentication is supported. OpenRDP will securely receive the one-time response; do not copy any browser address."));instructions->setWordWrap(true);layout->addWidget(instructions);
    auto* status=new QLabel(QStringLiteral("Starting private browser…"));layout->addWidget(status);
    auto* cancel=new QPushButton(QStringLiteral("Cancel"));layout->addWidget(cancel,0,Qt::AlignRight);
    const QUrl browserUrl=QUrl::fromEncoded(authorizationUrl.toUtf8(),QUrl::StrictMode);
    if (!browserUrl.isValid() || browserUrl.scheme()!=QStringLiteral("https")) {
        QMessageBox::critical(this,QStringLiteral("Web Account Sign-In"),QStringLiteral("FreeRDP produced an invalid web authentication address."));
        emit webAuthenticationProvided(QString(),false);
        return;
    }
    QString redirectUrl;
    const QString browser=QStandardPaths::findExecutable(QStringLiteral("chromium"));
    if (browser.isEmpty()) {
        QMessageBox::critical(this,QStringLiteral("Web Account Sign-In"),QStringLiteral("Chromium is required for phone/passkey sign-in but was not found."));
        emit webAuthenticationProvided(QString(),false);
        return;
    }
    QTemporaryDir profileDir(QDir::tempPath()+QStringLiteral("/openrdp-auth-XXXXXX"));
    if (!profileDir.isValid()) {
        QMessageBox::critical(this,QStringLiteral("Web Account Sign-In"),QStringLiteral("A private browser profile could not be created."));
        emit webAuthenticationProvided(QString(),false);
        return;
    }
    QTcpServer portReservation;
    if (!portReservation.listen(QHostAddress::LocalHost,0)) {
        QMessageBox::critical(this,QStringLiteral("Web Account Sign-In"),QStringLiteral("A private callback channel could not be created."));
        emit webAuthenticationProvided(QString(),false);
        return;
    }
    const quint16 debugPort=portReservation.serverPort();
    portReservation.close();
    QProcess browserProcess;
    browserProcess.setProgram(browser);
    browserProcess.setArguments({QStringLiteral("--user-data-dir=")+profileDir.path(),
        QStringLiteral("--remote-debugging-address=127.0.0.1"),
        QStringLiteral("--remote-debugging-port=")+QString::number(debugPort),
        QStringLiteral("--no-first-run"),QStringLiteral("--no-default-browser-check"),
        QStringLiteral("--incognito"),QStringLiteral("--new-window"),browserUrl.toString(QUrl::FullyEncoded)});
    browserProcess.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    QNetworkAccessManager network;
    QTimer pollTimer; pollTimer.setInterval(200);
    connect(&pollTimer,&QTimer::timeout,&dialog,[&network,debugPort]{
        network.get(QNetworkRequest(QUrl(QStringLiteral("http://127.0.0.1:%1/json/list").arg(debugPort))));
    });
    connect(&network,&QNetworkAccessManager::finished,&dialog,[&dialog,&redirectUrl,status](QNetworkReply* reply){
        const QByteArray body=reply->readAll();
        reply->deleteLater();
        const QJsonDocument document=QJsonDocument::fromJson(body);
        if (!document.isArray()) return;
        status->setText(QStringLiteral("Waiting for Microsoft sign-in to complete…"));
        for (const QJsonValue& value : document.array()) {
            const QString candidate=value.toObject().value(QStringLiteral("url")).toString();
            if (!authorizationCodeFromRedirect(candidate)) continue;
            redirectUrl=candidate;
            dialog.accept();
            return;
        }
    });
    connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);
    connect(&browserProcess,&QProcess::errorOccurred,&dialog,[&dialog,status](QProcess::ProcessError error){
        if (error==QProcess::FailedToStart) {status->setText(QStringLiteral("Chromium could not be started."));dialog.reject();}
    });
    browserProcess.start();
    pollTimer.start();
    const bool accepted=dialog.exec()==QDialog::Accepted && !redirectUrl.isEmpty();
    pollTimer.stop();
    browserProcess.terminate();
    if (!browserProcess.waitForFinished(2000)) {browserProcess.kill();browserProcess.waitForFinished(2000);}
    emit webAuthenticationProvided(accepted?redirectUrl:QString(),accepted);
}
void MainWindow::showError(RdpError e){QMessageBox box(QMessageBox::Critical,QStringLiteral("Remote Desktop Connection Failed"),e.userMessage,QMessageBox::Ok,this);box.setDetailedText(e.technicalDetails);box.exec();}
void MainWindow::sessionEnded(){pendingHistoryEntry_.reset();pages_->setCurrentIndex(0);display_->clear();connect_->setEnabled(true);}
void MainWindow::closeEvent(QCloseEvent* e){emit stopSession();e->accept();}
}
