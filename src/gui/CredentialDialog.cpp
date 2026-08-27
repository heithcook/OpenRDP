#include "gui/CredentialDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
namespace openrdp {
CredentialDialog::CredentialDialog(const QString& server, const QString& username, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("OpenRDP Credentials"));
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("Credentials required for %1").arg(server), this));
    auto* form = new QFormLayout;
    username_ = new QLineEdit(username, this);
    password_ = new QLineEdit(this); password_->setEchoMode(QLineEdit::Password);
    form->addRow(QStringLiteral("User:"), username_); form->addRow(QStringLiteral("Password:"), password_);
    layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Connect"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons); password_->setFocus();
}
QString CredentialDialog::username() const { return username_->text(); }
QString CredentialDialog::password() const { return password_->text(); }
}
