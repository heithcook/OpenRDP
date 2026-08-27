#include "gui/CertificateDialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
namespace openrdp {
CertificateDialog::CertificateDialog(const CertificateInfo& c, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Remote Computer Identity"));
    auto* layout = new QVBoxLayout(this);
    auto* text = new QLabel(QStringLiteral("<b>The identity of the remote computer could not be verified.</b><br><br>"
        "Server: %1:%2<br>Issued to: %3<br>Subject: %4<br>Issued by: %5<br>SHA-256: %6<br><br>%7")
        .arg(c.server, QString::number(c.port), c.commonName.toHtmlEscaped(), c.subject.toHtmlEscaped(),
             c.issuer.toHtmlEscaped(), c.fingerprint.toHtmlEscaped(), c.reason.toHtmlEscaped()), this);
    text->setTextFormat(Qt::RichText); text->setWordWrap(true); layout->addWidget(text);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto* connectAnyway = buttons->addButton(QStringLiteral("Connect Anyway"), QDialogButtonBox::AcceptRole);
    connect(connectAnyway, &QPushButton::clicked, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject); layout->addWidget(buttons);
}
}
