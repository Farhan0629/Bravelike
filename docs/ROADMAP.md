# Roadmap

## Existing foundation

- [x] Independent C++20 core, domain rules, allow precedence, explainable decisions
- [x] Core tests and CLI demo
- [x] Single Windows CEF Views window and toolbar
- [x] Address/search resolution and navigation shortcuts
- [x] Request cancellation and in-memory exact-host Shields
- [x] Cumulative counters, local home page and branding
- [x] Exact-home URL spoof protection and startup-independent Home navigation (review branch)
- [x] Legacy build-option precedence fix (review branch)
- [x] CEF bootstrap sandbox build path (review branch)
- [ ] Windows runtime verification of sandbox and new UI changes

## Reliability before expansion

- [ ] Browser/navigation-scoped Shields policy, including redirects
- [ ] Defined popup and external-protocol behavior
- [ ] Repeatable Windows GUI automation and fixture-driven request tests
- [ ] GPU compatibility benchmarks and crash diagnostics
- [ ] Traffic audit and precise privacy statement
- [ ] Maintainer-authorized license and distribution notices

## Browser essentials

- [ ] Tabs, tab lifecycle and tab-scoped state
- [ ] Download confirmation/progress and safe handling
- [ ] SQLite history and bookmarks
- [ ] Persistent Shields settings and session restore
- [ ] Settings and integrity-checked filter updates/rollback

## Release

- [ ] Validated sandbox, permissions and internal-page boundaries
- [ ] Writable per-user profile storage design
- [ ] Benchmarks, signed installer/update strategy, threat model

VPN, cryptocurrency custody, account sync, a custom rendering engine, and full EasyList compatibility are not first-release promises.
