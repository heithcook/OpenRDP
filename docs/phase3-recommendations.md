# Phase 3 recommendations

Last updated: 2026-08-28

This file records deferred scope only. Phase 3 work must not begin during Phase 2.

## Smart-card redirection

Smart-card redirection was explicitly moved from Phase 2 to Phase 3 on 2026-08-28.
Future implementation should use FreeRDP's smart-card RDPDR integration with PC/SC and
`pcsc-lite`, keep PINs/private keys/authentication material out of storage and logs, and
distinguish service, reader, card, server-policy, and channel failures. It requires real
reader/card testing before PASS; without hardware it must be reported as
**IMPLEMENTED — NOT VALIDATED**.

OpenRDP continues to parse and preserve unknown `.rdp` properties safely. A Phase 2
connection must not activate `redirectsmartcards` even when an imported file requests it.
