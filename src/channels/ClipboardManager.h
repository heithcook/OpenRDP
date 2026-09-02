#pragma once
#include <QObject>
class QClipboard;
namespace openrdp {
class ClipboardManager final:public QObject{
    Q_OBJECT
public: explicit ClipboardManager(QObject* parent=nullptr);
    void setEnabled(bool enabled);
signals: void localTextChanged(QString text);
public slots: void applyRemoteText(QString text);
private slots: void clipboardChanged();
private: QClipboard* clipboard_=nullptr;bool enabled_=false;bool suppressNext_=false;
};
}
