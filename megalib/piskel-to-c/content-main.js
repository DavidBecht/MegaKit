// Laeuft in der MAIN-Welt (Seiten-Kontext) → hat Zugriff auf window.pskl

(function () {
  'use strict';

  console.log('[MegaCard] MAIN-Script geladen');

  window.addEventListener('megacard-read-pixels', function () {
    console.log('[MegaCard] Event empfangen');

    try {
      var ctrl =
        (window.pskl && window.pskl.app && window.pskl.app.piskelController) ||
        (window.pskl && window.pskl.controller) ||
        null;

      if (!ctrl) {
        window.dispatchEvent(new CustomEvent('megacard-pixels-result', {
          detail: { error: 'Controller nicht gefunden.' }
        }));
        return;
      }

      var piskel = ctrl.getPiskel ? ctrl.getPiskel() : ctrl.piskel;
      var W = piskel.getWidth();
      var H = piskel.getHeight();

      var fi = 0;
      if (typeof ctrl.currentFrameIndex === 'number') fi = ctrl.currentFrameIndex;
      else if (ctrl.getCurrentFrameIndex) fi = ctrl.getCurrentFrameIndex();

      console.log('[MegaCard] Sprite:', W + 'x' + H, 'Frame:', fi);

      var layers = piskel.getLayers();
      var grid = [];
      for (var y = 0; y < H; y++) grid.push(new Array(W).fill(false));

      for (var li = 0; li < layers.length; li++) {
        var frame = layers[li].getFrameAt(fi);

        for (var y = 0; y < H; y++) {
          for (var x = 0; x < W; x++) {
            if (grid[y][x]) continue;

            var pixel;

            // Piskel speichert als Uint32Array, Methode heisst getPixel (nicht getPixelAt)
            if (typeof frame.getPixel === 'function') {
              pixel = frame.getPixel(x, y);
            } else {
              // Fallback: flaches Array (row-major)
              pixel = frame.pixels[y * W + x];
            }

            // 0 = vollständig transparent
            if (!pixel) continue;

            // Alpha-Kanal prüfen: ARGB → oberstes Byte; RGBA → unterstes Byte
            var alpha = (pixel >>> 24) || (pixel & 0xFF);
            if (alpha === 0) continue;

            grid[y][x] = true;
          }
        }
      }

      console.log('[MegaCard] Pixel gelesen, sende Ergebnis');
      window.dispatchEvent(new CustomEvent('megacard-pixels-result', {
        detail: { grid: grid, width: W, height: H }
      }));

    } catch (e) {
      console.error('[MegaCard] Fehler:', e);
      window.dispatchEvent(new CustomEvent('megacard-pixels-result', {
        detail: { error: e.toString() }
      }));
    }
  });

  console.log('[MegaCard] Event-Listener bereit');
})();
