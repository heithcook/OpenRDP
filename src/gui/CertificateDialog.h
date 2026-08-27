#pragma once
#include <QDialog>
#include "rdp/RdpSession.h"
namespace openrdp {
class CertificateDialog final : public QDialog {
    Q_OBJECT
public: explicit CertificateDialog(const CertificateInfo& info, QWidget* parent = nullptr);
};
}
