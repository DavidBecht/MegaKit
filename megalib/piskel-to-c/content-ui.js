// Laeuft in der ISOLATED-Welt → UI (Button + Modal)

(function () {
  'use strict';

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
      hexLines.push('\t' + chunk.map(function (b) {
        return '0x' + b.toString(16).padStart(2, '0');
      }).join(', '));
    }
    return [
      '// \'' + name + '\', ' + width + 'x' + height + 'px',
      'static const uint8_t ' + name + '_daten[] PROGMEM = {',
      hexLines.join(',\n'),
      '};',
      'BITMAP_HORIZONTAL(' + name + ', ' + name + '_daten, ' + width + ', ' + height + ');',
    ].join('\n');
  }

  // ── Code-Generator ────────────────────────────────────────────────────────

  function toAllesBlock(grid, width, height) {
    var name = 'Mein_Auto';
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
      'max-width:740px;width:94%;box-shadow:0 8px 32px rgba(0,0,0,.6);font-family:monospace;';

    var title = document.createElement('div');
    title.textContent = '⬇ In mein_auto.c kopieren  (' + pixels.width + '\xd7' + pixels.height + 'px)';
    title.style.cssText = 'font-size:15px;font-weight:bold;color:#4fc1ff;margin-bottom:6px;';

    var sub = document.createElement('div');
    sub.textContent = 'Alles zwischen den ====-Linien in mein_auto.c ersetzen.';
    sub.style.cssText = 'font-size:12px;color:#6a9955;margin-bottom:12px;font-family:sans-serif;';

    var pre = document.createElement('pre');
    pre.style.cssText =
      'background:#252526;padding:12px;border-radius:4px;overflow:auto;' +
      'max-height:400px;font-size:11.5px;white-space:pre;margin:0 0 12px;' +
      'border:1px solid #3c3c3c;color:#ce9178;';
    pre.textContent = toAllesBlock(pixels.grid, pixels.width, pixels.height);

    var btnRow = document.createElement('div');
    btnRow.style.cssText = 'display:flex;gap:10px;justify-content:flex-end;';

    function mkBtn(label, bg) {
      var b = document.createElement('button');
      b.textContent = label;
      b.style.cssText =
        'background:' + bg + ';color:#fff;border:none;border-radius:4px;' +
        'padding:7px 18px;cursor:pointer;font-size:13px;font-family:sans-serif;';
      return b;
    }

    var copyBtn  = mkBtn('Kopieren', '#0e639c');
    var closeBtn = mkBtn('Schliessen', '#555');

    copyBtn.onclick = function () {
      navigator.clipboard.writeText(pre.textContent).then(function () {
        copyBtn.textContent = '✓ Kopiert!';
        setTimeout(function () { copyBtn.textContent = 'Kopieren'; }, 1500);
      });
    };
    closeBtn.onclick = function () { overlay.remove(); };
    overlay.onclick  = function (e) { if (e.target === overlay) overlay.remove(); };

    btnRow.append(copyBtn, closeBtn);
    box.append(title, sub, pre, btnRow);
    overlay.append(box);
    document.body.append(overlay);
  }

  // ── Ergebnis-Event empfangen (kommt von content-main.js via MAIN-Welt) ───

  window.addEventListener('megacard-pixels-result', function (e) {
    var btn = document.getElementById('megacard-btn');
    if (btn) btn.textContent = '⬇ MegaCard C';

    var data = e.detail;
    if (!data) return;
    if (data.error) {
      alert('Fehler: ' + data.error);
      return;
    }
    showModal(data);
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
      window.dispatchEvent(new CustomEvent('megacard-read-pixels'));
      setTimeout(function () {
        if (btn.textContent === '⏳ Lese...') {
          btn.textContent = '⬇ MegaCard C';
          alert('Kein Response – bitte F12 Console zeigen.');
        }
      }, 3000);
    };

    document.body.appendChild(btn);
  }

  insertButton();
  var iv = setInterval(function () {
    insertButton();
    if (document.getElementById('megacard-btn')) clearInterval(iv);
  }, 500);

})();
