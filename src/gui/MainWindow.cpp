#include "gui/MainWindow.h"
#include "gui/CertificateDialog.h"
#include "gui/CredentialDialog.h"
#include "gui/RdpDisplayWidget.h"
#include "channels/ClipboardManager.h"
#include "profiles/RdpProfileMapper.h"
#include "rdp/RdpSettings.h"
#include <QAction>
#include <QCloseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QScreen>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStackedWidget>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QtConcurrentRun>
namespace openrdp {
MainWindow::MainWindow(const QString& initialServer,const QString& initialUser,
    const QString& initialRdpFile,QWidget* parent):QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("OpenRDP")); resize(1100,700);
    pages_=new QStackedWidget(this); setCentralWidget(pages_);
    auto* page=new QWidget; auto* layout=new QVBoxLayout(page); auto* form=new QFormLayout;
    profileSelector_=new QComboBox;profileSelector_->setAccessibleName(QStringLiteral("Connection profile"));profileSelector_->addItem(QStringLiteral("New connection"));
    profileName_=new QLineEdit;profileName_->setPlaceholderText(QStringLiteral("Connection name"));
    savedConnections_=new QComboBox; savedConnections_->addItem(QStringLiteral("Select a previous session…"));
    computer_=new QLineEdit(initialServer); username_=new QLineEdit(initialUser); connect_=new QPushButton(QStringLiteral("Connect"));
    webAccount_=new QCheckBox(QStringLiteral("Use a web account to sign in to the remote computer"));
    multiMonitor_=new QCheckBox(QStringLiteral("Use all monitors"));
    shareClipboard_=new QCheckBox(QStringLiteral("Share clipboard"));shareClipboard_->setChecked(true);
    audioMode_=new QComboBox;audioMode_->addItem(QStringLiteral("Play remote audio on this computer"),0);audioMode_->addItem(QStringLiteral("Leave audio at remote computer"),1);audioMode_->addItem(QStringLiteral("Do not play remote audio"),2);
    outputDevice_=new QComboBox;outputDevice_->setEditable(true);outputDevice_->addItem(QStringLiteral("System Default"));outputDevice_->setAccessibleName(QStringLiteral("Audio output device"));
    inputDevice_=new QComboBox;inputDevice_->setEditable(true);inputDevice_->addItem(QStringLiteral("System Default"));inputDevice_->setAccessibleName(QStringLiteral("Microphone device"));
    shareMicrophone_=new QCheckBox(QStringLiteral("Share microphone"));shareMicrophone_->setChecked(false);
    inputDevice_->setEnabled(false);
    connect(shareMicrophone_,&QCheckBox::toggled,inputDevice_,&QWidget::setEnabled);
    connect(audioMode_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){outputDevice_->setEnabled(index==0);});
    form->addRow(QStringLiteral("Profile:"),profileSelector_);form->addRow(QStringLiteral("Name:"),profileName_);form->addRow(QStringLiteral("Previous session:"),savedConnections_); form->addRow(QStringLiteral("Computer:"),computer_); form->addRow(QStringLiteral("User name:"),username_);
    form->addRow(QStringLiteral("Remote audio:"),audioMode_);
    form->addRow(QStringLiteral("Output device:"),outputDevice_);form->addRow(QStringLiteral("Microphone device:"),inputDevice_);
    layout->addLayout(form); layout->addWidget(webAccount_); layout->addWidget(multiMonitor_); layout->addWidget(shareClipboard_); layout->addWidget(shareMicrophone_);
    layout->addWidget(new QLabel(QStringLiteral("Local folders shared with the remote computer"),page));folderList_=new QListWidget(page);folderList_->setAccessibleName(QStringLiteral("Redirected local folders"));layout->addWidget(folderList_);
    auto* folderButtons=new QHBoxLayout;auto* addFolder=new QPushButton(QStringLiteral("Add Folder…"),page);auto* removeFolder=new QPushButton(QStringLiteral("Remove"),page);folderButtons->addWidget(addFolder);folderButtons->addWidget(removeFolder);folderButtons->addStretch();layout->addLayout(folderButtons);
    connect(addFolder,&QPushButton::clicked,this,&MainWindow::addRedirectedFolder);connect(removeFolder,&QPushButton::clicked,this,&MainWindow::removeRedirectedFolder);
    auto* profileButtons=new QHBoxLayout;auto* newProfileButton=new QPushButton(QStringLiteral("New"),page);auto* saveProfileButton=new QPushButton(QStringLiteral("Save Profile"),page);auto* deleteProfileButton=new QPushButton(QStringLiteral("Delete Profile"),page);profileButtons->addWidget(newProfileButton);profileButtons->addWidget(saveProfileButton);profileButtons->addWidget(deleteProfileButton);profileButtons->addStretch();layout->addLayout(profileButtons);
    connect(newProfileButton,&QPushButton::clicked,this,&MainWindow::newProfile);connect(saveProfileButton,&QPushButton::clicked,this,&MainWindow::saveProfile);connect(deleteProfileButton,&QPushButton::clicked,this,&MainWindow::deleteProfile);connect(profileSelector_,qOverload<int>(&QComboBox::activated),this,&MainWindow::selectProfile);
    sharePrinters_=new QCheckBox(QStringLiteral("Share selected printers"),page);sharePrinters_->setChecked(false);layout->addWidget(sharePrinters_);
    printerList_=new QListWidget(page);printerList_->setAccessibleName(QStringLiteral("Installed printers"));printerList_->setEnabled(false);layout->addWidget(printerList_);
    connect(sharePrinters_,&QCheckBox::toggled,printerList_,&QWidget::setEnabled);
    auto* printerWatcher=new QFutureWatcher<QVector<PrinterInfo>>(this);
    connect(printerWatcher,&QFutureWatcher<QVector<PrinterInfo>>::finished,this,[this,printerWatcher]{availablePrinters_=printerWatcher->result();printerList_->clear();for(const auto& printer:availablePrinters_){auto* item=new QListWidgetItem(printer.displayName,printerList_);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(pendingPrinterNames_.contains(printer.name)?Qt::Checked:Qt::Unchecked);item->setToolTip(printer.isDefault?QStringLiteral("Default CUPS printer"):printer.name);}printerWatcher->deleteLater();});
    printerWatcher->setFuture(QtConcurrent::run(&discoverCupsPrinters));
    layout->addWidget(connect_,0,Qt::AlignRight); layout->addStretch(); pages_->addWidget(page);
    display_=new RdpDisplayWidget; pages_->addWidget(display_);
    display_->installEventFilter(this);
    resizeDebounce_=new QTimer(this);resizeDebounce_->setSingleShot(true);resizeDebounce_->setInterval(220);
    auto* fileMenu=menuBar()->addMenu(QStringLiteral("&File"));
    auto* openAction=fileMenu->addAction(QStringLiteral("&Open Connection File…"));
    openAction->setShortcut(QKeySequence::Open);
    auto* saveAsAction=fileMenu->addAction(QStringLiteral("Save Connection &As…"));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(openAction,&QAction::triggered,this,&MainWindow::openRdpFile);
    connect(saveAsAction,&QAction::triggered,this,&MainWindow::saveRdpFileAs);
    fileMenu->addSeparator();
    auto* removeRecentAction=fileMenu->addAction(QStringLiteral("Remove Selected Recent Connection"));
    auto* clearRecentAction=fileMenu->addAction(QStringLiteral("Clear Recent Connections"));
    connect(removeRecentAction,&QAction::triggered,this,&MainWindow::removeRecentConnection);
    connect(clearRecentAction,&QAction::triggered,this,&MainWindow::clearRecentConnections);
    auto* disconnectAction=menuBar()->addAction(QStringLiteral("Disconnect")); disconnectAction->setEnabled(false);
    auto* viewMenu=menuBar()->addMenu(QStringLiteral("&View"));
    fullScreenAction_=viewMenu->addAction(QStringLiteral("Full Screen"));
    fullScreenAction_->setCheckable(true);
    fullScreenAction_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+Return")));
    connect(fullScreenAction_,&QAction::triggered,this,&MainWindow::toggleFullScreen);
    auto* informationAction=viewMenu->addAction(QStringLiteral("Connection Information"));
    connect(informationAction,&QAction::triggered,this,&MainWindow::showConnectionInformation);
    sessionToolbar_=addToolBar(QStringLiteral("Session"));
    sessionToolbar_->setMovable(false);
    sessionToolbar_->hide();
    sessionServerLabel_=new QLabel(QStringLiteral("Not connected"),sessionToolbar_);
    sessionServerLabel_->setAccessibleName(QStringLiteral("Connected server"));
    sessionToolbar_->addWidget(sessionServerLabel_);
    sessionToolbar_->addSeparator();
    pinToolbarAction_=sessionToolbar_->addAction(QIcon::fromTheme(QStringLiteral("window-pin")),QStringLiteral("Pin connection bar"));
    pinToolbarAction_->setCheckable(true);pinToolbarAction_->setChecked(true);
    auto* minimizeAction=sessionToolbar_->addAction(QIcon::fromTheme(QStringLiteral("window-minimize")),QStringLiteral("Minimize"));
    auto* toolbarFullScreen=sessionToolbar_->addAction(QIcon::fromTheme(QStringLiteral("view-fullscreen")),QStringLiteral("Exit Full Screen"));
    auto* toolbarDisconnect=sessionToolbar_->addAction(QIcon::fromTheme(QStringLiteral("network-disconnect")),QStringLiteral("Disconnect"));
    connect(minimizeAction,&QAction::triggered,this,&QWidget::showMinimized);
    connect(toolbarFullScreen,&QAction::triggered,this,&MainWindow::toggleFullScreen);
    toolbarHideTimer_=new QTimer(this);toolbarHideTimer_->setSingleShot(true);toolbarHideTimer_->setInterval(1800);
    connect(toolbarHideTimer_,&QTimer::timeout,this,[this]{if(isFullScreen()&&!pinToolbarAction_->isChecked())sessionToolbar_->hide();});
    connect(pinToolbarAction_,&QAction::toggled,this,[this](bool pinned){if(pinned){toolbarHideTimer_->stop();sessionToolbar_->show();}else if(isFullScreen())toolbarHideTimer_->start();});
    session_=new RdpSession; session_->moveToThread(&worker_); worker_.start();
    clipboardManager_=new ClipboardManager(this);
    microphoneIndicator_=new QLabel(QStringLiteral("Microphone shared"),this);microphoneIndicator_->setAccessibleName(QStringLiteral("Microphone is shared with the remote computer"));microphoneIndicator_->hide();statusBar()->addPermanentWidget(microphoneIndicator_);
    connect(connect_,&QPushButton::clicked,this,&MainWindow::connectClicked);
    connect(savedConnections_,qOverload<int>(&QComboBox::activated),this,&MainWindow::selectSavedConnection);
    connect(disconnectAction,&QAction::triggered,this,[this]{emit stopSession();});
    connect(toolbarDisconnect,&QAction::triggered,this,[this]{emit stopSession();});
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
        pendingHistoryEntry_.reset();pages_->setCurrentWidget(display_);disconnectAction->setEnabled(true);clipboardManager_->setEnabled(clipboardEnabled_);microphoneIndicator_->setVisible(shareMicrophone_->isChecked());connectedAt_=QDateTime::currentDateTimeUtc();sessionServerLabel_->setText(QStringLiteral("Secure connection to %1").arg(computer_->text().trimmed()));sessionToolbar_->show();
    });
    connect(session_,&RdpSession::disconnected,this,[this,disconnectAction]{microphoneIndicator_->hide();clipboardManager_->setEnabled(false);disconnectAction->setEnabled(false);sessionToolbar_->hide();connectedAt_={};sessionEnded();});
    connect(session_,&RdpSession::frameUpdated,this,[this](const QImage& frame,QRect){display_->setFrame(frame);});
    connect(display_,&RdpDisplayWidget::mouseInput,session_,&RdpSession::sendMouse,Qt::DirectConnection);
    connect(display_,&RdpDisplayWidget::keyInput,session_,&RdpSession::sendKey,Qt::DirectConnection);
    connect(display_,&RdpDisplayWidget::viewportResized,this,[this](QSize size){pendingDisplaySize_=size;resizeDebounce_->start();});
    connect(resizeDebounce_,&QTimer::timeout,this,[this]{emit remoteResizeRequested(pendingDisplaySize_);});
    connect(this,&MainWindow::remoteResizeRequested,session_,&RdpSession::requestDisplayResize,Qt::DirectConnection);
    connect(clipboardManager_,&ClipboardManager::localTextChanged,session_,&RdpSession::setLocalClipboardText,Qt::DirectConnection);
    connect(session_,&RdpSession::remoteClipboardText,clipboardManager_,&ClipboardManager::applyRemoteText);
    reloadSavedConnections();
    reloadProfiles();
    if(!initialRdpFile.isEmpty())QTimer::singleShot(0,this,[this,initialRdpFile]{loadRdpFile(initialRdpFile);});
}
MainWindow::~MainWindow(){emit stopSession();worker_.quit();worker_.wait(5000);delete session_;}
void MainWindow::connectClicked(){QString error;auto parsed=parseServer(computer_->text(),&error);if(!parsed){QMessageBox::warning(this,QStringLiteral("OpenRDP"),error);return;} ConnectionSettings s; s.hostname=parsed->hostname;s.port=parsed->port;parseUsername(username_->text(),s.username,s.domain);s.authenticationMode=webAccount_->isChecked()?AuthenticationMode::EntraWebAccount:AuthenticationMode::NlaPassword;
    if(multiMonitor_->isChecked()){
        for(QScreen* screen:QGuiApplication::screens()){
            const qreal scale=screen->devicePixelRatio();const QRect logical=screen->geometry();
            const QRect pixels(qRound(logical.x()*scale),qRound(logical.y()*scale),qRound(logical.width()*scale),qRound(logical.height()*scale));
            int orientation=0;switch(screen->orientation()){case Qt::PortraitOrientation:orientation=90;break;case Qt::InvertedLandscapeOrientation:orientation=180;break;case Qt::InvertedPortraitOrientation:orientation=270;break;default:break;}
            s.monitors.append({screen->name(),pixels,screen->physicalSize().toSize(),scale,orientation,screen==QGuiApplication::primaryScreen()});
        }
        if(!validMonitorTopology(s.monitors,&error)){QMessageBox::warning(this,QStringLiteral("Monitor Configuration"),error);return;}
        s.dynamicResolution=false;
    }
    clipboardEnabled_=shareClipboard_->isChecked();s.clipboard=clipboardEnabled_;s.audio.output=audioMode_->currentIndex()==1?RemoteAudioMode::Remote:audioMode_->currentIndex()==2?RemoteAudioMode::Disabled:RemoteAudioMode::Local;s.audio.microphone=shareMicrophone_->isChecked();s.audio.outputDevice=outputDevice_->currentText()==QStringLiteral("System Default")?QString():outputDevice_->currentText();s.audio.inputDevice=inputDevice_->currentText()==QStringLiteral("System Default")?QString():inputDevice_->currentText();
    for(const auto& folder:redirectedFolders_){const auto checked=validateRedirectedFolder(folder.name,folder.canonicalPath,&error);if(!checked){QMessageBox::warning(this,QStringLiteral("Local Folder Unavailable"),error);return;}s.folders.append(*checked);}
    if(sharePrinters_->isChecked()){for(int i=0;i<printerList_->count()&&i<availablePrinters_.size();++i)if(printerList_->item(i)->checkState()==Qt::Checked)s.printers.append(availablePrinters_.at(i));}
    pendingHistoryEntry_=ConnectionHistoryEntry{computer_->text().trimmed(),username_->text(),s.authenticationMode,{},currentProfileId_};connect_->setEnabled(false);emit startSession(s);}
void MainWindow::reloadSavedConnections(){
    savedEntries_=history_.load();savedConnections_->clear();savedConnections_->addItem(QStringLiteral("Select a previous session…"));
    for(const auto& entry:savedEntries_)savedConnections_->addItem(entry.username.isEmpty()?entry.server:QStringLiteral("%1 — %2").arg(entry.server,entry.username));
}
void MainWindow::selectSavedConnection(const int index){
    if(index<=0||index>savedEntries_.size())return;
    const auto& entry=savedEntries_.at(index-1);
    if(!entry.profileId.isEmpty()){
        const auto profile=std::find_if(profiles_.cbegin(),profiles_.cend(),[&entry](const ConnectionProfile& candidate){return candidate.id==entry.profileId;});
        if(profile!=profiles_.cend()){
            applyProfile(*profile);
            for(int i=0;i<profiles_.size();++i)if(profiles_.at(i).id==profile->id){profileSelector_->setCurrentIndex(i+1);break;}
            return;
        }
    }
    computer_->setText(entry.server);username_->setText(entry.username);webAccount_->setChecked(entry.authenticationMode==AuthenticationMode::EntraWebAccount);
}
void MainWindow::removeRecentConnection(){
    const int index=savedConnections_->currentIndex();
    if(index<=0||index>savedEntries_.size())return;
    if(history_.remove(savedEntries_.at(index-1)))reloadSavedConnections();
}
void MainWindow::clearRecentConnections(){
    if(savedEntries_.isEmpty())return;
    if(QMessageBox::question(this,QStringLiteral("Clear Recent Connections"),QStringLiteral("Remove all recent connection entries?"),QMessageBox::Cancel|QMessageBox::Yes,QMessageBox::Cancel)==QMessageBox::Yes&&history_.clear())reloadSavedConnections();
}
ConnectionProfile MainWindow::profileFromEditor() const{
    ConnectionProfile profile;profile.id=currentProfileId_;profile.name=profileName_->text().trimmed();profile.server=computer_->text().trimmed();profile.username=username_->text();profile.authenticationMode=webAccount_->isChecked()?AuthenticationMode::EntraWebAccount:AuthenticationMode::NlaPassword;profile.display.mode=multiMonitor_->isChecked()?DisplayMode::MultipleMonitors:DisplayMode::SingleMonitor;
    profile.resources.clipboard=shareClipboard_->isChecked()?ClipboardSharing::Bidirectional:ClipboardSharing::Disabled;profile.resources.audioPlayback=audioMode_->currentIndex()==1?AudioPlayback::Remote:audioMode_->currentIndex()==2?AudioPlayback::Disabled:AudioPlayback::Local;profile.resources.audioOutputDevice=outputDevice_->currentText()==QStringLiteral("System Default")?QString():outputDevice_->currentText();profile.resources.audioInputDevice=inputDevice_->currentText()==QStringLiteral("System Default")?QString():inputDevice_->currentText();profile.resources.microphone=shareMicrophone_->isChecked();
    for(const auto& folder:redirectedFolders_)
        profile.resources.folders.append({true,folder.name,folder.canonicalPath});
    profile.resources.printers=sharePrinters_->isChecked();
    if(profile.resources.printers)
        for(int i=0;i<printerList_->count()&&i<availablePrinters_.size();++i)
            if(printerList_->item(i)->checkState()==Qt::Checked)
                profile.resources.printerNames.append(availablePrinters_.at(i).name);
    for(const auto& existing:profiles_){
        if(existing.id==currentProfileId_){
            profile.favorite=existing.favorite;
            profile.lastConnectedAt=existing.lastConnectedAt;
            break;
        }
    }
    return profile;
}
void MainWindow::applyProfile(const ConnectionProfile& profile){
    currentProfileId_=profile.id;profileName_->setText(profile.name);computer_->setText(profile.server);username_->setText(profile.username);webAccount_->setChecked(profile.authenticationMode==AuthenticationMode::EntraWebAccount);multiMonitor_->setChecked(profile.display.mode==DisplayMode::MultipleMonitors);shareClipboard_->setChecked(profile.resources.clipboard==ClipboardSharing::Bidirectional);audioMode_->setCurrentIndex(profile.resources.audioPlayback==AudioPlayback::Remote?1:profile.resources.audioPlayback==AudioPlayback::Disabled?2:0);outputDevice_->setEditText(profile.resources.audioOutputDevice.isEmpty()?QStringLiteral("System Default"):profile.resources.audioOutputDevice);inputDevice_->setEditText(profile.resources.audioInputDevice.isEmpty()?QStringLiteral("System Default"):profile.resources.audioInputDevice);shareMicrophone_->setChecked(profile.resources.microphone);
    redirectedFolders_.clear();folderList_->clear();for(const auto& folder:profile.resources.folders)if(folder.enabled){redirectedFolders_.append({folder.remoteName,folder.localPath});folderList_->addItem(QStringLiteral("%1 — %2").arg(folder.remoteName,folder.localPath));}
    sharePrinters_->setChecked(profile.resources.printers);pendingPrinterNames_=profile.resources.printerNames;for(int i=0;i<printerList_->count()&&i<availablePrinters_.size();++i)printerList_->item(i)->setCheckState(pendingPrinterNames_.contains(availablePrinters_.at(i).name)?Qt::Checked:Qt::Unchecked);
}
void MainWindow::reloadProfiles(){profiles_.clear();profileSelector_->clear();profileSelector_->addItem(QStringLiteral("New connection"));for(const QString& id:profileStore_.profileIds()){QString error;const auto profile=profileStore_.load(id,&error);if(!profile)continue;profiles_.append(*profile);profileSelector_->addItem(profile->name.isEmpty()?profile->server:profile->name,id);}if(!currentProfileId_.isEmpty()){for(int i=0;i<profiles_.size();++i)if(profiles_.at(i).id==currentProfileId_){profileSelector_->setCurrentIndex(i+1);break;}}}
void MainWindow::selectProfile(const int index){if(index<=0||index>profiles_.size()){newProfile();return;}applyProfile(profiles_.at(index-1));}
void MainWindow::newProfile(){currentProfileId_.clear();profileSelector_->setCurrentIndex(0);profileName_->clear();computer_->clear();username_->clear();webAccount_->setChecked(false);multiMonitor_->setChecked(false);shareClipboard_->setChecked(true);audioMode_->setCurrentIndex(0);outputDevice_->setEditText(QStringLiteral("System Default"));inputDevice_->setEditText(QStringLiteral("System Default"));shareMicrophone_->setChecked(false);redirectedFolders_.clear();folderList_->clear();sharePrinters_->setChecked(false);pendingPrinterNames_.clear();for(int i=0;i<printerList_->count();++i)printerList_->item(i)->setCheckState(Qt::Unchecked);computer_->setFocus();}
void MainWindow::saveProfile(){ConnectionProfile profile=profileFromEditor();if(profile.name.isEmpty()){QMessageBox::warning(this,QStringLiteral("Save Profile"),QStringLiteral("Enter a profile name."));profileName_->setFocus();return;}QString error;if(!profileStore_.save(profile,&error)){QMessageBox::critical(this,QStringLiteral("Unable to Save Profile"),error);return;}currentProfileId_=profile.id;reloadProfiles();statusBar()->showMessage(QStringLiteral("Profile saved."),3000);}
void MainWindow::deleteProfile(){if(currentProfileId_.isEmpty())return;if(QMessageBox::question(this,QStringLiteral("Delete Profile"),QStringLiteral("Delete this connection profile?"),QMessageBox::Cancel|QMessageBox::Yes,QMessageBox::Cancel)!=QMessageBox::Yes)return;QString error;if(!profileStore_.remove(currentProfileId_,&error)){QMessageBox::critical(this,QStringLiteral("Unable to Delete Profile"),error);return;}newProfile();reloadProfiles();}
void MainWindow::openRdpFile(){
    const QString path=QFileDialog::getOpenFileName(this,QStringLiteral("Open Connection File"),{},
        QStringLiteral("Remote Desktop files (*.rdp);;All files (*)"));
    if(!path.isEmpty())loadRdpFile(path);
}
void MainWindow::saveRdpFileAs(){
    QString path=QFileDialog::getSaveFileName(this,QStringLiteral("Save Connection As"),{},
        QStringLiteral("Remote Desktop files (*.rdp)"));
    if(path.isEmpty())return;
    if(!path.endsWith(QStringLiteral(".rdp"),Qt::CaseInsensitive))path+=QStringLiteral(".rdp");
    writeRdpFile(path);
}
bool MainWindow::loadRdpFile(const QString& path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this,QStringLiteral("Unable to Open Connection File"),file.errorString());return false;
    }
    const auto parsed=RdpFileParser().parse(file.readAll());
    if(!parsed){
        const QString location=parsed.errorLine>0?QStringLiteral(" on line %1").arg(parsed.errorLine):QString();
        QMessageBox::critical(this,QStringLiteral("Unable to Open Connection File"),parsed.error+location);return false;
    }
    const auto imported=RdpProfileMapper().importFile(parsed.file);
    QStringList requested;
    for(const auto& item:imported.resourceRequests){
        switch(item.resource){
        case SensitiveResource::Clipboard: requested.append(QStringLiteral("Clipboard"));break;
        case SensitiveResource::Microphone: requested.append(QStringLiteral("Microphone"));break;
        case SensitiveResource::LocalDrives: requested.append(QStringLiteral("Local drives or folders"));break;
        case SensitiveResource::Printers: requested.append(QStringLiteral("Printers"));break;
        }
    }
    if(parsed.file.lastProperty(QStringLiteral("redirectsmartcards")).has_value())
        requested.append(QStringLiteral("Smart cards (deferred to Phase 3 and disabled)"));
    if(!requested.isEmpty()){
        QMessageBox warning(QMessageBox::Warning,QStringLiteral("Local Resources Requested"),
            QStringLiteral("This connection file requests access to resources on this computer. "
                "OpenRDP will import the connection with these resources disabled."),
            QMessageBox::Cancel|QMessageBox::Open,this);
        warning.setInformativeText(requested.join(QStringLiteral("\n")));
        warning.setDefaultButton(QMessageBox::Cancel);
        if(warning.exec()!=QMessageBox::Open)return false;
    }
    computer_->setText(imported.profile.server);
    username_->setText(imported.profile.username);
    multiMonitor_->setChecked(imported.profile.display.mode==DisplayMode::MultipleMonitors);
    preservedRdpProperties_=imported.source;
    clipboardEnabled_=imported.profile.resources.clipboard==ClipboardSharing::Bidirectional;
    shareClipboard_->setChecked(clipboardEnabled_);
    const auto audio=imported.profile.resources.audioPlayback;audioMode_->setCurrentIndex(audio==AudioPlayback::Remote?1:audio==AudioPlayback::Disabled?2:0);
    shareMicrophone_->setChecked(false);
    sharePrinters_->setChecked(false);
    statusBar()->showMessage(QStringLiteral("Opened %1 with local resource sharing disabled.").arg(QFileInfo(path).fileName()),5000);
    return true;
}
bool MainWindow::writeRdpFile(const QString& path){
    ConnectionProfile profile;
    profile.server=computer_->text().trimmed();profile.username=username_->text();
    profile.authenticationMode=webAccount_->isChecked()?AuthenticationMode::EntraWebAccount:AuthenticationMode::NlaPassword;
    profile.display.mode=multiMonitor_->isChecked()?DisplayMode::MultipleMonitors:DisplayMode::SingleMonitor;
    for(const auto& folder:redirectedFolders_)profile.resources.folders.append({true,folder.name,folder.canonicalPath});
    profile.resources.printers=sharePrinters_->isChecked();if(profile.resources.printers)for(int i=0;i<printerList_->count()&&i<availablePrinters_.size();++i)if(printerList_->item(i)->checkState()==Qt::Checked)profile.resources.printerNames.append(availablePrinters_.at(i).name);
    if(profile.server.isEmpty()){
        QMessageBox::warning(this,QStringLiteral("Save Connection"),QStringLiteral("Enter a computer before saving."));return false;
    }
    const RdpFile exported=RdpProfileMapper().exportFile(profile,preservedRdpProperties_);
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(RdpFileWriter().write(exported))<0||!file.commit()){
        QMessageBox::critical(this,QStringLiteral("Unable to Save Connection File"),file.errorString());return false;
    }
    preservedRdpProperties_=exported;
    statusBar()->showMessage(QStringLiteral("Connection file saved."),3000);return true;
}
void MainWindow::toggleFullScreen(){
    if(isFullScreen()){
        showNormal();
        if(!windowGeometryBeforeFullScreen_.isEmpty())restoreGeometry(windowGeometryBeforeFullScreen_);
        windowGeometryBeforeFullScreen_.clear();
        fullScreenAction_->setChecked(false);
    }else{
        windowGeometryBeforeFullScreen_=saveGeometry();
        showFullScreen();
        fullScreenAction_->setChecked(true);
    }
}
bool MainWindow::eventFilter(QObject* watched,QEvent* event){
    if(watched==display_&&isFullScreen()&&event->type()==QEvent::MouseMove){
        const auto* mouse=static_cast<QMouseEvent*>(event);
        if(mouse->position().y()<=16){sessionToolbar_->show();if(!pinToolbarAction_->isChecked())toolbarHideTimer_->start();}
    }
    return QMainWindow::eventFilter(watched,event);
}
void MainWindow::showConnectionInformation(){
    if(pages_->currentWidget()!=display_){QMessageBox::information(this,QStringLiteral("Connection Information"),QStringLiteral("No remote desktop is connected."));return;}
    QStringList details;
    details<<QStringLiteral("Server: %1").arg(computer_->text().trimmed())
           <<QStringLiteral("User: %1").arg(username_->text().isEmpty()?QStringLiteral("Prompted at connection"):username_->text())
           <<QStringLiteral("Security: TLS with NLA and certificate verification")
           <<QStringLiteral("Monitors: %1").arg(multiMonitor_->isChecked()?QGuiApplication::screens().size():1)
           <<QStringLiteral("Clipboard: %1").arg(clipboardEnabled_?QStringLiteral("Configured on"):QStringLiteral("Off"))
           <<QStringLiteral("Audio: %1").arg(audioMode_->currentText())
           <<QStringLiteral("Microphone: %1").arg(shareMicrophone_->isChecked()?QStringLiteral("Configured on"):QStringLiteral("Off"))
           <<QStringLiteral("Folders: %1").arg(redirectedFolders_.size())
           <<QStringLiteral("Printers: %1").arg(sharePrinters_->isChecked()?QStringLiteral("Configured"):QStringLiteral("Off"))
           <<QStringLiteral("Connected for: %1 seconds").arg(connectedAt_.secsTo(QDateTime::currentDateTimeUtc()));
    QMessageBox::information(this,QStringLiteral("Connection Information"),details.join(u'\n'));
}
void MainWindow::addRedirectedFolder(){
    const QString path=QFileDialog::getExistingDirectory(this,QStringLiteral("Select Local Folder"));if(path.isEmpty())return;
    bool accepted=false;const QString suggested=QFileInfo(path).fileName().isEmpty()?QStringLiteral("Folder"):QFileInfo(path).fileName();
    const QString name=QInputDialog::getText(this,QStringLiteral("Remote Folder Name"),QStringLiteral("Name shown in Windows:"),QLineEdit::Normal,suggested,&accepted);if(!accepted)return;
    QString error;const auto folder=validateRedirectedFolder(name,path,&error);if(!folder){QMessageBox::warning(this,QStringLiteral("Cannot Share Folder"),error);return;}
    auto updated=redirectedFolders_;updated.append(*folder);if(!uniqueFolderNames(updated,&error)){QMessageBox::warning(this,QStringLiteral("Cannot Share Folder"),error);return;}
    redirectedFolders_=updated;folderList_->addItem(QStringLiteral("%1 — %2").arg(folder->name,folder->canonicalPath));
}
void MainWindow::removeRedirectedFolder(){const int row=folderList_->currentRow();if(row<0||row>=redirectedFolders_.size())return;redirectedFolders_.removeAt(row);delete folderList_->takeItem(row);}
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
