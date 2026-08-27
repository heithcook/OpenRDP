#pragma once
#include <QDialog>
class QLineEdit;
namespace openrdp {
class CredentialDialog final : public QDialog {
    Q_OBJECT
public:
    CredentialDialog(const QString& server, const QString& username, QWidget* parent = nullptr);
    QString username() const;
    QString password() const;
private:
    QLineEdit* username_;
    QLineEdit* password_;
};
}
