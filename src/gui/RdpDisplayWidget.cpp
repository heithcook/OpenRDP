#include "gui/RdpDisplayWidget.h"
#include "rdp/RdpInput.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <freerdp/input.h>
namespace openrdp {
RdpDisplayWidget::RdpDisplayWidget(QWidget* parent) : QWidget(parent) { setFocusPolicy(Qt::StrongFocus); setMouseTracking(true); setMinimumSize(640, 360); }
void RdpDisplayWidget::setFrame(const QImage& frame) { frame_ = frame; update(); }
void RdpDisplayWidget::clear() { frame_ = {}; update(); }
void RdpDisplayWidget::paintEvent(QPaintEvent*) { QPainter p(this); p.fillRect(rect(), Qt::black); if (!frame_.isNull()) p.drawImage(rect(), frame_); }
QPoint RdpDisplayWidget::remotePoint(const QPointF& p) const { return scaleMousePosition(p.toPoint(), size(), frame_.size()); }
void RdpDisplayWidget::mouseMoveEvent(QMouseEvent* e) { const auto p=remotePoint(e->position()); emit mouseInput(PTR_FLAGS_MOVE,p.x(),p.y()); }
void RdpDisplayWidget::mousePressEvent(QMouseEvent* e) { const auto p=remotePoint(e->position()); quint16 f=PTR_FLAGS_DOWN; if(e->button()==Qt::LeftButton)f|=PTR_FLAGS_BUTTON1; else if(e->button()==Qt::RightButton)f|=PTR_FLAGS_BUTTON2; else if(e->button()==Qt::MiddleButton)f|=PTR_FLAGS_BUTTON3; emit mouseInput(f,p.x(),p.y()); }
void RdpDisplayWidget::mouseReleaseEvent(QMouseEvent* e) { const auto p=remotePoint(e->position()); quint16 f=0; if(e->button()==Qt::LeftButton)f=PTR_FLAGS_BUTTON1; else if(e->button()==Qt::RightButton)f=PTR_FLAGS_BUTTON2; else if(e->button()==Qt::MiddleButton)f=PTR_FLAGS_BUTTON3; emit mouseInput(f,p.x(),p.y()); }
void RdpDisplayWidget::wheelEvent(QWheelEvent* e) { const auto p=remotePoint(e->position()); const int delta=e->angleDelta().y(); quint16 f=PTR_FLAGS_WHEEL | static_cast<quint16>(qMin(qAbs(delta),255)); if(delta<0)f|=PTR_FLAGS_WHEEL_NEGATIVE; emit mouseInput(f,p.x(),p.y()); }
void RdpDisplayWidget::setRemoteShift(const bool down) {
    if (remoteShiftDown_ == down) return;
    remoteShiftDown_ = down;
    emit keyInput(VK_LSHIFT, down, false);
}
void RdpDisplayWidget::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Shift) { setRemoteShift(true); return; }
    setRemoteShift(e->modifiers().testFlag(Qt::ShiftModifier));
    emit keyInput(qtKeyToVirtualKey(static_cast<Qt::Key>(e->key())),true,e->isAutoRepeat());
}
void RdpDisplayWidget::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Shift) { setRemoteShift(false); return; }
    emit keyInput(qtKeyToVirtualKey(static_cast<Qt::Key>(e->key())),false,e->isAutoRepeat());
}
void RdpDisplayWidget::focusOutEvent(QFocusEvent* e) { setRemoteShift(false); QWidget::focusOutEvent(e); }
}
