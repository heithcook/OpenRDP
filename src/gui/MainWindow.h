#pragma once
#include <QMainWindow>
#include <QThread>
#include <optional>
#include "app/ConnectionHistory.h"
#include "rdp/RdpSession.h"
class QCheckBox; class QComboBox; class QLineEdit; class QPushButton; class QStackedWidget;
namespace openrdp {
class RdpDisplayWidget;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public: explicit MainWindow(const QString& initialServer={}, const QString& initialUser={}, QWidget* parent=nullptr);
    ~MainWindow() override;
protected: void closeEvent(QCloseEvent*) override;
private slots: void connectClicked(); void showCredentials(QString server, QString username);
    void showCertificate(CertificateInfo info); void showError(RdpError error); void sessionEnded();
    void showWebAuthentication(QString authorizationUrl); void selectSavedConnection(int index);
signals: void startSession(ConnectionSettings settings); void stopSession();
    void credentialsProvided(QString username, QString password, bool accepted); void certificateDecision(bool accepted);
    void webAuthenticationProvided(QString redirectUrl, bool accepted);
private:
    QLineEdit* computer_; QLineEdit* username_; QPushButton* connect_; QStackedWidget* pages_;
    RdpDisplayWidget* display_; RdpSession* session_; QThread worker_;
    QCheckBox* webAccount_; QComboBox* savedConnections_;
    ConnectionHistory history_; QVector<ConnectionHistoryEntry> savedEntries_;
    std::optional<ConnectionHistoryEntry> pendingHistoryEntry_;
    void reloadSavedConnections();
};
}
