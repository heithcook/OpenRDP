#pragma once
#include <QMainWindow>
#include <QPointer>
#include <QThread>
#include <optional>
#include "app/ConnectionHistory.h"
#include "profiles/RdpFile.h"
#include "profiles/ProfileStore.h"
#include "rdp/RdpSession.h"
class QAction; class QCheckBox; class QComboBox; class QLabel; class QLineEdit; class QListWidget; class QPushButton; class QStackedWidget; class QTimer; class QToolBar;
namespace openrdp {
class RdpDisplayWidget;
class ClipboardManager;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public: explicit MainWindow(const QString& initialServer={}, const QString& initialUser={},
    const QString& initialRdpFile={}, QWidget* parent=nullptr);
    ~MainWindow() override;
protected: void closeEvent(QCloseEvent*) override; bool eventFilter(QObject*,QEvent*) override;
private slots: void connectClicked(); void showCredentials(QString server, QString username);
    void showCertificate(CertificateInfo info); void showError(RdpError error); void sessionEnded();
    void showWebAuthentication(QString authorizationUrl); void selectSavedConnection(int index);
    void openRdpFile(); void saveRdpFileAs();
    void toggleFullScreen();
    void removeRecentConnection(); void clearRecentConnections(); void showConnectionInformation();
    void addRedirectedFolder(); void removeRedirectedFolder();
    void newProfile(); void saveProfile(); void deleteProfile(); void selectProfile(int index);
signals: void startSession(ConnectionSettings settings); void stopSession();
    void credentialsProvided(QString username, QString password, bool accepted); void certificateDecision(bool accepted);
    void webAuthenticationProvided(QString redirectUrl, bool accepted);
    void remoteResizeRequested(QSize size);
private:
    QLineEdit* computer_; QLineEdit* username_; QPushButton* connect_; QStackedWidget* pages_;
    RdpDisplayWidget* display_; RdpSession* session_; QThread worker_;
    QCheckBox* webAccount_; QComboBox* savedConnections_;
    QCheckBox* multiMonitor_;
    QCheckBox* shareClipboard_;
    QComboBox* audioMode_;
    QCheckBox* shareMicrophone_;
    QComboBox* outputDevice_;
    QComboBox* inputDevice_;
    QLabel* microphoneIndicator_;
    QToolBar* sessionToolbar_ = nullptr;
    QLabel* sessionServerLabel_ = nullptr;
    QAction* fullScreenAction_ = nullptr;
    QAction* pinToolbarAction_ = nullptr;
    QListWidget* folderList_;
    QVector<RedirectedFolderConfig> redirectedFolders_;
    QCheckBox* sharePrinters_;
    QListWidget* printerList_;
    QVector<PrinterInfo> availablePrinters_;
    QComboBox* profileSelector_;
    QLineEdit* profileName_;
    ProfileStore profileStore_;
    QVector<ConnectionProfile> profiles_;
    QString currentProfileId_;
    QStringList pendingPrinterNames_;
    ConnectionHistory history_; QVector<ConnectionHistoryEntry> savedEntries_;
    std::optional<ConnectionHistoryEntry> pendingHistoryEntry_;
    RdpFile preservedRdpProperties_;
    QByteArray windowGeometryBeforeFullScreen_;
    QTimer* resizeDebounce_ = nullptr;
    QTimer* toolbarHideTimer_ = nullptr;
    QSize pendingDisplaySize_;
    QVector<MonitorInfo> activeMonitors_;
    QVector<QPointer<QWidget>> monitorWindows_;
    QVector<QPointer<RdpDisplayWidget>> monitorDisplays_;
    bool multiMonitorActive_ = false;
    ClipboardManager* clipboardManager_ = nullptr;
    bool clipboardEnabled_ = true;
    QDateTime connectedAt_;
    void reloadSavedConnections();
    bool loadRdpFile(const QString& path);
    bool writeRdpFile(const QString& path);
    void reloadProfiles();
    ConnectionProfile profileFromEditor() const;
    void applyProfile(const ConnectionProfile& profile);
    void enterMultiMonitorPresentation();
    void leaveMultiMonitorPresentation();
    void updateDisplayFrame(const QImage& frame);
};
}
