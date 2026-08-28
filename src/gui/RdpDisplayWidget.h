#pragma once
#include <QImage>
#include <QWidget>
namespace openrdp {
class RdpDisplayWidget final : public QWidget {
    Q_OBJECT
public: explicit RdpDisplayWidget(QWidget* parent = nullptr);
public slots: void setFrame(const QImage& frame); void clear();
signals: void mouseInput(quint16 flags, quint16 x, quint16 y); void keyInput(quint32 virtualKey, bool down, bool repeat);
protected:
    void paintEvent(QPaintEvent*) override; void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override; void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override; void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override; void focusOutEvent(QFocusEvent*) override;
private: QPoint remotePoint(const QPointF&) const; QImage frame_;
    void setRemoteShift(bool down);
    bool remoteShiftDown_ = false;
};
}
