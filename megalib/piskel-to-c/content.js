// Piskel → MegaCard C  (content script)

(function () {
  'use strict';

  // ── Seiten-Script injizieren (hat Zugriff auf window.pskl) ────────────────

  function injectPageScript() {
    var script = document.createElement('script');
    script.id = 'megacard-page-script';
    script.textContent = `
(function () {
  console.log('[MegaCard] Seiten-Script geladen');

  window.addEventListener('megacard-read-pixels', function () {
    console.log('[MegaCard] Event empfangen, suche pskl...');
    console.log('[MegaCard] window.pskl =', typeof window.pskl);

    try {
      // Verschiedene API-Pfade ausprobieren
      var ctrl =
        (window.pskl && window.pskl.app && window.pskl.app.piskelController) ||
        (window.pskl && window.pskl.controller) ||
        (window.piskelController) ||
        null;

      console.log('[MegaCard] Controller gefunden:', !!ctrl);

      if (!ctrl) {
        // Alle pskl-Properties ausgeben zum Debuggen
        if (window.pskl) {
          console.log('[MegaCard] pskl keys:', Object.keys(window.pskl).join(', '));
          if (window.pskl.app) console.log('[MegaCard] pskl.app keys:', Object.keys(window.pskl.app).join(', '));
        }
        window.postMessage({ type: 'megacard-pixels', error: 'Controller nicht gefunden. Siehe Console (F12) fuer Details.' }, '*');
        return;
      }

      var piskel = ctrl.getPiskel ? ctrl.getPiskel() : ctrl.piskel;
      var W = piskel.getWidth();
      var H = piskel.getHeight();

      // Frame-Index bestimmen
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
            var c = frame.getPixelAt(x, y);
            if (!c || c === 'rgba(0,0,0,0)') continue;
            var isWhite = (c === '#ffffff' || c === 'rgb(255,255,255)' || c === 0xFFFFFFFF);
            if (!isWhite) grid[y][x] = true;
          }
        }
      }

      console.log('[MegaCard] Pixel gelesen, sende Daten...');
      window.postMessage({ type: 'megacard-pixels', grid: grid, width: W, height: H }, '*');

    } catch(e) {
      console.error('[MegaCard] Fehler:', e);
      window.postMessage({ type: 'megacard-pixels', error: e.toString() }, '*');
    }
  });

  console.log('[MegaCard] Event-Listener registriert');
})();
    `;
    (document.head || document.documentElement).appendChild(script);
    script.remove();
  }

  injectPageScript();

  // ── BITMAP_HORIZONTAL Konvertierung ───────────────────────────────────────

  function toBitmapHorizontal(grid, width, height, name) {
    var bytesPerRow = Math.ceil(width / 8);
    var bytes = [];
    for (var y = 0; y < height; y++) {
      for (var bx = 0; bx < bytesPerRow; bx++) {
        var byte = 0;
        for (var bit = 0; bit < 8; bit++) {
          var x = bx * 8 + bit;
          if (x < width && grid[y][x]) byte |= (1 << (7 - bit));
        }
        bytes.push(byte);
      }
    }
    var hexLines = [];
    for (var i = 0; i < bytes.length; i += 16) {
      var chunk = bytes.slice(i, i + 16);
      hexLines.push('\t' + chunk.map(function(b){ return '0x' + b.toString(16).padStart(2,'0'); }).join(', '));
    }
    return [
      '// \'' + name + '\', ' + width + 'x' + height + 'px',
      'static const uint8_t ' + name + '_daten[] PROGMEM = {',
      hexLines.join(',\n'),
      '};',
      'BITMAP_HORIZONTAL(' + name + ', ' + name + '_daten, ' + width + ', ' + height + ');',
    ].join('\n');
  }

  // ── Modal ─────────────────────────────────────────────────────────────────

  var lastPixels = null;

  function showModal(pixels) {
    lastPixels = pixels;
    document.getElementById('megacard-modal')?.remove();

    var overlay = document.createElement('div');
    overlay.id = 'megacard-modal';
    overlay.style.cssText =
      'position:fixed;inset:0;z-index:999999;background:rgba(0,0,0,.8);' +
      'display:flex;align-items:center;justify-content:center;';

    var box = document.createElement('div');
    box.style.cssText =
      'background:#1e1e1e;color:#d4d4d4;border-radius:8px;padding:20px;' +
      'max-width:720px;width:92%;box-shadow:0 8px 32px rgba(0,0,0,.6);font-family:monospace;';

    var title = document.createElement('div');
    title.textContent = 'MegaCard C-Code  (' + pixels.width + '×' + pixels.height + 'px)';
    title.style.cssText = 'font-size:15px;font-weight:bold;color:#4fc1ff;margin-bottom:10px;';

    var nameRow = document.createElement('div');
    nameRow.style.cssText = 'display:flex;align-items:center;gap:8px;margin-bottom:10px;';
    var lbl = document.createElement('span');
    lbl.textContent = 'Name:'; lbl.style.color = '#9cdcfe'; lbl.style.fontSize = '13px';
    var inp = document.createElement('input');
    inp.value = 'MeinSprite';
    inp.style.cssText =
      'background:#3c3c3c;color:#d4d4d4;border:1px solid #555;border-radius:4px;' +
      'padding:3px 8px;font-family:monospace;font-size:13px;flex:1;';
    nameRow.append(lbl, inp);

    var pre = document.createElement('pre');
    pre.style.cssText =
      'background:#252526;padding:12px;border-radius:4px;overflow:auto;' +
      'max-height:380px;font-size:11.5px;white-space:pre;margin:0 0 10px;' +
      'border:1px solid #3c3c3c;color:#ce9178;';
    pre.textContent = toBitmapHorizontal(pixels.grid, pixels.width, pixels.height, inp.value);

    inp.addEventListener('input', function () {
      pre.textContent = toBitmapHorizontal(lastPixels.grid, lastPixels.width, lastPixels.height, inp.value || 'Sprite');
    });

    var btnRow = document.createElement('div');
    btnRow.style.cssText = 'display:flex;gap:10px;justify-content:flex-end;';

    function mkBtn(label, bg) {
      var b = document.createElement('button');
      b.textContent = label;
      b.style.cssText = 'background:' + bg + ';color:#fff;border:none;border-radius:4px;' +
        'padding:7px 18px;cursor:pointer;font-size:13px;font-family:sans-serif;';
      return b;
    }

    var copyBtn  = mkBtn('Kopieren', '#0e639c');
    var closeBtn = mkBtn('Schliessen', '#555');

    copyBtn.onclick = function () {
      navigator.clipboard.writeText(pre.textContent).then(function () {
        copyBtn.textContent = '✓ Kopiert!';
        setTimeout(function(){ copyBtn.textContent = 'Kopieren'; }, 1500);
      });
    };
    closeBtn.onclick = function () { overlay.remove(); };
    overlay.onclick  = function (e) { if (e.target === overlay) overlay.remove(); };

    btnRow.append(copyBtn, closeBtn);
    box.append(title, nameRow, pre, btnRow);
    overlay.append(box);
    document.body.append(overlay);
  }

  // ── Nachrichten vom Seiten-Script empfangen ───────────────────────────────

  window.addEventListener('message', function (e) {
    if (!e.data || e.data.type !== 'megacard-pixels') return;
    var btn = document.getElementById('megacard-btn');
    if (btn) btn.textContent = '⬇ MegaCard C';

    if (e.data.error) {
      alert('Fehler: ' + e.data.error + '\n\nBitte F12 → Console oeffnen fuer Details.');
      return;
    }
    showModal(e.data);
  });

  // ── Button ────────────────────────────────────────────────────────────────

  function insertButton() {
    if (document.getElementById('megacard-btn')) return;

    var btn = document.createElement('button');
    btn.id = 'megacard-btn';
    btn.textContent = '⬇ MegaCard C';
    btn.style.cssText =
      'position:fixed;top:12px;right:12px;z-index:999999;' +
      'background:#0e639c;color:#fff;border:none;border-radius:5px;' +
      'padding:7px 13px;cursor:pointer;font-size:13px;font-weight:bold;' +
      'font-family:sans-serif;box-shadow:0 2px 8px rgba(0,0,0,.4);';

    btn.onclick = function () {
      btn.textContent = '⏳ Lese...';
      console.log('[MegaCard] Button geklickt, sende Event...');
      window.dispatchEvent(new CustomEvent('megacard-read-pixels'));
      // Timeout falls kein Response kommt
      setTimeout(function () {
        if (btn.textContent === '⏳ Lese...') {
          btn.textContent = '⬇ MegaCard C';
          alert('Kein Response erhalten.\nBitte F12 → Console oeffnen und Button nochmal klicken.');
        }
      }, 3000);
    };

    document.body.appendChild(btn);
    console.log('[MegaCard] Button eingefuegt');
  }

  insertButton();
  var iv = setInterval(function () {
    insertButton();
    if (document.getElementById('megacard-btn')) clearInterval(iv);
  }, 500);

})();
