# Specification Quality Checklist: Runtime Help Overlay and Command-Line Help

**Purpose**: Validate specification completeness and quality before proceeding to planning  
**Created**: 2026-03-02  
**Feature**: [spec.md](../spec.md)

## Content Quality

- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [x] Written for non-technical stakeholders
- [x] All mandatory sections completed

## Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [x] Success criteria are technology-agnostic (no implementation details)
- [x] All acceptance scenarios are defined
- [x] Edge cases are identified
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria
- [x] User scenarios cover primary flows
- [x] Feature meets measurable outcomes defined in Success Criteria
- [x] No implementation details leak into specification

## Notes

- All items passed initial validation.
- Updated 2026-03-02: Added FR-016 (console matrix rain effect) and FR-017 (pipe/redirect fallback) for CLI help rain reveal. Updated User Story 3 with 8 acceptance scenarios, 3 new edge cases, and 3 new assumptions. SC-002 updated to allow 5s for animation.
- Updated 2026-03-03: Console approach abandoned — FR-016 and FR-017 removed. FR-015 rewritten for graphical rain dialog (HelpRainDialog with D3D11/D2D rendering). US3 rewritten to use custom graphical window instead of console ANSI output. US5 updated to reuse HelpRainDialog. Console-specific edge cases removed, dialog-specific edge cases added. SC-002 retained (5s animation limit still applies to graphical dialog).
- Updated 2026-03-04: Major design refinements — proportional font (Segoe UI) replaces monospace, per-character queued reveal replaces column-based reveal, two independent streak pools (reveal + decorative) replace single pool, rain fills entire window at random pixel x-positions (no column grid). Content split: `/?` dialog shows switches only (Options + Screensaver Options, no hotkeys), `?` key uses in-app overlay for hotkeys (NOT HelpRainDialog). BuildTextGrid/BuildColumnActivityMap superseded. UnicodeSymbols.h added. New tasks T085–T097.
- Spec is ready for implementation.
