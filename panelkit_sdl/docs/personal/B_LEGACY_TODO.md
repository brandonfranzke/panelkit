Based on my investigation, here's a comprehensive view of the legacy/transitional systems:

  Legacy & Transitional Systems Still Present

  1. Widget Integration Layer (Biggest)

  - The entire shadow widget system we discussed
  - ~5 files dedicated to this transitional layer
  - Should be removed once widgets are fully primary

  2. Global State Variables in app.c

  Several globals marked with "TODO: Remove":
  - show_time - duplicated in state store
  - small_font - orphaned from removed functions
  - page1_text[] and page1_text_color - hardcoded UI strings
  - current_user_data - duplicated in state store
  - Various UI state not yet in state store (fps counters, debug flags)

  3. API Structure Migration

  The API system is mid-migration:
  - Current: Single-service structure
  - Target: Multi-service structure from config
  - TODOs in api_parsers.c and app.c indicate incomplete migration

  4. Event System Deprecations

  - event_publish() marked as deprecated in favor of event_emit()
  - Legacy event callbacks in app.c that just forward to widget system

  5. Hardcoded UI Elements

  Still present outside widget system:
  - Debug overlay (direct SDL rendering)
  - FPS counter (not a widget)
  - Button dimension macros
  - Color arrays for buttons

  6. Minor Technical Debt

  - State event bridge missing wildcard subscriptions
  - Config manager missing source tracking
  - Error logger missing old file deletion
  - Input evdev missing delta tracking
