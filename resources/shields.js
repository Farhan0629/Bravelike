// KINGFN Shields v3 - cosmetic filtering + YouTube/Twitch ad elimination.
// Injected into every HTTP(S) main frame while Shields are on.
// Runs at OnLoadStart (as early as possible) AND re-runs on SPA navigation.
(function () {
  'use strict';

  if (!window.__kingfnShields) {
    window.__kingfnShields = true;
    bootstrap();
  } else {
    // SPA navigation (same window reuse): re-arm platform-specific killers.
    var host = location.hostname.replace(/^www\./, '');
    if (/(?:^|\.)youtube(?:-nocookie)?\.com$/.test(host)) startYouTubeKiller();
    if (/(?:^|\.)twitch\.tv$/.test(host)) startTwitchKiller();
  }

  function bootstrap() {
    var host = location.hostname.replace(/^www\./, '');
    var isYT     = /(?:^|\.)youtube(?:-nocookie)?\.com$/.test(host);
    var isChess  = /(?:^|\.)chess\.com$/.test(host);
    var isTwitch = /(?:^|\.)twitch\.tv$/.test(host);

    injectCss(buildSelectors(isYT, isChess, isTwitch));

    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', function () {
        injectCss(buildSelectors(isYT, isChess, isTwitch));
      });
    }

    if (isYT)     startYouTubeKiller();
    if (isTwitch) startTwitchKiller();

    // History API hook for SPA navigation detection (YouTube, Twitch, etc.)
    hookHistoryApi(function () {
      setTimeout(function () {
        var h = location.hostname.replace(/^www\./, '');
        injectCss(buildSelectors(
          /(?:^|\.)youtube(?:-nocookie)?\.com$/.test(h),
          /(?:^|\.)chess\.com$/.test(h),
          /(?:^|\.)twitch\.tv$/.test(h)
        ));
        if (/(?:^|\.)youtube(?:-nocookie)?\.com$/.test(h)) startYouTubeKiller();
        if (/(?:^|\.)twitch\.tv$/.test(h)) startTwitchKiller();
      }, 50);
    });
  }

  // ─── CSS selectors ─────────────────────────────────────────────────────────
  function buildSelectors(isYT, isChess, isTwitch) {
    var generic = [
      'ins.adsbygoogle', '.adsbygoogle',
      'div[id^="div-gpt-ad"]',
      'div[id^="google_ads_iframe"]',
      'iframe[id^="google_ads_iframe"]',
      'iframe[src*="doubleclick.net"]',
      'iframe[src*="googlesyndication.com"]',
      'iframe[src*="amazon-adsystem.com"]',
      'iframe[src*="taboola.com"]',
      'div[id^="taboola-"]',
      '.OUTBRAIN',
      '[data-google-query-id]',
      '[data-ad-slot]', '[data-ad-unit]', '[data-adunit]',
      '.ad-slot', '.ad-container', '.ad-banner', '.ad-wrapper',
      '.advertisement', '.sponsored-ad',
      '[id*="advertisement"]', '[class*="advertisement"]',
      '.sponsored-content', '[class*="sponsored-content"]',
      'div[class*="AdSlot"]', 'div[id*="AdSlot"]',
      '.ad-unit', '[class*="google-ad"]', '[id*="google-ad"]',
      'div[data-testid*="ad"]', 'div[aria-label*="Advertisement"]'
    ];

    var youtube = [
      '#masthead-ad', '#player-ads',
      'ytd-ad-slot-renderer',
      'ytd-in-feed-ad-layout-renderer',
      'ytd-banner-promo-renderer',
      'ytd-statement-banner-renderer',
      'ytd-promoted-sparkles-web-renderer',
      'ytd-promoted-sparkles-text-search-renderer',
      'ytd-promoted-video-renderer',
      'ytd-display-ad-renderer',
      'ytd-companion-slot-renderer',
      'ytd-action-companion-ad-renderer',
      'ytd-player-legacy-desktop-watch-ads-renderer',
      'ytd-search-pyv-renderer',
      'ytd-video-masthead-ad-v3-renderer',
      'ytd-primetime-promo-renderer',
      'ytd-merch-shelf-renderer',
      'ytd-rich-item-renderer:has(> #content > ytd-ad-slot-renderer)',
      'ytd-rich-section-renderer:has(ytd-statement-banner-renderer)',
      '.ytd-mealbar-promo-renderer',
      'ytm-promoted-sparkles-web-renderer',
      '.ytp-ad-overlay-container', '.ytp-ad-overlay-slot',
      '.ytp-ad-image-overlay', '.ytp-ad-text-overlay',
      '.ytp-featured-product', '.ytp-suggested-action',
      '.ytp-ad-progress', '.ytp-ad-progress-list',
      '.ytp-ad-preview-container', '.ytp-ad-preview-text-modern',
      '.ytp-ad-button-icon', '.ytp-ad-visit-advertiser-button',
      '.ytp-ad-module',
      '.ytp-ce-element.ytp-ce-element-shadow',
      'div.ytp-ad-player-overlay',
      'div.ytp-ad-player-overlay-instream-info',
      '.ad-container.ytp-ad-player-overlay',
      '#panels > ytd-engagement-panel-section-list-renderer[target-id="engagement-panel-ads"]'
    ];

    var twitch = [
      '.video-ads', '.tw-ad', '.ad-banner-default',
      '[class*="ad-slot"]', '[data-a-target*="ad"]',
      '.player-ad-notice', '.video-player__ad-notice',
      '.tw-countdown', '.stream-chat-ad-container',
      '[data-test-selector="ad-overlay"]',
      '.ad-countdown', '.layout-advert',
      // Twitch ad countdown overlay
      'div[data-a-target="video-ad-countdown"]',
      'div[data-a-target="player-overlay-ad"]'
    ];

    var chess = [
      '#board-layout-ad', '.board-layout-ad', '#below-board-ad',
      '#tall-sidebar-ad', '#sidebar-ad', '.sidebar-ad',
      '.sidebar-ad-component', '[id*="sidebar-ad"]',
      '[class*="ad-slot"]', '[class*="ad-component"]',
      '[class*="ads-component"]', '[id^="placeholder-ad"]',
      '[id*="-ad-container"]', '[class*="-ad-container"]',
      '.ad-placeholder', '.ad-component-wrapper',
      '.bottom-banner-ad', '.video-ad-container'
    ];

    var sel = generic.slice();
    if (isYT)     sel = sel.concat(youtube);
    if (isTwitch) sel = sel.concat(twitch);
    if (isChess)  sel = sel.concat(chess);
    return sel;
  }

  function injectCss(selectors) {
    var style = document.getElementById('kingfn-shields-css');
    if (!style) {
      style = document.createElement('style');
      style.id = 'kingfn-shields-css';
      (document.head || document.documentElement).appendChild(style);
    }
    style.textContent = selectors.map(function (s) {
      return s + '{display:none!important;visibility:hidden!important;pointer-events:none!important;}';
    }).join('\n');
  }

  // ─── YouTube ad killer ─────────────────────────────────────────────────────
  var ytKillerRunning = false;
  var ytInterval = null;
  var ytObserver = null;

  function startYouTubeKiller() {
    interceptYouTubeAdRequests();
    if (ytKillerRunning) return;
    ytKillerRunning = true;

    if (ytInterval) clearInterval(ytInterval);
    ytInterval = setInterval(ytTick, 100);

    if (ytObserver) ytObserver.disconnect();
    ytObserver = new MutationObserver(function (mutations) {
      var dirty = false;
      for (var i = 0; i < mutations.length; i++) {
        var added = mutations[i].addedNodes;
        for (var j = 0; j < added.length; j++) {
          if (added[j].nodeType === 1 && isYtAdNode(added[j])) {
            hideNode(added[j]);
            dirty = true;
          }
        }
        if (mutations[i].type === 'attributes') dirty = true;
      }
      if (dirty) ytTick();
    });
    ytObserver.observe(document.documentElement, {
      childList: true, subtree: true, attributes: true,
      attributeFilter: ['class']
    });
  }

  var ytAdTags = {
    'YTD-AD-SLOT-RENDERER': 1, 'YTD-IN-FEED-AD-LAYOUT-RENDERER': 1,
    'YTD-DISPLAY-AD-RENDERER': 1, 'YTD-COMPANION-SLOT-RENDERER': 1,
    'YTD-ACTION-COMPANION-AD-RENDERER': 1, 'YTD-PROMOTED-VIDEO-RENDERER': 1,
    'YTD-PROMOTED-SPARKLES-WEB-RENDERER': 1, 'YTD-BANNER-PROMO-RENDERER': 1,
    'YTD-STATEMENT-BANNER-RENDERER': 1, 'YTD-VIDEO-MASTHEAD-AD-V3-RENDERER': 1
  };

  function isYtAdNode(n) {
    if (ytAdTags[n.tagName]) return true;
    var id = n.id || '', cls = n.className || '';
    if (typeof cls !== 'string') cls = '';
    return id === 'player-ads' || id === 'masthead-ad' ||
           cls.indexOf('ytp-ad-') === 0 || cls.indexOf('ad-showing') >= 0;
  }

  function hideNode(n) {
    n.style.cssText += 'display:none!important;visibility:hidden!important;';
  }

  var skipSels = [
    '.ytp-skip-ad-button', '.ytp-ad-skip-button',
    '.ytp-ad-skip-button-modern', 'button.ytp-ad-skip-button-modern',
    '.ytp-ad-skip-button-slot button', '.ytp-ad-skip-button-container button',
    '.ytp-ad-overlay-close-button'
  ];
  var adActive = false, savedMuted = false, savedRate = 1;

  function ytTick() {
    var player = document.querySelector('#movie_player, .html5-video-player');
    if (!player) return;
    var video = player.querySelector('video');
    var showingAd = player.classList.contains('ad-showing') ||
                    player.classList.contains('ad-interrupting');

    if (showingAd && video) {
      if (!adActive) {
        adActive = true;
        savedMuted = video.muted;
        savedRate = video.playbackRate || 1;
      }
      video.muted = true;
      try { video.playbackRate = 16; } catch (e) {}
      if (isFinite(video.duration) && video.duration > 0 &&
          video.currentTime < video.duration - 0.05) {
        try { video.currentTime = video.duration - 0.05; } catch (e) {}
      }
      clickSkip(player);
      clickSkip(document);
      var overlay = player.querySelector('.ytp-ad-player-overlay') ||
                    player.querySelector('.ad-container');
      if (overlay) hideNode(overlay);
    } else if (adActive) {
      adActive = false;
      if (video) {
        video.muted = savedMuted;
        try { video.playbackRate = savedRate; } catch (e) {}
      }
    }
    clickSkip(document);
    nukeYtOverlays(player);
  }

  function clickSkip(root) {
    for (var i = 0; i < skipSels.length; i++) {
      var btn = root.querySelector(skipSels[i]);
      if (btn && btn.offsetParent !== null) try { btn.click(); } catch (e) {}
    }
  }

  function nukeYtOverlays(player) {
    if (!player) return;
    var els = player.querySelectorAll(
      '.ytp-ad-overlay-container,.ytp-ad-image-overlay,' +
      '.ytp-ad-text-overlay,.ytp-ad-overlay-slot,' +
      '.ytp-ad-player-overlay,.ytp-ad-module,' +
      'div.ytp-ad-player-overlay-instream-info'
    );
    for (var i = 0; i < els.length; i++) hideNode(els[i]);
  }

  // ─── YouTube fetch/XHR intercept ───────────────────────────────────────────
  var ytAdPaths = [
    '/youtubei/v1/log_event', '/youtubei/v1/ad_break',
    '/api/stats/ads', '/api/stats/atr', '/pagead/',
    '/ptracking', '/get_midroll_info', '/pcs/activeview',
    '/gen_204?adthru', '/pagead/adview', '/pagead/conversion'
  ];
  function isYtAdUrl(url) {
    if (!url) return false;
    if (typeof url !== 'string') { try { url = url.toString(); } catch(e) { return false; } }
    for (var i = 0; i < ytAdPaths.length; i++)
      if (url.indexOf(ytAdPaths[i]) >= 0) return true;
    return false;
  }

  var _fetchDone = false, _xhrDone = false;
  function interceptYouTubeAdRequests() {
    if (!_fetchDone && typeof window.fetch === 'function') {
      _fetchDone = true;
      var origFetch = window.fetch.bind(window);
      window.fetch = function (resource, init) {
        var url = (resource && resource.url) ? resource.url : resource;
        if (isYtAdUrl(url))
          return Promise.resolve(new Response('{}', { status: 200,
            headers: { 'Content-Type': 'application/json' } }));
        return origFetch(resource, init);
      };
    }
    if (!_xhrDone) {
      _xhrDone = true;
      var OrigXHR = window.XMLHttpRequest;
      function PatchedXHR() {
        var xhr = new OrigXHR();
        var _open = xhr.open.bind(xhr), _send = xhr.send.bind(xhr), _blocked = false;
        xhr.open = function (m, u) { if (isYtAdUrl(u)) _blocked = true; return _open.apply(xhr, arguments); };
        xhr.send = function () { if (_blocked) return; return _send.apply(xhr, arguments); };
        return xhr;
      }
      PatchedXHR.prototype = OrigXHR.prototype;
      window.XMLHttpRequest = PatchedXHR;
    }
  }

  // ─── Twitch ad killer ─────────────────────────────────────────────────────
  // Twitch uses a different approach: it injects ads directly into the HLS
  // stream. The best JS-level approach is to detect the ad state and mute/
  // skip, while the C++ layer blocks the ad server domains.
  var twitchInterval = null;
  var twitchAdActive = false;

  function startTwitchKiller() {
    if (twitchInterval) return;
    twitchInterval = setInterval(twitchTick, 500);
    interceptTwitchAdRequests();
  }

  var twitchAdUrls = [
    'usher.twitchsvc.net',
    'jtvnw.net/ad',
    'twitchadvertising.tv',
    '/ad_break',
    '/midroll',
    'spade.twitch.tv'
  ];
  function isTwitchAdUrl(url) {
    if (!url) return false;
    for (var i = 0; i < twitchAdUrls.length; i++)
      if (url.indexOf(twitchAdUrls[i]) >= 0) return true;
    return false;
  }

  var _twitchFetchDone = false, _twitchXhrDone = false;
  function interceptTwitchAdRequests() {
    if (!_twitchFetchDone && typeof window.fetch === 'function') {
      _twitchFetchDone = true;
      var origFetch2 = window.fetch.bind(window);
      window.fetch = function (resource, init) {
        var url = (resource && resource.url) ? resource.url : resource;
        if (isTwitchAdUrl(url))
          return Promise.resolve(new Response('{"status":"ok"}',
            { status: 200, headers: { 'Content-Type': 'application/json' } }));
        return origFetch2(resource, init);
      };
    }
    if (!_twitchXhrDone) {
      _twitchXhrDone = true;
      var OrigXHR2 = window.XMLHttpRequest;
      function PatchedXHR2() {
        var xhr2 = new OrigXHR2();
        var _open2 = xhr2.open.bind(xhr2), _send2 = xhr2.send.bind(xhr2), _b2 = false;
        xhr2.open = function (m, u) { if (isTwitchAdUrl(u)) _b2 = true; return _open2.apply(xhr2, arguments); };
        xhr2.send = function () { if (_b2) return; return _send2.apply(xhr2, arguments); };
        return xhr2;
      }
      PatchedXHR2.prototype = OrigXHR2.prototype;
      window.XMLHttpRequest = PatchedXHR2;
    }
  }

  function twitchTick() {
    // Detect Twitch ad overlay and hide it.
    var adNotice = document.querySelector('.video-ads,.player-ad-notice,.ad-banner-default');
    if (adNotice) {
      hideNode(adNotice);
      var video = document.querySelector('video');
      if (video && !twitchAdActive) {
        twitchAdActive = true;
        video.muted = true;
      }
    } else if (twitchAdActive) {
      twitchAdActive = false;
      var video2 = document.querySelector('video');
      if (video2) video2.muted = false;
    }
    // Nuke countdown overlays.
    var overlays = document.querySelectorAll(
      '[data-a-target="video-ad-countdown"],[data-a-target="player-overlay-ad"],' +
      '.tw-countdown,.ad-countdown,.layout-advert'
    );
    for (var i = 0; i < overlays.length; i++) hideNode(overlays[i]);
  }

  // ─── History API hook for SPA navigation ──────────────────────────────────
  function hookHistoryApi(callback) {
    if (window.__kingfnHistoryHooked) return;
    window.__kingfnHistoryHooked = true;
    function wrap(orig) {
      return function () {
        var r = orig.apply(this, arguments);
        callback();
        return r;
      };
    }
    if (history.pushState)    history.pushState    = wrap(history.pushState);
    if (history.replaceState) history.replaceState = wrap(history.replaceState);
    window.addEventListener('popstate', callback);
  }

})();
