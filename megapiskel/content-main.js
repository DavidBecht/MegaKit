// Laeuft in der MAIN-Welt (Seiten-Kontext) → hat Zugriff auf window.pskl
//
// Nachrichten mit content-ui.js (ISOLATED-Welt), immer als CustomEvent auf window:
//   megacard-read-pixels    → megacard-pixels-result  aktuellen Frame auslesen
//   megacard-import (JSON)  → megacard-import-result  Bitmap in Piskel schreiben
// Import-Daten gehen als JSON-Text hin und her, damit sie sicher zwischen den
// beiden Welten ankommen.

(function () {
  'use strict';

  console.log('[MegaCard] MAIN-Script geladen');

  function controller() {
    return (window.pskl && window.pskl.app && window.pskl.app.piskelController) ||
           (window.pskl && window.pskl.controller) ||
           null;
  }

  // Name des Sprites aus Piskel (Feld „Name“ beim Speichern), leer wenn unbekannt
  function spriteName(piskel) {
    try {
      var d = piskel.getDescriptor ? piskel.getDescriptor() : piskel.descriptor;
      return (d && d.name) || '';
    } catch (e) {
      return '';
    }
  }

  function currentFrameIndex(ctrl) {
    if (ctrl.getCurrentFrameIndex) return ctrl.getCurrentFrameIndex();
    if (typeof ctrl.currentFrameIndex === 'number') return ctrl.currentFrameIndex;
    return 0;
  }

  // ── Farben ────────────────────────────────────────────────────────────────
  //
  // Piskel speichert jeden Pixel als Uint32 mit den Bytes R, G, B, A im Speicher,
  // als Zahl gelesen also 0xAABBGGRR. 0 ist vollständig transparent.

  var FARBE_AN = '#ffffff';    // Import: gesetzter Pixel
  var FARBE_AUS = '#000000';   // Import: Hintergrund

  function pixelLesen(frame, x, y, w) {
    if (typeof frame.getPixel === 'function') return frame.getPixel(x, y) >>> 0;
    return frame.pixels[y * w + x] >>> 0;   // Fallback: flaches Array, zeilenweise
  }

  function istTransparent(p) {
    return (p >>> 24) === 0;
  }

  // Wahrgenommene Helligkeit, ab der Hälfte gilt ein Pixel als an
  function istHell(p) {
    var r = p & 0xFF, g = (p >>> 8) & 0xFF, b = (p >>> 16) & 0xFF;
    return (299 * r + 587 * g + 114 * b) / 1000 >= 128;
  }

  // ── Export: aktuellen Frame auslesen ──────────────────────────────────────

  window.addEventListener('megacard-read-pixels', function () {
    console.log('[MegaCard] Event empfangen');

    try {
      var ctrl = controller();
      if (!ctrl) {
        window.dispatchEvent(new CustomEvent('megacard-pixels-result', {
          detail: { error: 'Controller nicht gefunden.' }
        }));
        return;
      }

      var piskel = ctrl.getPiskel ? ctrl.getPiskel() : ctrl.piskel;
      var W = piskel.getWidth();
      var H = piskel.getHeight();
      var fi = currentFrameIndex(ctrl);

      console.log('[MegaCard] Sprite:', W + 'x' + H, 'Frame:', fi);

      // Wie auf dem Display: helle Pixel sind an, dunkle und transparente aus.
      // Bei mehreren Ebenen zählt die oberste Ebene, die an der Stelle nicht transparent ist.
      var layers = piskel.getLayers();
      var frames = layers.map(function (l) { return l.getFrameAt(fi); });
      var grid = [];
      for (var y = 0; y < H; y++) {
        var zeile = [];
        for (var x = 0; x < W; x++) {
          var an = false;
          for (var li = frames.length - 1; li >= 0; li--) {
            var p = pixelLesen(frames[li], x, y, W);
            if (istTransparent(p)) continue;
            an = istHell(p);
            break;
          }
          zeile.push(an);
        }
        grid.push(zeile);
      }

      console.log('[MegaCard] Pixel gelesen, sende Ergebnis');
      window.dispatchEvent(new CustomEvent('megacard-pixels-result', {
        detail: { grid: grid, width: W, height: H, name: spriteName(piskel) }
      }));

    } catch (e) {
      console.error('[MegaCard] Fehler:', e);
      window.dispatchEvent(new CustomEvent('megacard-pixels-result', {
        detail: { error: e.toString() }
      }));
    }
  });

  // ── Import: Bitmap in Piskel schreiben ────────────────────────────────────

  function antwort(daten) {
    window.dispatchEvent(new CustomEvent('megacard-import-result', { detail: JSON.stringify(daten) }));
  }

  // Ereignisse von Piskel auslösen: neu zeichnen und Rückgängig-Schritt anlegen
  function melden(namen) {
    var $ = window.$, ev = window.Events;
    if (!$ || !$.publish || !ev) return;
    namen.forEach(function (n) {
      if (n === 'PISKEL_SAVE_STATE') {
        $.publish(ev.PISKEL_SAVE_STATE, { type: 'SNAPSHOT' });
      } else if (ev[n]) {
        $.publish(ev[n]);
      }
    });
  }

  // Wie auf dem Display: schwarzer Hintergrund, gesetzte Pixel weiß
  function framesFuellen(frame, grid, w, h) {
    for (var y = 0; y < h; y++) {
      for (var x = 0; x < w; x++) {
        frame.setPixel(x, y, grid[y][x] ? FARBE_AN : FARBE_AUS);
      }
    }
  }

  window.addEventListener('megacard-import', function (e) {
    try {
      var d = JSON.parse(e.detail);
      var ctrl = controller();
      if (!ctrl) return antwort({ error: 'Controller nicht gefunden. Zuerst „Create Sprite“ wählen.' });

      var piskel = ctrl.getPiskel ? ctrl.getPiskel() : ctrl.piskel;
      var cw = piskel.getWidth(), ch = piskel.getHeight();

      if (cw === d.width && ch === d.height) {
        // Gleiche Größe: aktuellen Frame der aktuellen Ebene überschreiben
        var frame = ctrl.getCurrentFrame();
        framesFuellen(frame, d.grid, d.width, d.height);
        melden(['PISKEL_SAVE_STATE', 'PISKEL_RESET']);
        console.log('[MegaCard] Import in aktuellen Frame:', d.width + 'x' + d.height);
        return antwort({ ok: true, art: 'frame' });
      }

      if (!d.replace) {
        // Andere Größe: erst nachfragen, das ganze Sprite würde ersetzt
        return antwort({ needsReplace: true, cw: cw, ch: ch });
      }

      // Neues Sprite in der Größe der Bitmap anlegen
      var m = window.pskl.model;
      var neu = new m.Frame(d.width, d.height);
      framesFuellen(neu, d.grid, d.width, d.height);
      var layer = m.Layer.fromFrames('Layer 1', [neu]);
      var fps = piskel.getFPS ? piskel.getFPS() : 12;
      var descriptor = new m.piskel.Descriptor(d.name || 'MegaCard', '');
      var neuesSprite = m.Piskel.fromLayers([layer], fps, descriptor);

      ctrl.setPiskel(neuesSprite);
      // Nur der öffentliche Controller meldet selbst, sonst hier nachholen
      if (!ctrl.getWrappedPiskelController) {
        melden(['FRAME_SIZE_CHANGED', 'PISKEL_RESET', 'PISKEL_SAVE_STATE']);
      }
      console.log('[MegaCard] Import als neues Sprite:', d.width + 'x' + d.height);
      return antwort({ ok: true, art: 'neu' });

    } catch (err) {
      console.error('[MegaCard] Import-Fehler:', err);
      return antwort({ error: String(err) });
    }
  });

  console.log('[MegaCard] Event-Listener bereit');
})();
