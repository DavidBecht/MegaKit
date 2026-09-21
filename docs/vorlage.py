"""
vorlage.py -- Geruest, Aussehen und Verhalten der Doku-Seite.

Hier steht nur Layout. Was auf der Seite steht, kommt aus den Headern der
megalib (kopf_lesen.py) und aus inhalt.py.

Die Seite ist eine einzelne HTML-Datei ohne fremde Bibliotheken: Sie laesst
sich auch offline oeffnen, etwa aus einem Ordner auf dem Schulserver.
"""

# ---------------------------------------------------------------------------
# Geruest. Wird mit str.format gefuellt, geschweifte Klammern daher nur als
# Platzhalter verwenden.
# ---------------------------------------------------------------------------

SEITE = """<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>megalib &ndash; Funktionen der MEGACARD</title>
<meta name="description" content="Alle Funktionen der megalib fuer die MEGACARD V6.11 der HTL Rankweil, mit Beispielen und echten Display-Bildern aus dem Simulator.">
<link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'><rect width='16' height='16' rx='3' fill='%230d1a12'/><rect x='3' y='4' width='10' height='7' rx='1' fill='%234ade80'/></svg>">
<style>
{css}
</style>
</head>
<body>

<header class="kopf">
  <a class="marke" href="#start">
    <span class="marke-punkt"></span>
    <span class="marke-text">megalib</span>
    <span class="version">{version}</span>
  </a>
  <div class="suchfeld">
    <input id="suche" type="search" placeholder="Funktion suchen &hellip;  (Taste /)"
           autocomplete="off" spellcheck="false" aria-label="Funktion suchen">
  </div>
  <nav class="kopf-links">
    <a href="{repo}" target="_blank" rel="noopener">GitHub</a>
    <button id="thema" title="Hell oder dunkel" aria-label="Darstellung wechseln">&#9681;</button>
  </nav>
</header>

<nav class="reiter-leiste" role="tablist">
{knoepfe}
</nav>

<div class="rahmen">
  <aside class="seitenleiste">
{leisten}
  </aside>

  <main>
    <div id="suchergebnis" hidden>
      <h1>Suche</h1>
      <p class="untertitel" id="suche-zahl"></p>
      <div id="treffer"></div>
    </div>
{inhalte}
  </main>
</div>

<footer class="fuss">
  <p>Erzeugt aus den Headern der megalib. Die Bilder stammen aus dem Simulator
     megasim und zeigen genau das, was das daneben stehende Programm zeichnet.</p>
  <p><a href="{repo}">MegaKit auf GitHub</a> &middot;
     HTL Rankweil &middot; MEGACARD V6.11</p>
</footer>

<script>
const VERZEICHNIS = {verzeichnis};
{js}
</script>
</body>
</html>
"""

# ---------------------------------------------------------------------------
# Aussehen
# ---------------------------------------------------------------------------

CSS = r"""
:root {
  --bg: #f4f6f4;
  --flaeche: #ffffff;
  --flaeche-2: #f8faf8;
  --rand: #dde3de;
  --text: #16211b;
  --gedaempft: #5d6b62;
  --akzent: #0d7a45;
  --akzent-schwach: #e4f3ea;
  --code-bg: #f4f6f4;
  --schatten: 0 1px 2px rgba(20, 40, 30, .06);
  --k: #9333c4; --t: #1f6fb2; --s: #a35a00; --n: #b23b23; --p: #7a4fc4;
  --blau: #1f6fb2; --blau-bg: #e8f1fa;
  --gelb: #9a6700; --gelb-bg: #fdf4e3;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-thema="hell"]) {
    --bg: #0e1411; --flaeche: #151d18; --flaeche-2: #111815; --rand: #26312b;
    --text: #dde7e1; --gedaempft: #93a49a; --akzent: #4ade80;
    --akzent-schwach: #16281e; --code-bg: #0d1311;
    --schatten: 0 1px 2px rgba(0, 0, 0, .3);
    --k: #c792ea; --t: #79b8ff; --s: #e5c07b; --n: #ff9e64; --p: #c586c0;
    --blau: #79b8ff; --blau-bg: #142231;
    --gelb: #e5c07b; --gelb-bg: #2a2418;
  }
}
:root[data-thema="dunkel"] {
  --bg: #0e1411; --flaeche: #151d18; --flaeche-2: #111815; --rand: #26312b;
  --text: #dde7e1; --gedaempft: #93a49a; --akzent: #4ade80;
  --akzent-schwach: #16281e; --code-bg: #0d1311;
  --schatten: 0 1px 2px rgba(0, 0, 0, .3);
  --k: #c792ea; --t: #79b8ff; --s: #e5c07b; --n: #ff9e64; --p: #c586c0;
  --blau: #79b8ff; --blau-bg: #142231;
  --gelb: #e5c07b; --gelb-bg: #2a2418;
}

* { box-sizing: border-box; }
html { scroll-behavior: smooth; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font: 15px/1.6 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
  -webkit-text-size-adjust: 100%;
}
a { color: var(--akzent); }
code, pre, .mono { font-family: "Cascadia Mono", "JetBrains Mono", Consolas, "SF Mono", monospace; }

/* --- Kopf --- */
.kopf {
  position: sticky; top: 0; z-index: 30;
  display: flex; align-items: center; gap: 16px;
  padding: 10px 20px;
  background: color-mix(in srgb, var(--flaeche) 96%, transparent);
  backdrop-filter: blur(8px);
  border-bottom: 1px solid var(--rand);
}
.marke { display: flex; align-items: center; gap: 8px; text-decoration: none; color: var(--text); }
.marke-punkt { width: 12px; height: 12px; border-radius: 3px; background: var(--akzent); }
.marke-text { font-weight: 650; letter-spacing: -.01em; }
.version {
  font-size: 11px; color: var(--gedaempft); border: 1px solid var(--rand);
  border-radius: 99px; padding: 1px 8px;
}
.suchfeld { flex: 1; display: flex; justify-content: center; }
#suche {
  width: 100%; max-width: 420px; padding: 7px 12px;
  border: 1px solid var(--rand); border-radius: 8px;
  background: var(--bg); color: var(--text); font-size: 14px;
}
#suche:focus { outline: 2px solid var(--akzent); outline-offset: -1px; }
.kopf-links { display: flex; align-items: center; gap: 12px; font-size: 14px; }
.kopf-links a { text-decoration: none; color: var(--gedaempft); }
.kopf-links a:hover { color: var(--akzent); }
#thema {
  border: 1px solid var(--rand); background: var(--bg); color: var(--gedaempft);
  border-radius: 8px; width: 30px; height: 30px; cursor: pointer; font-size: 15px;
}

/* --- Reiter --- */
.reiter-leiste {
  position: sticky; top: 51px; z-index: 25;
  display: flex; gap: 2px; padding: 0 20px;
  background: var(--flaeche); border-bottom: 1px solid var(--rand);
  overflow-x: auto; scrollbar-width: none;
}
.reiter-leiste::-webkit-scrollbar { display: none; }
.reiter-knopf {
  border: 0; background: none; color: var(--gedaempft);
  padding: 11px 16px; font-size: 14.5px; font-weight: 550; cursor: pointer;
  border-bottom: 2px solid transparent; white-space: nowrap;
}
.reiter-knopf:hover { color: var(--text); }
.reiter-knopf.aktiv { color: var(--akzent); border-bottom-color: var(--akzent); }

/* --- Rahmen --- */
.rahmen {
  display: grid; grid-template-columns: 250px minmax(0, 1fr);
  gap: 36px; max-width: 1340px; margin: 0 auto; padding: 28px 20px 60px;
}
main { min-width: 0; }

/* --- Seitenleiste --- */
.seitenleiste { position: sticky; top: 104px; align-self: start; max-height: calc(100vh - 130px); overflow-y: auto; }
.leiste { display: none; flex-direction: column; gap: 1px; padding-bottom: 20px; }
.leiste.aktiv { display: flex; }
.leiste-abschnitt {
  margin-top: 14px; padding: 4px 0; font-size: 12px; font-weight: 700;
  text-transform: uppercase; letter-spacing: .05em;
  color: var(--gedaempft); text-decoration: none;
}
.leiste-abschnitt:first-child { margin-top: 0; }
.leiste-abschnitt:hover { color: var(--akzent); }
.leiste-eintrag {
  padding: 3px 10px; font-size: 13px; color: var(--gedaempft);
  text-decoration: none; border-left: 2px solid var(--rand);
  font-family: "Cascadia Mono", Consolas, monospace;
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
}
.leiste-eintrag:hover { color: var(--akzent); border-left-color: var(--akzent); background: var(--akzent-schwach); }

/* --- Inhalt --- */
.reiter-inhalt { display: none; }
.reiter-inhalt.aktiv { display: block; }
.reiter-kopf h1, #suchergebnis h1 { font-size: 30px; margin: 0 0 4px; letter-spacing: -.02em; }
.untertitel { margin: 0 0 22px; color: var(--gedaempft); font-size: 15px; }
.einleitung { margin-bottom: 30px; }
.einleitung h3 { font-size: 17px; margin: 26px 0 8px; }
.lead { font-size: 16px; }
.abschnitt { margin-bottom: 42px; scroll-margin-top: 104px; }
.abschnitt > h2 {
  font-size: 13px; text-transform: uppercase; letter-spacing: .06em;
  color: var(--akzent); margin: 0 0 4px; padding-top: 6px;
  border-top: 2px solid var(--akzent-schwach);
}
.abschnitt-text { margin: 0 0 18px; color: var(--gedaempft); max-width: 70ch; }

/* --- Eintrag --- */
.eintrag {
  background: var(--flaeche); border: 1px solid var(--rand); border-radius: 12px;
  padding: 16px 18px; margin-bottom: 14px; box-shadow: var(--schatten);
  scroll-margin-top: 104px;
}
.eintrag h3 { display: flex; align-items: center; gap: 10px; margin: 0 0 10px; font-size: 17px; }
.eintrag-name { font-family: "Cascadia Mono", Consolas, monospace; font-size: 16px; }
.art {
  font-size: 10.5px; font-weight: 700; text-transform: uppercase; letter-spacing: .05em;
  padding: 2px 7px; border-radius: 5px; background: var(--akzent-schwach); color: var(--akzent);
}
.art-makro { background: var(--gelb-bg); color: var(--gelb); }
.art-konstante { background: var(--gelb-bg); color: var(--gelb); }
.art-typ { background: var(--blau-bg); color: var(--blau); }
.anker { margin-left: auto; color: var(--rand); text-decoration: none; font-weight: 700; }
.eintrag:hover .anker { color: var(--gedaempft); }
.brief { margin: 0 0 10px; font-weight: 500; }
.text { color: var(--text); }
.text p, .einleitung p { margin: 0 0 10px; max-width: 74ch; }
.eintrag code, .einleitung code, .abschnitt-text code {
  background: var(--code-bg); border: 1px solid var(--rand); border-radius: 4px;
  padding: 0 4px; font-size: 12.8px;
}
a.quer { text-decoration: none; }
a.quer code { border-bottom: 1px solid var(--akzent); }

/* --- Code --- */
pre.code {
  background: var(--code-bg); border: 1px solid var(--rand); border-radius: 8px;
  padding: 11px 13px; overflow-x: auto; font-size: 12.9px; line-height: 1.55;
  margin: 0 0 12px; tab-size: 4;
}
pre.sig { border-left: 3px solid var(--akzent); font-size: 13.2px; }
pre.code .k { color: var(--k); }
pre.code .t { color: var(--t); }
pre.code .s { color: var(--s); }
pre.code .n { color: var(--n); }
pre.code .p { color: var(--p); }
pre.code .c { color: var(--gedaempft); font-style: italic; }
pre.code .l, pre.code a.l { color: var(--akzent); font-weight: 600; text-decoration: none; }
pre.code a.l:hover { text-decoration: underline; }

/* --- Tabellen --- */
.params { width: 100%; border-collapse: collapse; margin: 4px 0 12px; font-size: 14px; }
.params caption {
  text-align: left; font-size: 11.5px; font-weight: 700; text-transform: uppercase;
  letter-spacing: .05em; color: var(--gedaempft); padding-bottom: 4px;
}
.params td { padding: 5px 8px; border-top: 1px solid var(--rand); vertical-align: top; }
.params td:first-child { width: 30%; white-space: nowrap; }
.felder td:first-child { width: 42%; }
.params tr.gruppe td {
  font-size: 11.5px; text-transform: uppercase; letter-spacing: .05em;
  color: var(--akzent); background: var(--flaeche-2); font-weight: 700;
}
.vergleich table { width: 100%; border-collapse: collapse; margin: 6px 0 16px; font-size: 14.5px; }
.vergleich th, .vergleich td { padding: 8px 10px; border: 1px solid var(--rand); vertical-align: top; text-align: left; }
.vergleich th { background: var(--flaeche-2); }
.vergleich td:first-child { color: var(--gedaempft); width: 22%; }
.th-klein { font-weight: 400; font-size: 12px; color: var(--gedaempft); }
.merkliste { max-width: 74ch; padding-left: 20px; }
.merkliste li { margin-bottom: 7px; }

/* --- Hinweise --- */
.rueckgabe { margin: 0 0 10px; }
.etikett {
  display: inline-block; font-size: 11px; font-weight: 700; text-transform: uppercase;
  letter-spacing: .05em; color: var(--gedaempft); margin-right: 8px;
}
.hinweis {
  border-left: 3px solid var(--blau); background: var(--blau-bg);
  padding: 9px 12px; border-radius: 0 8px 8px 0; margin: 0 0 10px; font-size: 14px;
}
.hinweis.warnung { border-left-color: var(--gelb); background: var(--gelb-bg); }
.hinweis.tipp { border-left-color: var(--akzent); background: var(--akzent-schwach); }
.hinweis code { background: transparent; border: 0; padding: 0; }

/* --- Beispiel --- */
.beispiel {
  border: 1px solid var(--rand); border-radius: 10px; overflow: hidden;
  margin: 14px 0 6px; background: var(--flaeche-2);
}
.beispiel-kopf {
  padding: 7px 13px; font-size: 12px; font-weight: 700; text-transform: uppercase;
  letter-spacing: .05em; color: var(--akzent); background: var(--akzent-schwach);
  border-bottom: 1px solid var(--rand);
}
.beispiel-inhalt { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 14px; padding: 13px; align-items: start; }
.beispiel-inhalt pre.code { margin: 0; background: var(--flaeche); }
.schirm { margin: 0; display: flex; flex-direction: column; gap: 6px; align-items: center; }
.schirm img {
  display: block; width: 288px; max-width: 100%; image-rendering: pixelated;
  background: #050c05; border: 4px solid #1e50dc; border-radius: 5px;
}
.schirm figcaption { font-size: 11.5px; color: var(--gedaempft); }
.abschnitt > .beispiel { margin-bottom: 18px; }

/* --- Suche --- */
#treffer { display: flex; flex-direction: column; gap: 6px; }
.treffer {
  display: flex; align-items: center; gap: 10px; padding: 9px 12px;
  border: 1px solid var(--rand); border-radius: 9px; background: var(--flaeche);
  text-decoration: none; color: var(--text);
}
.treffer:hover { border-color: var(--akzent); }
.treffer .name { font-family: "Cascadia Mono", Consolas, monospace; font-weight: 600; }
.treffer .kurz { color: var(--gedaempft); font-size: 13.5px; flex: 1; min-width: 0;
  overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.treffer .wo { font-size: 11.5px; color: var(--gedaempft); border: 1px solid var(--rand); border-radius: 99px; padding: 1px 8px; }
.leer { color: var(--gedaempft); }
@keyframes blinzeln { from { background: var(--akzent-schwach); } to { background: var(--flaeche); } }
.eintrag.gefunden { animation: blinzeln 1.4s ease-out; }

.quelle { margin-top: 12px; font-size: 11.5px; }
.quelle a { color: var(--gedaempft); text-decoration: none;
  font-family: "Cascadia Mono", Consolas, monospace; }
.quelle a:hover { color: var(--akzent); }
.quelle .zeile { margin-left: 8px; }

/* --- Fuss --- */
.fuss {
  border-top: 1px solid var(--rand); padding: 22px 20px 40px; text-align: center;
  color: var(--gedaempft); font-size: 13px;
}
.fuss p { margin: 4px 0; }

/* --- Schmale Fenster --- */
@media (max-width: 950px) {
  .rahmen { grid-template-columns: minmax(0, 1fr); gap: 0; padding-top: 20px; }
  .seitenleiste { display: none; }
  .beispiel-inhalt { grid-template-columns: minmax(0, 1fr); }
  .schirm img { width: 100%; max-width: 320px; }
  .kopf { flex-wrap: wrap; gap: 10px; }
  .suchfeld { order: 3; flex-basis: 100%; }
  .reiter-leiste { top: 96px; }
  .abschnitt, .eintrag { scroll-margin-top: 150px; }
}

/* --- Drucken --- */
@media print {
  .kopf, .reiter-leiste, .seitenleiste, #suchergebnis, .anker { display: none !important; }
  .reiter-inhalt { display: block !important; page-break-before: always; }
  .rahmen { display: block; max-width: none; padding: 0; }
  .eintrag { break-inside: avoid; box-shadow: none; }
}
"""

# ---------------------------------------------------------------------------
# Verhalten
# ---------------------------------------------------------------------------

JS = r"""
const knoepfe = document.querySelectorAll('.reiter-knopf');
const inhalte = document.querySelectorAll('.reiter-inhalt');
const leisten = document.querySelectorAll('.leiste');
const suche   = document.getElementById('suche');
const ergebnis = document.getElementById('suchergebnis');

const REITER_NAME = {};
knoepfe.forEach(k => REITER_NAME[k.dataset.reiter] = k.textContent);

function reiterZeigen(id, merken) {
  let gefunden = false;
  knoepfe.forEach(k => { const an = k.dataset.reiter === id;
                         k.classList.toggle('aktiv', an); gefunden ||= an; });
  if (!gefunden) return false;
  inhalte.forEach(i => i.classList.toggle('aktiv', i.dataset.reiter === id));
  leisten.forEach(l => l.classList.toggle('aktiv', l.dataset.reiter === id));
  ergebnis.hidden = true;
  if (merken) { try { localStorage.setItem('megalib-reiter', id); } catch (e) {} }
  return true;
}

knoepfe.forEach(k => k.addEventListener('click', () => {
  reiterZeigen(k.dataset.reiter, true);
  window.scrollTo({ top: 0, behavior: 'smooth' });
  history.replaceState(null, '', '#' + k.dataset.reiter);
}));

/* Zu einem Eintrag springen, auch wenn er in einem anderen Reiter steht. */
function zeigen(name, sofort) {
  const ziel = document.getElementById(name);
  if (!ziel) return false;
  const reiter = ziel.closest('.reiter-inhalt');
  if (reiter) reiterZeigen(reiter.dataset.reiter, true);
  requestAnimationFrame(() => {
    // 'instant' statt 'auto': 'auto' wuerde das sanfte Scrollen aus dem
    // CSS uebernehmen, das beim Laden von den Bildern unterbrochen wird.
    ziel.scrollIntoView({ behavior: sofort ? 'instant' : 'smooth', block: 'start' });
    if (ziel.classList.contains('eintrag')) {
      ziel.classList.remove('gefunden');
      void ziel.offsetWidth;
      ziel.classList.add('gefunden');
    }
  });
  return true;
}

document.addEventListener('click', e => {
  const a = e.target.closest('a[href^="#"]');
  if (!a) return;
  const ziel = a.getAttribute('href').slice(1);
  if (!ziel) return;
  if (reiterZeigen(ziel, true)) {
    e.preventDefault();
    window.scrollTo({ top: 0, behavior: 'smooth' });
    history.replaceState(null, '', '#' + ziel);
    return;
  }
  if (zeigen(ziel)) { e.preventDefault(); history.replaceState(null, '', '#' + ziel); }
});

/* --- Suche --- */
const treffer = document.getElementById('treffer');
const zahl = document.getElementById('suche-zahl');

function suchen() {
  const wort = suche.value.trim().toLowerCase();
  if (!wort) {
    ergebnis.hidden = true;
    inhalte.forEach(i => i.classList.toggle('aktiv',
      i.dataset.reiter === (document.querySelector('.reiter-knopf.aktiv') || {}).dataset?.reiter));
    return;
  }
  const passt = VERZEICHNIS.filter(e =>
    e.n.toLowerCase().includes(wort) || (e.k || '').toLowerCase().includes(wort));
  passt.sort((a, b) => {
    const av = a.n.toLowerCase().startsWith(wort) ? 0 : 1;
    const bv = b.n.toLowerCase().startsWith(wort) ? 0 : 1;
    return av - bv || a.n.localeCompare(b.n);
  });
  treffer.innerHTML = passt.length ? passt.map(e =>
    '<a class="treffer" href="#' + e.n + '">' +
    '<span class="name">' + e.n + '</span>' +
    '<span class="kurz">' + (e.k || '') + '</span>' +
    '<span class="wo">' + (REITER_NAME[e.r] || '') + '</span></a>').join('')
    : '<p class="leer">Nichts gefunden. Die Suche schaut in Namen und Kurzbeschreibungen.</p>';
  zahl.textContent = passt.length + (passt.length === 1 ? ' Eintrag' : ' Eintraege')
                     + ' fuer „' + suche.value.trim() + '“';
  inhalte.forEach(i => i.classList.remove('aktiv'));
  ergebnis.hidden = false;
  window.scrollTo({ top: 0 });
}

suche.addEventListener('input', suchen);
suche.addEventListener('keydown', e => {
  if (e.key === 'Escape') { suche.value = ''; suchen(); suche.blur(); }
});
document.addEventListener('keydown', e => {
  if (e.key === '/' && document.activeElement !== suche) { e.preventDefault(); suche.focus(); }
});

/* --- Hell oder dunkel --- */
const thema = document.getElementById('thema');
try {
  const gemerkt = localStorage.getItem('megalib-thema');
  if (gemerkt) document.documentElement.dataset.thema = gemerkt;
} catch (e) {}
thema.addEventListener('click', () => {
  const dunkel = window.matchMedia('(prefers-color-scheme: dark)').matches;
  const jetzt = document.documentElement.dataset.thema || (dunkel ? 'dunkel' : 'hell');
  const neu = jetzt === 'dunkel' ? 'hell' : 'dunkel';
  document.documentElement.dataset.thema = neu;
  try { localStorage.setItem('megalib-thema', neu); } catch (e) {}
});

/* --- Startzustand --- */
(function () {
  const ziel = decodeURIComponent(location.hash.slice(1));
  if (ziel && reiterZeigen(ziel, false)) return;
  let gemerkt = null;
  try { gemerkt = localStorage.getItem('megalib-reiter'); } catch (e) {}
  reiterZeigen(gemerkt && document.querySelector('.leiste[data-reiter="' + gemerkt + '"]')
               ? gemerkt : 'start', false);
  if (ziel) zeigen(ziel, true);
})();
"""
