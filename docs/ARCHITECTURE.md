# Architecture

## Design goals

- Keep filtering independent from CEF so it can be tested quickly.
- Never block a request without retaining an explainable matching rule.
- Default to allowing unsupported syntax instead of silently over-blocking.
- Store browsing information locally unless a future feature explicitly says otherwise.

## Components

```text
CEF browser process
  -> request interception adapter (planned)
      -> FilterEngine
          -> block/allow rule index
      -> PrivacyStats
  -> Shields panel (planned)
  -> history/bookmark repositories (planned)
```

`FilterEngine` accepts a URL and returns an action, reason, and matching source rule. The browser adapter will translate CEF resource-request callbacks into this stable interface. This prevents CEF-specific types from spreading through the core.

## Security boundaries

The renderer must not directly modify filter rules, history, or browser settings. Those operations belong to the browser process. Before a public release, the CEF sandbox must be enabled, external protocols must require confirmation, downloads must be sanitized, and navigation to privileged internal pages must be restricted.
