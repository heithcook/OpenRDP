#pragma once

#include <QImage>
#include <QMutex>

struct rdp_context;
typedef struct rdp_context rdpContext;

namespace openrdp {
class RdpRenderer {
public:
    bool initialize(rdpContext* context);
    void shutdown(rdpContext* context);
    QImage snapshot() const;
private:
    mutable QMutex mutex_;
    QImage frame_;
};
} // namespace openrdp
