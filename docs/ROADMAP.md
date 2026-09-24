# Roadmap

## M0 — Repository and privacy core

- [x] CMake C++20 project
- [x] Domain/subdomain rules
- [x] Allow-rule precedence
- [x] Explainable decisions
- [x] Thread-safe counters
- [x] Unit tests and CLI demo
- [x] CEF shell scaffold

## M1 — Windows browser shell

- [x] Pin and verify a CEF binary release
- [ ] Enable the CEF sandbox
- [x] Create a single-window browser UI
- [ ] Add a tab strip and tab lifecycle
- [x] Address bar and navigation controls
- [ ] Browser-process request logging
- [x] Connect FilterEngine to request interception

## M2 — Browser essentials

- [ ] Multiple tabs and keyboard shortcuts
- [ ] New-tab page
- [ ] Downloads with confirmation and status
- [ ] SQLite history and bookmarks
- [ ] Session restoration

## M3 — Shields experience

- [x] In-memory per-site Shields toggle, keyed by exact host
- [ ] Persist the site allowlist and scope requests by browser/tab during navigation
- [x] Cumulative blocked-request counter
- [ ] Explain matched rules and categories in UI
- [ ] Settings for filter-list sources
- [ ] Safe list update and rollback

## M4 — Evaluation and release

- [ ] Repeatable benchmark harness
- [ ] CSV/JSON metrics export
- [ ] Windows installer
- [ ] Threat model and privacy statement
- [ ] Final report figures and demo script

## Explicit non-goals for the first release

VPN infrastructure, cryptocurrency custody, account synchronization, a custom rendering engine, and claims of complete EasyList compatibility.
