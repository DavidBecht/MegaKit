// Laeuft in der ISOLATED-Welt → UI (Buttons, Export- und Import-Fenster)
//
// Bitmap-Formate der megalib (siehe display_draw.h):
//   HORIZONTAL: ein Byte = 8 Pixel nebeneinander, höchstwertiges Bit links,
//               jede Zeile beginnt an einer Bytegrenze → ceil(w/8) * h Byte
//   VERTIKAL:   ein Byte = 8 Pixel übereinander, niederwertigstes Bit oben,
//               Bytes je 8er-Zeile (Page) von links nach rechts → w * ceil(h/8) Byte

(function () {
  'use strict';

  var NAME_STANDARD = 'Mein_Bild';
  var SPEICHER_NAME = 'megapiskel-name';
  var SPEICHER_FORMAT = 'megapiskel-format';
  var BTN_EXPORT = '⬇ MegaCard C Export';
  var BTN_IMPORT = '⬆ Import';

  // ── Speicher (pro Browser für piskelapp.com) ──────────────────────────────

  function lesen(schluessel) {
    try { return localStorage.getItem(schluessel) || ''; } catch (e) { return ''; }
  }
  function merken(schluessel, wert) {
    try { localStorage.setItem(schluessel, wert); } catch (e) { /* egal */ }
  }

  // ── C-Name ────────────────────────────────────────────────────────────────

  // Macht aus beliebigem Text einen gültigen C-Bezeichner:
  // Umlaute ersetzen, Sonderzeichen zu '_', darf nicht mit einer Ziffer beginnen
  function cName(text) {
    var n = String(text || '')
      .replace(/ä/g, 'ae').replace(/ö/g, 'oe').replace(/ü/g, 'ue')
      .replace(/Ä/g, 'Ae').replace(/Ö/g, 'Oe').replace(/Ü/g, 'Ue').replace(/ß/g, 'ss')
      .replace(/[^A-Za-z0-9_]+/g, '_')
      .replace(/^_+|_+$/g, '');
    if (/^[0-9]/.test(n)) n = '_' + n;
    return n;
  }

  // Vorschlag: Name des Sprites in Piskel, sonst der zuletzt verwendete, sonst Standard
  function startName(pixels) {
    var n = cName(pixels.name);
    if (n && n.toLowerCase() !== 'new_piskel') return n;
    return cName(lesen(SPEICHER_NAME)) || NAME_STANDARD;
  }

  // ── Umwandlung Pixel ↔ Bytes ──────────────────────────────────────────────

  function byteAnzahl(w, h, format) {
    return format === 'VERTIKAL' ? w * Math.ceil(h / 8) : Math.ceil(w / 8) * h;
  }

  function kodieren(grid, w, h, format) {
    var bytes = new Array(byteAnzahl(w, h, format)).fill(0);
    for (var y = 0; y < h; y++) {
      for (var x = 0; x < w; x++) {
        if (!grid[y][x]) continue;
        if (format === 'VERTIKAL') bytes[(y >> 3) * w + x] |= 1 << (y & 7);
        else bytes[y * Math.ceil(w / 8) + (x >> 3)] |= 0x80 >> (x & 7);
      }
    }
    return bytes;
  }

  function dekodieren(bytes, w, h, format) {
    var grid = [];
    for (var y = 0; y < h; y++) {
      var zeile = [];
      for (var x = 0; x < w; x++) {
        var an = (format === 'VERTIKAL')
          ? bytes[(y >> 3) * w + x] & (1 << (y & 7))
          : bytes[y * Math.ceil(w / 8) + (x >> 3)] & (0x80 >> (x & 7));
        zeile.push(!!an);
      }
      grid.push(zeile);
    }
    return grid;
  }

  // ── C-Code ────────────────────────────────────────────────────────────────

  function hexZeilen(bytes) {
    var zeilen = [];
    for (var i = 0; i < bytes.length; i += 16) {
      zeilen.push('\t' + bytes.slice(i, i + 16).map(function (b) {
        return '0x' + b.toString(16).padStart(2, '0');
      }).join(', '));
    }
    return zeilen.join(',\n');
  }

  function cCode(name, bytes, w, h, format) {
    return [
      '// \'' + name + '\', ' + w + 'x' + h + 'px',
      'static const uint8_t ' + name + '_daten[] PROGMEM = {',
      hexZeilen(bytes),
      '};',
      'BITMAP_' + format + '(' + name + ', ' + name + '_daten, ' + w + ', ' + h + ');'
    ].join('\n');
  }

  // Liest eingefügten C-Code oder reine Byte-Listen. Erkennt:
  //   BITMAP_VERTIKAL/HORIZONTAL(Name, Tabelle, w, h)  → Name, Format, Größe
  //   // 'Name', 16x16px  (MegaPiskel und image2cpp)    → Name, Größe
  // Gelesen wird nur die erste Tabelle in { }.
  function codeLesen(text) {
    var r = { name: '', width: 0, height: 0, format: '', bytes: [], fehler: '' };

    var makro = /BITMAP_(VERTIKAL|HORIZONTAL)\s*\(\s*([A-Za-z_]\w*)\s*,\s*[A-Za-z_]\w*\s*,\s*(\d+)\s*,\s*(\d+)\s*\)/.exec(text);
    if (makro) {
      r.format = makro[1];
      r.name = makro[2];
      r.width = +makro[3];
      r.height = +makro[4];
    }
    var kopf = /'([^']*)'\s*,\s*(\d+)\s*x\s*(\d+)\s*px/i.exec(text);
    if (kopf) {
      if (!r.name) r.name = cName(kopf[1]);
      if (!r.width) { r.width = +kopf[2]; r.height = +kopf[3]; }
    }

    var ohneKommentare = text.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/.*$/gm, '');
    var auf = ohneKommentare.indexOf('{');
    var daten;
    if (auf >= 0) {
      var zu = ohneKommentare.indexOf('}', auf);
      daten = ohneKommentare.slice(auf + 1, zu < 0 ? undefined : zu);
    } else {
      daten = ohneKommentare.replace(/BITMAP_\w+\s*\([^)]*\)/g, '');
    }

    var zahlen = daten.match(/0x[0-9a-fA-F]+|0b[01]+|\b\d+\b/g) || [];
    for (var i = 0; i < zahlen.length; i++) {
      var z = zahlen[i];
      var wert = /^0b/i.test(z) ? parseInt(z.slice(2), 2) : (/^0x/i.test(z) ? parseInt(z, 16) : parseInt(z, 10));
      if (wert > 255) {
        r.fehler = z + ' ist größer als ein Byte (255).';
        break;
      }
      r.bytes.push(wert);
    }
    return r;
  }

  // ── Bausteine für die Fenster ─────────────────────────────────────────────

  function element(tag, css, text) {
    var el = document.createElement(tag);
    if (css) el.style.cssText = css;
    if (text) el.textContent = text;
    return el;
  }

  function knopf(label, bg) {
    return element('button',
      'background:' + bg + ';color:#fff;border:none;border-radius:4px;' +
      'padding:7px 18px;cursor:pointer;font-size:13px;font-family:sans-serif;', label);
  }

  function eingabe(css) {
    var i = element('input',
      'background:#252526;color:#d4d4d4;border:1px solid #3c3c3c;border-radius:4px;' +
      'padding:5px 8px;font-family:monospace;font-size:13px;' + (css || ''));
    i.type = 'text';
    i.spellcheck = false;
    return i;
  }

  function zeile(label) {
    var z = element('label',
      'display:flex;align-items:center;gap:8px;margin-bottom:10px;font-family:sans-serif;font-size:13px;');
    if (label) z.append(element('span', 'min-width:120px;', label));
    return z;
  }

  // Zwei Optionsfelder HORIZONTAL / VERTIKAL, ruft beiAenderung(format) auf
  function formatWahl(start, beiAenderung) {
    var z = zeile('Format:');
    var gruppe = 'megapiskel-format-' + Math.random().toString(36).slice(2);
    var felder = {};
    [['HORIZONTAL', 'Horizontal'], ['VERTIKAL', 'Vertikal (schneller)']].forEach(function (f) {
      var l = element('label', 'display:flex;align-items:center;gap:4px;cursor:pointer;');
      var r = element('input');
      r.type = 'radio';
      r.name = gruppe;
      r.value = f[0];
      r.checked = (f[0] === start);
      r.addEventListener('change', function () { if (r.checked) beiAenderung(f[0]); });
      felder[f[0]] = r;
      l.append(r, document.createTextNode(f[1]));
      z.append(l);
    });
    z.setzen = function (format) { if (felder[format]) felder[format].checked = true; };
    return z;
  }

  function codeFeld() {
    var pre = element('pre',
      'background:#252526;padding:12px;border-radius:4px;overflow:auto;' +
      'max-height:340px;font-size:11.5px;white-space:pre;margin:0 0 12px;' +
      'border:1px solid #3c3c3c;color:#ce9178;cursor:text;outline:none;' +
      'user-select:text;-webkit-user-select:text;');
    pre.tabIndex = 0;
    return pre;
  }

  // Dunkler Hintergrund plus Kasten. Hält Maus und Tastatur von Piskel fern:
  // Piskel hört auf die ganze Seite (Zeichnen, Shortcuts, Strg+C für Frames)
  // und verhindert dabei die Textauswahl.
  function fenster(titelText, untertitel) {
    var alt = document.getElementById('megacard-modal');
    if (alt) alt.remove();

    var overlay = element('div',
      'position:fixed;inset:0;z-index:999999;background:rgba(0,0,0,.8);' +
      'display:flex;align-items:center;justify-content:center;');
    overlay.id = 'megacard-modal';

    var box = element('div',
      'background:#1e1e1e;color:#d4d4d4;border-radius:8px;padding:20px;max-height:92vh;overflow:auto;' +
      'max-width:740px;width:94%;box-shadow:0 8px 32px rgba(0,0,0,.6);font-family:monospace;' +
      'user-select:text;-webkit-user-select:text;');

    box.append(
      element('div', 'font-size:15px;font-weight:bold;color:#4fc1ff;margin-bottom:6px;', titelText),
      element('div', 'font-size:12px;color:#6a9955;margin-bottom:12px;font-family:sans-serif;', untertitel));

    // Klick auf den Hintergrund schließt, aber nur wenn er dort auch begonnen hat.
    // Sonst würde Markieren mit Loslassen außerhalb das Fenster schließen.
    var downAufHintergrund = false;
    overlay.addEventListener('mousedown', function (e) { downAufHintergrund = (e.target === overlay); });
    overlay.addEventListener('click', function (e) {
      if (downAufHintergrund && e.target === overlay) overlay.remove();
    });
    ['mousedown', 'mouseup', 'mousemove', 'click', 'dblclick', 'contextmenu', 'selectstart',
     'keydown', 'keyup', 'keypress', 'copy', 'cut', 'paste', 'wheel'].forEach(function (typ) {
      overlay.addEventListener(typ, function (e) { e.stopPropagation(); });
    });
    overlay.addEventListener('keydown', function (e) {
      if (e.key === 'Escape') overlay.remove();
    });

    overlay.append(box);
    document.body.append(overlay);
    return { overlay: overlay, box: box, schliessen: function () { overlay.remove(); } };
  }

  function kopieren(btn, text) {
    var label = btn.textContent;
    navigator.clipboard.writeText(text).then(function () {
      btn.textContent = '✓ Kopiert!';
      setTimeout(function () { btn.textContent = label; }, 1500);
    });
  }

  function knopfZeile() {
    return element('div', 'display:flex;gap:10px;justify-content:flex-end;flex-wrap:wrap;');
  }

  // ── Export-Fenster ────────────────────────────────────────────────────────

  function exportZeigen(pixels) {
    var f = fenster('MegaCard C Export  (' + pixels.width + '\xd7' + pixels.height + ' px)',
      'In die eigene .c-Datei kopieren, außerhalb jeder Funktion. Teile lassen sich markieren und mit Strg+C kopieren.');

    var format = lesen(SPEICHER_FORMAT) === 'VERTIKAL' ? 'VERTIKAL' : 'HORIZONTAL';

    var nameZeile = zeile('Name der Bitmap:');
    var nameInput = eingabe('flex:1;');
    nameInput.value = startName(pixels);
    var nameHint = element('span', 'font-size:12px;color:#d7ba7d;');
    nameZeile.append(nameInput, nameHint);

    var wahl = formatWahl(format, function (neu) { format = neu; merken(SPEICHER_FORMAT, neu); erneuern(); });
    var pre = codeFeld();
    var bytes = [];

    function erneuern() {
      var name = cName(nameInput.value) || NAME_STANDARD;
      nameHint.textContent = (name !== nameInput.value) ? '→ ' + name : '';
      bytes = kodieren(pixels.grid, pixels.width, pixels.height, format);
      pre.textContent = cCode(name, bytes, pixels.width, pixels.height, format);
      return name;
    }

    var alles = knopf('Kopieren', '#0e639c');
    var nurBytes = knopf('Nur Bytes kopieren', '#3a6e3a');
    var zu = knopf('Schließen', '#555');

    alles.onclick = function () { merken(SPEICHER_NAME, erneuern()); kopieren(alles, pre.textContent); };
    nurBytes.onclick = function () { erneuern(); kopieren(nurBytes, hexZeilen(bytes)); };
    zu.onclick = f.schliessen;

    nameInput.addEventListener('input', erneuern);
    // Enter im Namensfeld kopiert den ganzen Code
    nameInput.addEventListener('keydown', function (e) { if (e.key === 'Enter') alles.click(); });

    var knoepfe = knopfZeile();
    knoepfe.append(nurBytes, alles, zu);
    f.box.append(nameZeile, wahl, pre, knoepfe);
    erneuern();
    nameInput.focus();
    nameInput.select();
  }

  // ── Import-Fenster ────────────────────────────────────────────────────────

  var importAktuell = null;   // Rückmeldung von content-main.js an das offene Fenster

  function importZeigen() {
    var f = fenster('MegaCard C Import',
      'C-Code einer Bitmap oder nur die Bytes einfügen. Name, Größe und Format werden aus ' +
      'BITMAP_VERTIKAL/HORIZONTAL(…) oder dem Kommentar „// \'Name\', 16x16px“ erkannt.');

    var text = element('textarea',
      'width:100%;box-sizing:border-box;height:160px;background:#252526;color:#ce9178;' +
      'border:1px solid #3c3c3c;border-radius:4px;padding:8px;font-family:monospace;font-size:11.5px;' +
      'margin-bottom:10px;resize:vertical;');
    text.spellcheck = false;
    text.placeholder = 'static const uint8_t Bombe_daten[] PROGMEM = {\n\t0x3c, 0x7e, …\n};\nBITMAP_VERTIKAL(Bombe, Bombe_daten, 8, 8);';

    var format = lesen(SPEICHER_FORMAT) === 'VERTIKAL' ? 'VERTIKAL' : 'HORIZONTAL';
    var groesse = zeile('Größe (B × H):');
    var wInput = eingabe('width:60px;'), hInput = eingabe('width:60px;');
    groesse.append(wInput, document.createTextNode('×'), hInput, document.createTextNode('Pixel'));
    var wahl = formatWahl(format, function (neu) { format = neu; pruefen(); });

    var vorschau = element('canvas', 'image-rendering:pixelated;background:#000;border:1px solid #3c3c3c;' +
      'display:block;margin:0 0 8px;max-width:100%;');
    var status = element('div', 'font-family:sans-serif;font-size:12px;min-height:1.4em;margin-bottom:12px;');

    var los = knopf('Importieren', '#0e639c');
    var zu = knopf('Schließen', '#555');
    var knoepfe = knopfZeile();
    knoepfe.append(los, zu);
    zu.onclick = function () { importAktuell = null; f.schliessen(); };

    f.box.append(text, groesse, wahl, vorschau, status, knoepfe);

    var gelesen = null, grid = null, ersetzen = false, name = '';

    function meldung(farbe, t) { status.style.color = farbe; status.textContent = t; }

    function zeichnen(g, w, h) {
      var s = Math.max(1, Math.min(8, Math.floor(512 / w), Math.floor(256 / h)));
      vorschau.width = w * s;
      vorschau.height = h * s;
      var ctx = vorschau.getContext('2d');
      ctx.fillStyle = '#000';
      ctx.fillRect(0, 0, vorschau.width, vorschau.height);
      ctx.fillStyle = '#9cdcfe';
      for (var y = 0; y < h; y++)
        for (var x = 0; x < w; x++)
          if (g[y][x]) ctx.fillRect(x * s, y * s, s, s);
    }

    // Prüft Eingabe und Felder; true, wenn importiert werden kann
    function pruefen() {
      ersetzen = false;
      los.textContent = 'Importieren';
      grid = null;
      vorschau.style.display = 'none';
      var w = parseInt(wInput.value, 10), h = parseInt(hInput.value, 10);
      if (!gelesen || !gelesen.bytes.length) { meldung('#888', 'Noch keine Bytes erkannt.'); return false; }
      if (gelesen.fehler) { meldung('#f48771', gelesen.fehler); return false; }
      if (!(w > 0 && h > 0)) { meldung('#d7ba7d', gelesen.bytes.length + ' Byte erkannt. Breite und Höhe eintragen.'); return false; }
      var soll = byteAnzahl(w, h, format);
      if (gelesen.bytes.length !== soll) {
        meldung('#f48771', gelesen.bytes.length + ' Byte erkannt, ' + format.toLowerCase() + ' bei ' +
          w + '×' + h + ' sind ' + soll + ' Byte nötig. Größe oder Format prüfen.');
        return false;
      }
      grid = dekodieren(gelesen.bytes, w, h, format);
      vorschau.style.display = 'block';
      zeichnen(grid, w, h);
      var hinweis = (w > 128 || h > 64) ? ' Achtung: größer als das Display (128×64).' : '';
      meldung('#6a9955', '✓ ' + soll + ' Byte, ' + w + '×' + h + ' px, ' + format.toLowerCase() +
        (name ? ', Name ' + name : '') + '.' + hinweis);
      return true;
    }

    // Neuer Text: erkannte Werte in die Felder übernehmen
    text.addEventListener('input', function () {
      gelesen = codeLesen(text.value);
      name = gelesen.name;
      if (gelesen.width) { wInput.value = gelesen.width; hInput.value = gelesen.height; }
      if (gelesen.format) { format = gelesen.format; wahl.setzen(format); }
      pruefen();
    });
    wInput.addEventListener('input', pruefen);
    hInput.addEventListener('input', pruefen);

    los.onclick = function () {
      var nochmal = ersetzen;        // zweiter Klick nach der Rückfrage: Sprite ersetzen
      if (!pruefen()) return;
      importAktuell = {
        fertig: function () { importAktuell = null; f.schliessen(); },
        nachfragen: function (cw, ch) {
          ersetzen = true;
          los.textContent = 'Neues Sprite anlegen';
          meldung('#d7ba7d', 'Das Sprite in Piskel ist ' + cw + '×' + ch + ' px, die Bitmap ' +
            wInput.value + '×' + hInput.value + ' px. Importieren legt ein neues Sprite in dieser Größe an ' +
            'und ersetzt das aktuelle (Strg+Z macht es rückgängig).');
        },
        fehler: function (t) { meldung('#f48771', 'Fehler: ' + t); }
      };
      window.dispatchEvent(new CustomEvent('megacard-import', {
        detail: JSON.stringify({
          grid: grid, width: grid[0].length, height: grid.length,
          name: name, replace: nochmal
        })
      }));
    };

    pruefen();
    text.focus();
  }

  // ── Rückmeldungen von content-main.js ─────────────────────────────────────

  window.addEventListener('megacard-pixels-result', function (e) {
    var btn = document.getElementById('megacard-btn');
    if (btn) btn.textContent = BTN_EXPORT;

    var data = e.detail;
    if (!data) return;
    if (data.error) {
      alert('Fehler: ' + data.error);
      return;
    }
    exportZeigen(data);
  });

  window.addEventListener('megacard-import-result', function (e) {
    if (!importAktuell) return;
    var d;
    try { d = JSON.parse(e.detail); } catch (err) { return; }
    if (d.error) importAktuell.fehler(d.error);
    else if (d.needsReplace) importAktuell.nachfragen(d.cw, d.ch);
    else if (d.ok) importAktuell.fertig();
  });

  // ── Buttons oben rechts ───────────────────────────────────────────────────

  function insertButton() {
    if (document.getElementById('megacard-btn')) return;

    var leiste = element('div',
      'position:fixed;top:12px;right:12px;z-index:999999;display:flex;gap:8px;');
    var css =
      'color:#fff;border:none;border-radius:5px;padding:7px 13px;cursor:pointer;' +
      'font-size:13px;font-weight:bold;font-family:sans-serif;box-shadow:0 2px 8px rgba(0,0,0,.4);';

    var imp = element('button', 'background:#3a6e3a;' + css, BTN_IMPORT);
    imp.onclick = importZeigen;

    var btn = element('button', 'background:#0e639c;' + css, BTN_EXPORT);
    btn.id = 'megacard-btn';
    btn.onclick = function () {
      btn.textContent = '⏳ Lese...';
      window.dispatchEvent(new CustomEvent('megacard-read-pixels'));
      setTimeout(function () {
        if (btn.textContent === '⏳ Lese...') {
          btn.textContent = BTN_EXPORT;
          alert('Kein Response – bitte F12 Console zeigen.');
        }
      }, 3000);
    };

    leiste.append(imp, btn);
    document.body.appendChild(leiste);
  }

  insertButton();
  var iv = setInterval(function () {
    insertButton();
    if (document.getElementById('megacard-btn')) clearInterval(iv);
  }, 500);

})();
