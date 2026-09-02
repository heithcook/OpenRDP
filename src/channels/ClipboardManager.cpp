#include "channels/ClipboardManager.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
namespace openrdp {
ClipboardManager::ClipboardManager(QObject* parent):QObject(parent),clipboard_(QGuiApplication::clipboard())
{connect(clipboard_,&QClipboard::dataChanged,this,&ClipboardManager::clipboardChanged);}
void ClipboardManager::setEnabled(bool enabled){enabled_=enabled;suppressNext_=false;if(enabled_&&clipboard_->mimeData()->hasText())emit localTextChanged(clipboard_->text());}
void ClipboardManager::applyRemoteText(QString text){if(!enabled_)return;if(clipboard_->text()==text)return;suppressNext_=true;clipboard_->setText(text,QClipboard::Clipboard);}
void ClipboardManager::clipboardChanged(){if(!enabled_)return;if(suppressNext_){suppressNext_=false;return;}const QMimeData* data=clipboard_->mimeData();if(data&&data->hasText())emit localTextChanged(data->text());}
}
