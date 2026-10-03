// KINGFN Shields - in-page cosmetic filtering and YouTube ad skipper.
// Injected by the browser into HTTP(S) main frames while Shields are on.
(function () {
  'use strict';
  if (window.__kingfnShields) return;
  window.__kingfnShields = true;

  var host = location.hostname.replace(/^www\./, '');
  var isYouTube = /(^|\.)youtube\.com$/.test(host) || /(^|\.)youtube-nocookie\.com$/.test(host);

  // ---------------- Cosmetic filters ----------------
  var generic = [
    'ins.adsbygoogle',
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
    '[data-ad-slot]',
    '[data-ad-unit]',
    '[data-adunit]',
    '.adsbygoogle',
    '.ad-slot',
    '.ad-container',
    '.ad-banner',
    '.ad-wrapper',
    '.advertisement',
    '.sponsored-ad'
  ];

  var youtube = [
    '#masthead-ad',
    '#player-ads',
    '#panels > ytd-engagement-panel-section-list-renderer[target-id="engagement-panel-ads"]',
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
    '.ytp-ad-overlay-container',
    '.ytp-ad-overlay-slot',
    '.ytp-ad-image-overlay',
    '.ytp-ad-text-overlay',
    '.ytp-featured-product',
    '.ytp-suggested-action'
  ];

  var chess = [
    '#board-layout-ad',
    '.board-layout-ad',
    '#below-board-ad',
    '#tall-sidebar-ad',
    '#sidebar-ad',
    '.sidebar-ad',
    '.sidebar-ad-component',
    '[id*="sidebar-ad"]',
    '[class*="ad-slot"]',
    '[class*="ad-component"]',
    '[class*="ads-component"]',
    '[id^="placeholder-ad"]',
    '[id*="-ad-container"]',
    '[class*="-ad-container"]',
    '.ad-placeholder',
    '.ad-component-wrapper',
    '.bottom-banner-ad',
    '.video-ad-container'
  ];

  var selectors = generic.slice();
  if (isYouTube) selectors = selectors.concat(youtube);
  if (/(^|\.)chess\.com$/.test(host)) selectors = selectors.concat(chess);

  function injectCss() {
    if (document.getElementById('kingfn-shields-css')) return;
    var style = document.createElement('style');
    style.id = 'kingfn-shields-css';
    // Apply each selector separately so one unsupported selector can't
    // invalidate the whole rule set.
    style.textContent = selectors.map(function (s) {
      return s + '{display:none !important;visibility:hidden !important;}';
    }).join('\n');
    (document.head || document.documentElement).appendChild(style);
  }

  if (document.documentElement) injectCss();
  document.addEventListener('DOMContentLoaded', injectCss);

  // ---------------- YouTube video ad skipper ----------------
  if (!isYouTube) return;

  var skipSelectors = [
    '.ytp-skip-ad-button',
    '.ytp-ad-skip-button',
    '.ytp-ad-skip-button-modern',
    '.ytp-ad-skip-button-slot button',
    '.ytp-ad-skip-button-container button',
    'button.ytp-ad-skip-button-modern',
    '.ytp-ad-overlay-close-button'
  ];

  var adActive = false;
  var savedMuted = false;
  var savedRate = 1;

  function clickSkip(root) {
    for (var i = 0; i < skipSelectors.length; i++) {
      var btn = root.querySelector(skipSelectors[i]);
      if (btn) { try { btn.click(); } catch (e) {} }
    }
  }

  function tick() {
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
          video.currentTime < video.duration - 0.1) {
        try { video.currentTime = video.duration; } catch (e) {}
      }
      clickSkip(player);
    } else if (adActive) {
      adActive = false;
      if (video) {
        video.muted = savedMuted;
        try { video.playbackRate = savedRate; } catch (e) {}
      }
    }

    // Overlay / banner ads inside the player.
    clickSkip(document);
  }

  setInterval(tick, 200);
})();
