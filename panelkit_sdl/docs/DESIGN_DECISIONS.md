# Design Decisions

This document tracks significant architectural and implementation decisions made during PanelKit development, including the tradeoffs considered. These decisions should be periodically reviewed as requirements evolve.

## Table of Contents
- [Dependency Management](#dependency-management)
  - [HTTP/2 Support in libcurl](#http2-support-in-libcurl)
- [Display Architecture](#display-architecture)
- [Logging System](#logging-system)

---

## Dependency Management

### HTTP/2 Support in libcurl

**Decision Date:** 2025-01-23  
**Status:** Implemented  
**Decision:** Disable HTTP/2 support in libcurl (--without-nghttp2)

#### Context
When building libcurl statically for the target device, we encountered linking errors due to missing nghttp2 symbols. We had to choose between adding nghttp2 as a dependency or disabling HTTP/2 support.

#### Alternatives Considered

1. **Enable HTTP/2 (add nghttp2 dependency)**
   - Pros:
     - Full HTTP/2 support with multiplexing and header compression
     - Better performance for multiple concurrent API requests
     - Lower latency for modern web services
     - Future-proof as HTTP/2 adoption grows
   - Cons:
     - Additional ~200KB library dependency
     - More complex build process
     - Larger binary size
     - Another dependency to maintain

2. **Disable HTTP/2 (chosen approach)**
   - Pros:
     - Smaller binary size and memory footprint
     - Fewer dependencies (aligns with appliance philosophy)
     - Simpler deployment
     - Reduced attack surface
   - Cons:
     - No HTTP/2 multiplexing benefits
     - Potentially slower with multiple concurrent requests
     - Some modern APIs may prefer HTTP/2

#### Rationale
- Current API usage is simple weather/data fetches without many concurrent requests
- HTTP/1.1 is still universally supported
- WebSocket support (for future live subscriptions) works fine over HTTP/1.1
- Appliance philosophy favors minimal dependencies
- Can be revisited if performance requirements change

#### Future Considerations
- Monitor if APIs begin requiring HTTP/2
- Benchmark performance difference for actual use cases
- Consider adding if multiple concurrent API calls become common

---

## Display Architecture

### SDL+DRM Backend Implementation

**Decision Date:** 2025-01-23  
**Status:** Implemented  
**Decision:** Create abstraction layer with runtime-selectable backends

#### Context
SDL2's framebuffer drivers were not available on Raspberry Pi 5, requiring a custom solution for display output.

#### Implementation
- Display backend interface allowing runtime selection
- Standard SDL backend for development
- SDL+DRM backend for production (requires only libdrm ~200KB vs Mesa ~169MB)
- Command-line selection via --display-backend

#### Rationale
- Maintains single codebase philosophy
- Allows development on host without DRM
- Minimal dependencies on target
- Clean abstraction for future backends

---

## Logging System

### Zlog Integration

**Decision Date:** 2025-01-22  
**Status:** Implemented  
**Decision:** Use zlog library with custom abstraction layer

#### Context
Needed production-ready logging for remote debugging of deployed devices.

#### Alternatives Considered
1. Custom implementation (too much effort)
2. syslog (limited features)
3. log4c (abandoned project)
4. zlog (chosen - active, thread-safe, feature-rich)

#### Implementation
- File-based logging to /var/log/panelkit/
- Automatic rotation at 10MB
- Multiple log levels
- System info logging at startup
- Panic handlers for crashes

---

## Future Decision Points

### To Be Decided
- [ ] API communication protocol (REST vs GraphQL vs WebSocket)
- [ ] Configuration management (files vs embedded vs remote)
- [ ] Update mechanism for deployed devices
- [ ] Metrics and telemetry approach
- [ ] Security/authentication for API calls

### Review Schedule
- Quarterly review of HTTP/2 decision based on API requirements
- Review display backend if new hardware platforms are added
- Review logging if remote log aggregation is needed