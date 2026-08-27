#include "rdp/RdpRenderer.h"

#include <QMutexLocker>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/codec/color.h>

namespace openrdp {
bool RdpRenderer::initialize(rdpContext* context)
{
    return context && gdi_init(context->instance, PIXEL_FORMAT_BGRA32);
}

void RdpRenderer::shutdown(rdpContext* context)
{
    if (context && context->gdi) gdi_free(context->instance);
    QMutexLocker lock(&mutex_);
    frame_ = {};
}

QImage RdpRenderer::snapshot() const
{
    QMutexLocker lock(&mutex_);
    return frame_.copy();
}
} // namespace openrdp
