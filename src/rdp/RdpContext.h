#pragma once

#include <freerdp/freerdp.h>

namespace openrdp { class RdpSession; }

struct OpenRdpContext {
    rdpContext context;
    openrdp::RdpSession* session = nullptr;
};

static_assert(offsetof(OpenRdpContext, context) == 0);
