#include "gui/MainWindow.h"
#include "gui/CertificateDialog.h"
#include "gui/CredentialDialog.h"
#include "gui/RdpDisplayWidget.h"
#include "rdp/RdpSettings.h"
#include <QCloseEvent>
#include <QFormLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
namespace openrdp {
MainWindow::MainWindow(const QString& initialServer,const QString& initialUser,QWidget* parent):QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("OpenRDP")); resize(1100,700);
    pages_=new QStackedWidget(this); setCentralWidget(pages_);
    auto* page=new QWidget; auto* layout=new QVBoxLayout(page); auto* form=new QFormLayout;
    computer_=new QLineEdit(initialServer); username_=new QLineEdit(initialUser); connect_=new QPushButton(QStringLiteral("Connect"));
    form->addRow(QStringLiteral("Computer:"),computer_); form->addRow(QStringLiteral("User name:"),username_);
    layout->addLayout(form); layout->addWidget(connect_,0,Qt::AlignRight); layout->addStretch(); pages_->addWidget(page);
    display_=new RdpDisplayWidget; pages_->addWidget(display_);
    auto* disconnectAction=menuBar()->addAction(QStringLiteral("Disconnect")); disconnectAction->setEnabled(false);
    session_=new RdpSession; session_->moveToThread(&worker_); worker_.start();
    connect(connect_,&QPushButton::clicked,this,&MainWindow::connectClicked);
    connect(disconnectAction,&QAction::triggered,this,[this]{emit stopSession();});
    connect(this,&MainWindow::startSession,session_,&RdpSession::connectToServer,Qt::QueuedConnection);
    connect(this,&MainWindow::stopSession,session_,&RdpSession::disconnect,Qt::DirectConnection);
    connect(this,&MainWindow::credentialsProvided,session_,&RdpSession::provideCredentials,Qt::DirectConnection);
    connect(this,&MainWindow::certificateDecision,session_,&RdpSession::provideCertificateDecision,Qt::DirectConnection);
    connect(session_,&RdpSession::authenticationRequired,this,&MainWindow::showCredentials);
    connect(session_,&RdpSession::certificateVerificationRequired,this,&MainWindow::showCertificate);
    connect(session_,&RdpSession::connectionError,this,&MainWindow::showError);
    connect(session_,&RdpSession::connected,this,[this,disconnectAction](QSize){pages_->setCurrentWidget(display_);disconnectAction->setEnabled(true);});
    connect(session_,&RdpSession::disconnected,this,[this,disconnectAction]{disconnectAction->setEnabled(false);sessionEnded();});
    connect(session_,&RdpSession::frameUpdated,this,[this](const QImage& frame,QRect){display_->setFrame(frame);});
    connect(display_,&RdpDisplayWidget::mouseInput,session_,&RdpSession::sendMouse,Qt::DirectConnection);
    connect(display_,&RdpDisplayWidget::keyInput,session_,&RdpSession::sendKey,Qt::DirectConnection);
}
MainWindow::~MainWindow(){emit stopSession();worker_.quit();worker_.wait(5000);delete session_;}
void MainWindow::connectClicked(){QString error;auto parsed=parseServer(computer_->text(),&error);if(!parsed){QMessageBox::warning(this,QStringLiteral("OpenRDP"),error);return;} ConnectionSettings s; s.hostname=parsed->hostname;s.port=parsed->port;parseUsername(username_->text(),s.username,s.domain);connect_->setEnabled(false);emit startSession(s);}
void MainWindow::showCredentials(QString server,QString user){CredentialDialog d(server,user,this);const bool ok=d.exec()==QDialog::Accepted;emit credentialsProvided(d.username(),ok?d.password():QString(),ok);}
void MainWindow::showCertificate(CertificateInfo info){CertificateDialog d(info,this);emit certificateDecision(d.exec()==QDialog::Accepted);}
void MainWindow::showError(RdpError e){QMessageBox box(QMessageBox::Critical,QStringLiteral("Remote Desktop Connection Failed"),e.userMessage,QMessageBox::Ok,this);box.setDetailedText(e.technicalDetails);box.exec();}
void MainWindow::sessionEnded(){pages_->setCurrentIndex(0);display_->clear();connect_->setEnabled(true);}
void MainWindow::closeEvent(QCloseEvent* e){emit stopSession();e->accept();}
}
