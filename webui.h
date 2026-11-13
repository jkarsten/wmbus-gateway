// Single-page web UI (HTML + CSS + JS, fully inline so it works offline inside the ESP32 access point).
// The UI language is German. All dynamic strings are inserted as text (textContent / esc()), never as raw HTML.
#pragma once
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b1220"><meta name="apple-mobile-web-app-capable" content="yes">
<title>wMBus Zähler</title>
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Crect width='32' height='32' rx='8' fill='%232563eb'/%3E%3Cpath d='M7 18c3-6 6-6 9 0s6 6 9 0' fill='none' stroke='white' stroke-width='3' stroke-linecap='round'/%3E%3C/svg%3E">
<style>
:root{--bg:#f2f4f8;--card:#fff;--fg:#0f1728;--mut:#667085;--line:#e4e8f0;--soft:#f6f8fb;--acc:#2563eb;--acc2:#4f46e5;--water:#0ea5e9;--heat:#f97316;--ok:#12b76a;--warn:#f79009;--bad:#f04438;--sh:0 1px 2px rgba(16,24,40,.05),0 8px 24px -12px rgba(16,24,40,.12);--r:16px}
@media(prefers-color-scheme:dark){:root{--bg:#090e19;--card:#111827;--fg:#eaf0fb;--mut:#92a3bf;--line:#202c42;--soft:#0e1523;--acc:#5b8cff;--acc2:#8b7bff;--water:#38bdf8;--heat:#fb923c;--ok:#32d583;--warn:#fdb022;--bad:#f97066;--sh:0 1px 0 rgba(255,255,255,.03) inset,0 10px 30px -14px rgba(0,0,0,.6)}}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html{scroll-behavior:smooth}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Inter,Roboto,sans-serif;-webkit-font-smoothing:antialiased;padding-bottom:84px}
svg{display:block}
.wrap{max-width:1100px;margin:0 auto;padding:0 18px}
/* Kopf */
.top{position:sticky;top:0;z-index:20;background:color-mix(in srgb,var(--bg) 82%,transparent);backdrop-filter:saturate(1.6) blur(14px);-webkit-backdrop-filter:saturate(1.6) blur(14px);border-bottom:1px solid var(--line)}
.top .wrap{display:flex;align-items:center;gap:14px;height:62px}
.logo{display:flex;align-items:center;gap:11px;font-weight:700;letter-spacing:-.2px;font-size:17px}
.logo i{width:34px;height:34px;border-radius:10px;background:linear-gradient(135deg,var(--acc),var(--acc2));display:grid;place-items:center;box-shadow:0 6px 16px -6px var(--acc)}
.logo small{display:block;font-weight:500;font-size:11.5px;color:var(--mut);letter-spacing:0;margin-top:-2px}
.chips{display:flex;gap:6px;margin-left:auto;flex-wrap:wrap;justify-content:flex-end}
.chip{display:inline-flex;align-items:center;gap:6px;font-size:12px;font-weight:550;padding:4px 10px;border-radius:99px;background:var(--card);border:1px solid var(--line);color:var(--mut);white-space:nowrap}
.dot{width:7px;height:7px;border-radius:50%;background:var(--mut);flex:none}
.dot.ok{background:var(--ok);box-shadow:0 0 0 3px color-mix(in srgb,var(--ok) 22%,transparent)}
.dot.warn{background:var(--warn)}.dot.bad{background:var(--bad)}
.tabs{display:flex;gap:2px;margin-left:12px}
.tabs button{border:0;background:none;color:var(--mut);font:inherit;font-weight:600;padding:8px 13px;border-radius:10px;cursor:pointer;transition:.15s}
.tabs button:hover{color:var(--fg)}.tabs button.on{background:var(--card);color:var(--fg);box-shadow:var(--sh)}
/* Tab-Leiste mobil */
.tabbar{display:none;position:fixed;left:0;right:0;bottom:0;z-index:30;background:color-mix(in srgb,var(--card) 90%,transparent);backdrop-filter:blur(16px);-webkit-backdrop-filter:blur(16px);border-top:1px solid var(--line);padding:6px 10px calc(6px + env(safe-area-inset-bottom))}
.tabbar button{flex:1;border:0;background:none;color:var(--mut);font:inherit;font-size:11px;font-weight:600;display:flex;flex-direction:column;align-items:center;gap:3px;padding:6px 0;cursor:pointer;border-radius:12px}
.tabbar button.on{color:var(--acc)}
@media(max-width:760px){.tabs{display:none}.tabbar{display:flex}.chips .hm{display:none}.logo small{display:none}.chips{flex-wrap:nowrap;gap:4px}.chip{padding:3px 8px;font-size:11.5px}.top .wrap{gap:8px}.logo{font-size:16px}}
main{padding:22px 0 30px}
.view{display:none;animation:in .35s ease}.view.on{display:block}
@keyframes in{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:none}}
@media(prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
h2{font-size:13px;text-transform:uppercase;letter-spacing:.08em;color:var(--mut);font-weight:650;margin:26px 2px 10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--r);box-shadow:var(--sh);padding:20px}
/* Hinweise */
.alerts{display:grid;gap:10px;margin-bottom:18px}
.alert{display:flex;align-items:center;gap:12px;padding:12px 14px;border-radius:14px;background:color-mix(in srgb,var(--warn) 12%,var(--card));border:1px solid color-mix(in srgb,var(--warn) 35%,var(--line));font-size:14px}
.alert.bad{background:color-mix(in srgb,var(--bad) 11%,var(--card));border-color:color-mix(in srgb,var(--bad) 35%,var(--line))}
.alert b{font-weight:650}.alert a{margin-left:auto;color:var(--acc);font-weight:650;text-decoration:none;white-space:nowrap;cursor:pointer}
/* Zeitbereich */
.bar{display:flex;align-items:center;gap:10px;margin-bottom:14px;flex-wrap:wrap}
.seg{display:inline-flex;background:var(--card);border:1px solid var(--line);border-radius:11px;padding:3px}
.seg button{border:0;background:none;color:var(--mut);font:inherit;font-size:13px;font-weight:600;padding:5px 13px;border-radius:8px;cursor:pointer;transition:.15s}
.seg button.on{background:var(--acc);color:#fff;box-shadow:0 4px 12px -4px var(--acc)}
.bar .sp{flex:1}
/* Zaehlerkarte */
.meters{display:grid;gap:18px;grid-template-columns:1fr}
.meters.one{grid-template-columns:minmax(0,760px);justify-content:center}   /* a single meter: centered, not stuck to the left */
@media(min-width:940px){.meters{grid-template-columns:1fr 1fr}}
.mc{--c:var(--water);position:relative;overflow:hidden;padding:0}
.mc.heat{--c:var(--heat)}
.mc::before{content:"";position:absolute;inset:0 0 auto 0;height:170px;background:radial-gradient(120% 100% at 0% 0%,color-mix(in srgb,var(--c) 20%,transparent),transparent 70%);pointer-events:none}
.mh{position:relative;display:flex;align-items:center;gap:13px;padding:20px 20px 0}
.ic{width:44px;height:44px;border-radius:13px;display:grid;place-items:center;background:linear-gradient(145deg,var(--c),color-mix(in srgb,var(--c) 60%,#000));box-shadow:0 8px 18px -8px var(--c);flex:none}
.mh h3{margin:0;font-size:16px;font-weight:650;line-height:1.2}
.mh .id{font-size:12.5px;color:var(--mut);font-variant-numeric:tabular-nums}
.live{margin-left:auto;display:inline-flex;align-items:center;gap:6px;font-size:12px;font-weight:600;color:var(--mut)}
.val{position:relative;padding:14px 20px 4px;display:flex;align-items:baseline;gap:8px}
.val b{font-size:46px;font-weight:700;letter-spacing:-1.5px;line-height:1.05;font-variant-numeric:tabular-nums}
.val span{font-size:19px;color:var(--mut);font-weight:550}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(128px,1fr));gap:10px;padding:10px 20px 6px}
.tile{background:var(--soft);border:1px solid var(--line);border-radius:12px;padding:10px 12px}
.tile small{display:block;color:var(--mut);font-size:11.5px;font-weight:550}
.tile b{font-size:16.5px;font-weight:650;font-variant-numeric:tabular-nums}
.tile b em{font-style:normal;font-size:12px;color:var(--mut);font-weight:550}
.ch{padding:10px 20px 20px}
.chh{display:flex;align-items:center;gap:8px;margin:8px 0 4px}
.chh span{font-size:12.5px;font-weight:600;color:var(--mut)}
.chh .seg{margin-left:auto}
.chh .seg button{padding:3px 10px;font-size:12px}
canvas{width:100%;height:200px;display:block;touch-action:pan-y}
canvas.s{height:92px}
.sig{display:inline-flex;gap:2px;align-items:flex-end;height:13px;margin-right:2px}
.sig i{width:3px;background:var(--line);border-radius:1px}
.sig i.f{background:var(--c)}
.empty{height:200px;display:grid;place-items:center;color:var(--mut);font-size:13.5px;text-align:center;line-height:1.5}
#tip{position:fixed;z-index:50;pointer-events:none;background:var(--fg);color:var(--bg);padding:6px 10px;border-radius:9px;font-size:12.5px;font-weight:600;opacity:0;transition:opacity .12s;white-space:nowrap;box-shadow:0 8px 20px -6px rgba(0,0,0,.4)}
#tip small{display:block;font-weight:500;opacity:.7;font-size:11px}
/* Skeleton */
.sk{background:linear-gradient(90deg,var(--soft),var(--line),var(--soft));background-size:200% 100%;animation:sk 1.3s infinite;border-radius:12px;height:300px}
@keyframes sk{to{background-position:-200% 0}}
/* Einstellungen */
.set{display:grid;grid-template-columns:210px 1fr;gap:20px;align-items:start}
@media(max-width:760px){.set{grid-template-columns:1fr}}
.snav{display:flex;flex-direction:column;gap:4px;position:sticky;top:78px}
@media(max-width:760px){.snav{flex-direction:row;overflow-x:auto;position:static;padding-bottom:4px}}
.snav button{display:flex;align-items:center;gap:10px;border:0;background:none;color:var(--mut);font:inherit;font-weight:600;padding:10px 12px;border-radius:11px;cursor:pointer;text-align:left;white-space:nowrap}
.snav button.on{background:var(--card);color:var(--fg);box-shadow:var(--sh)}
.sec{display:none}.sec.on{display:block;animation:in .3s}
.sec h3{margin:0 0 4px;font-size:18px;letter-spacing:-.2px}
.sec .lead{color:var(--mut);margin:0 0 18px;font-size:14px}
.row{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:14px}
.row.two{grid-template-columns:repeat(2,minmax(0,1fr))}.row.three{grid-template-columns:repeat(3,minmax(0,1fr))}@media(max-width:640px){.row.three{grid-template-columns:1fr}}@media(max-width:520px){.row.two{grid-template-columns:1fr}}
label{display:block;font-size:12.5px;font-weight:600;color:var(--mut);margin-bottom:5px}
input,select{width:100%;padding:11px 12px;border:1px solid var(--line);border-radius:11px;background:var(--soft);color:var(--fg);font:inherit;transition:.15s;min-width:0}
input:focus,select:focus{outline:0;border-color:var(--acc);box-shadow:0 0 0 4px color-mix(in srgb,var(--acc) 18%,transparent);background:var(--card)}
.help{font-size:12px;color:var(--mut);margin-top:5px}
.btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;border:0;background:linear-gradient(135deg,var(--acc),var(--acc2));color:#fff;padding:11px 20px;border-radius:11px;font:inherit;font-weight:650;cursor:pointer;box-shadow:0 8px 18px -8px var(--acc);transition:.15s}
.btn:hover{transform:translateY(-1px)}.btn:active{transform:none}.btn[disabled]{opacity:.6;cursor:wait}
.btn.sec2{background:var(--soft);color:var(--fg);border:1px solid var(--line);box-shadow:none}
.btn.dng{background:var(--bad);box-shadow:0 8px 18px -8px var(--bad)}
.act{display:flex;gap:10px;margin-top:18px;flex-wrap:wrap;align-items:center}
/* System */
.kv{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:12px}
.kv div{background:var(--soft);border:1px solid var(--line);border-radius:12px;padding:11px 13px}
.kv small{display:block;color:var(--mut);font-size:12px;font-weight:550}.kv b{font-weight:650;font-variant-numeric:tabular-nums;word-break:break-all}
.q{height:9px;border-radius:9px;background:var(--line);overflow:hidden;display:flex;margin:10px 0 6px}
.q i{display:block;height:100%}
.lt{width:100%;border-collapse:collapse;font-size:13.5px;font-variant-numeric:tabular-nums}.lt th{text-align:left;color:var(--mut);font-weight:600;font-size:12px;padding:10px 8px 8px;border-bottom:1px solid var(--line);white-space:nowrap}.lt td{padding:9px 8px;border-bottom:1px solid var(--line);white-space:nowrap}.lt tr:last-child td{border-bottom:0}.lt .dot{margin-right:8px}.lw{overflow-x:auto}
.legend{display:flex;flex-wrap:wrap;gap:8px 20px;font-size:13px;color:var(--mut)}.lg{display:inline-flex;align-items:center;gap:7px}.lg i{width:10px;height:10px;border-radius:3px;flex:none}.lg b{color:var(--fg);font-weight:600}
.drop{border:2px dashed var(--line);border-radius:14px;padding:26px;text-align:center;color:var(--mut);cursor:pointer;transition:.15s}
.drop:hover,.drop.dz{border-color:var(--acc);color:var(--fg);background:color-mix(in srgb,var(--acc) 6%,transparent)}
.prog{height:8px;background:var(--line);border-radius:8px;overflow:hidden;margin-top:14px;display:none}.prog i{display:block;height:100%;width:0;background:linear-gradient(90deg,var(--acc),var(--acc2));transition:width .2s}
.foot{text-align:center;color:var(--mut);font-size:12.5px;margin-top:30px}
#toast{position:fixed;left:50%;bottom:92px;transform:translate(-50%,20px);background:var(--fg);color:var(--bg);padding:11px 18px;border-radius:12px;font-weight:600;font-size:14px;opacity:0;transition:.25s;z-index:60;max-width:90vw;text-align:center;box-shadow:0 12px 30px -10px rgba(0,0,0,.5)}
#toast.on{opacity:1;transform:translate(-50%,0)}#toast.err{background:var(--bad);color:#fff}
#pwm{position:fixed;inset:0;z-index:90;display:none;place-items:center;background:color-mix(in srgb,var(--bg) 88%,transparent);backdrop-filter:blur(10px);-webkit-backdrop-filter:blur(10px);padding:18px}#pwm.on{display:grid}#pwm .card{max-width:440px;width:100%}
#ov{position:fixed;inset:0;background:color-mix(in srgb,var(--bg) 85%,transparent);backdrop-filter:blur(8px);-webkit-backdrop-filter:blur(8px);z-index:80;display:none;place-items:center;text-align:center}
#ov.on{display:grid}#ov .sp{width:46px;height:46px;border-radius:50%;border:4px solid var(--line);border-top-color:var(--acc);animation:rot 1s linear infinite;margin:0 auto 16px}
@keyframes rot{to{transform:rotate(360deg)}}
</style></head><body>
<header class="top"><div class="wrap">
 <div class="logo"><i><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.4" stroke-linecap="round"><path d="M3 13c2.2-5 4.4-5 6.6 0s4.4 5 6.6 0 2.9-3 4.8-3"/><circle cx="12" cy="12" r="0.1"/></svg></i><div>wMBus<small>Zählerempfänger</small></div></div>
 <nav class="tabs"><button data-v="home" class="on">Übersicht</button><button data-v="settings">Einstellungen</button><button data-v="system">System</button></nav>
 <div class="chips" id="chips"></div>
</div></header>
<main><div class="wrap">
<section class="view on" id="v-home">
 <div class="alerts" id="alerts"></div>
 <div class="bar"><div class="seg" id="range"><button data-r="21600">6 Std</button><button data-r="86400" class="on">24 Std</button><button data-r="259200">3 Tage</button></div><div class="sp"></div><span class="chip" id="updated"></span></div>
 <div class="meters" id="meters"><div class="sk"></div><div class="sk"></div></div>
</section>
<section class="view" id="v-settings">
 <div class="set">
  <nav class="snav" id="snav">
   <button data-s="wifi" class="on"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.5a10 10 0 0 1 14 0M8.5 16a5 5 0 0 1 7 0M2 9a15 15 0 0 1 20 0"/><circle cx="12" cy="19.5" r=".6"/></svg>WLAN</button>
   <button data-s="mqtt"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 11l9-8 9 8v9a1 1 0 0 1-1 1h-5v-6H9v6H4a1 1 0 0 1-1-1z"/></svg>Home Assistant</button>
   <button data-s="meters"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3s6 6.2 6 10.5a6 6 0 0 1-12 0C6 9.2 12 3 12 3z"/></svg>Zähler</button>
   <button data-s="sec"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="11" width="16" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 8 0v4"/></svg>Sicherheit</button>
  </nav>
  <div>
   <form class="card sec on" id="s-wifi" autocomplete="off"><h3>WLAN</h3><p class="lead">Verbindet den Empfänger mit deinem Heimnetz (nur 2,4 GHz). Der eigene Access Point bleibt immer aktiv.</p>
    <div class="row"><div><label>Netzwerk</label><select id="scan"><option value="">Netzwerke suchen …</option></select></div><div><label>Oder Name eintippen</label><input name="ss" autocapitalize="none"></div><div><label>WLAN-Passwort</label><input name="sp" type="password" autocomplete="new-password"></div></div>
    <div class="act"><button class="btn" type="submit">Speichern &amp; verbinden</button><button class="btn sec2" type="button" id="bscan">Netzwerke suchen</button></div></form>
   <form class="card sec" id="s-mqtt" autocomplete="off"><h3>Home Assistant</h3><p class="lead">Per MQTT. Alle Sensoren werden automatisch in Home Assistant angelegt – ohne YAML.</p>
    <div class="row"><div><label>Broker (IP oder Name)</label><input name="mh" placeholder="z. B. 192.168.1.10" autocapitalize="none"></div><div><label>Port</label><input name="mp" type="number" min="1" max="65535"></div><div><label>Benutzer</label><input name="mu" autocapitalize="none"></div><div><label>Passwort</label><input name="mw" type="password" autocomplete="new-password"></div><div><label>Topic-Präfix</label><input name="mb" autocapitalize="none"></div></div>
    <div class="act"><button class="btn" type="submit">Speichern</button></div></form>
   <form class="card sec" id="s-meters" autocomplete="off"><h3>Zähler</h3><p class="lead">Bis zu zwei Zähler. Die Seriennummer steht auf dem Zähler (8 Ziffern). Ein leeres Feld schaltet den Platz ab – er erscheint dann weder hier noch in Home Assistant.</p>
    <h2 style="margin:4px 2px 10px">Zähler 1</h2><div class="row three"><div><label>Art</label><select name="k0"><option value="0">Wasserzähler</option><option value="1">Wärmezähler</option></select></div><div><label>Name</label><input name="n0" maxlength="32"></div><div><label>Seriennummer (leer = aus)</label><input name="i0" inputmode="numeric" maxlength="8" pattern="[0-9]{8}" placeholder="8 Ziffern"></div></div>
    <h2 style="margin:20px 2px 10px">Zähler 2</h2><div class="row three"><div><label>Art</label><select name="k1"><option value="0">Wasserzähler</option><option value="1">Wärmezähler</option></select></div><div><label>Name</label><input name="n1" maxlength="32"></div><div><label>Seriennummer (leer = aus)</label><input name="i1" inputmode="numeric" maxlength="8" pattern="[0-9]{8}" placeholder="8 Ziffern"></div></div>
    <div class="act"><button class="btn" type="submit">Speichern</button></div></form>
   <form class="card sec" id="s-sec" autocomplete="off"><h3>Sicherheit</h3><p class="lead">Login-Name dieser Seite: <b>admin</b>. Gilt im eigenen WLAN und im Heimnetz.</p>
    <div class="row"><div><label>Neues Web-Passwort (ab 8 Zeichen)</label><input name="wp" type="password" autocomplete="new-password"></div><div><label>Name des Access Points</label><input name="as"></div><div><label>Neues AP-Passwort (ab 8 Zeichen)</label><input name="ap" type="password" autocomplete="new-password"></div></div>
    <div class="act"><button class="btn" type="submit">Speichern</button></div></form>
  </div>
 </div>
</section>
<section class="view" id="v-system">
 <h2 style="margin-top:4px">Gerät</h2><div class="card"><div class="kv" id="k-dev"></div></div>
 <h2>Empfangsqualität</h2><div class="card"><div id="k-rx"></div></div>
 <h2>Letzte gültige Telegramme</h2><div class="card" style="padding:6px 18px 12px"><div id="k-log"><div class="help" style="padding:10px 0">Lade …</div></div></div>
 <h2>Firmware-Update</h2><div class="card"><p style="margin:0 0 14px;color:var(--mut);font-size:14px">Neue Firmware (<code>.bin</code>) hochladen – der Empfänger startet danach automatisch neu. Einstellungen und Verlauf bleiben erhalten.</p>
  <label class="drop" id="drop"><input type="file" id="fw" accept=".bin" hidden><b id="dropt">Datei auswählen oder hierher ziehen</b></label><div class="prog" id="prog"><i></i></div></div>
 <h2>Aktionen</h2><div class="card"><div class="act" style="margin:0"><button class="btn sec2" id="brs">Neu starten</button><button class="btn dng" id="bfac">Werkseinstellungen</button></div><div class="help">Werkseinstellungen löschen WLAN, MQTT, Passwörter und den gespeicherten Verlauf.</div></div>
</section>
<div class="foot" id="foot"></div>
</div></main>
<nav class="tabbar"><button data-v="home" class="on"><svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12h4l3 8 4-16 3 8h4"/></svg>Übersicht</button><button data-v="settings"><svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M4 7h10M18 7h2M4 17h2M10 17h10"/><circle cx="16" cy="7" r="2"/><circle cx="8" cy="17" r="2"/></svg>Einstellungen</button><button data-v="system"><svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="5" y="5" width="14" height="14" rx="2"/><path d="M9 2v3M15 2v3M9 19v3M15 19v3M2 9h3M2 15h3M19 9h3M19 15h3"/></svg>System</button></nav>
<div id="tip"></div><div id="toast"></div>
<div id="pwm"><form class="card" id="pwf" autocomplete="off"><h3 style="margin:0 0 6px;font-size:19px">Willkommen! Lege ein neues Passwort fest</h3>
<p style="color:var(--mut);margin:0 0 16px;font-size:14px">Das Anfangspasswort ist öffentlich bekannt. Das neue Passwort gilt für die Anmeldung auf dieser Seite (Benutzer <b>admin</b>) und für das WLAN des Geräts. Bis dahin sind Änderungen gesperrt.</p>
<label>Neues Passwort (8–63 Zeichen)</label><input type="password" name="wp" minlength="8" maxlength="63" required autocomplete="new-password">
<label style="margin-top:12px">Passwort wiederholen</label><input type="password" id="wp2" minlength="8" maxlength="63" required autocomplete="new-password">
<div id="pwerr" class="help" style="color:var(--bad);min-height:18px"></div><div class="act" style="margin-top:8px"><button class="btn" type="submit">Passwort speichern</button></div></form></div>
<div id="ov"><div><div class="sp"></div><b id="ovt">Der Empfänger startet neu …</b><div class="help" id="ovs">Das dauert etwa 10 Sekunden.</div></div></div>
<script>
const $=(s,r=document)=>r.querySelector(s),$$=(s,r=document)=>[...r.querySelectorAll(s)];
const api=async(u,o)=>{const r=await fetch(u,o);if(!r.ok)throw new Error(r.status);return r.json()};
const POSTH={'X-Requested-With':'wmbus'};   // required by the firmware for every POST (CSRF protection)
const css=n=>getComputedStyle(document.body).getPropertyValue(n).trim();
const nf=(v,d)=>v==null?'–':Number(v).toLocaleString('de-DE',{minimumFractionDigits:d,maximumFractionDigits:d});
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const dur=s=>{s=Math.round(s);if(s<90)return s+' s';if(s<5400)return Math.round(s/60)+' min';if(s<172800)return nf(s/3600,1)+' Std';return nf(s/86400,1)+' Tage'};
const ICON={water:'<svg width="24" height="24" viewBox="0 0 24 24" fill="#fff"><path d="M12 2.5s6.5 6.6 6.5 11.2A6.5 6.5 0 0 1 5.5 13.7C5.5 9.1 12 2.5 12 2.5z"/></svg>',heat:'<svg width="24" height="24" viewBox="0 0 24 24" fill="#fff"><path d="M12 2c1 4.2-3.4 5.600-3.400 10.200a3.400 3.400 0 0 0 6.800 0c0-1.200-.5-2.100-1.100-3.100 3.300 1.600 5.500 4.300 5.500 7.600a8 8 0 0 1-16 0C3.800 8.300 9.600 6.500 12 2z"/></svg>'};
let S=null,builtKey='',H=[null,null],range=86400,mode=[0,0],view='home',shown=[null,null],built=false,fetchedAt=0;
function slotKey(){return S.m.map(m=>m.en?m.id+(m.water?'w':'h'):'-').join()}   // changes when meters are added/removed/retyped
function toast(m,err){const t=$('#toast');t.textContent=m;t.className='on'+(err?' err':'');clearTimeout(toast.t);toast.t=setTimeout(()=>t.className='',3200)}
function sigBars(r){const n=r>-70?4:r>-80?3:r>-90?2:r>-100?1:0;return '<span class="sig">'+[5,8,11,14].map((h,i)=>`<i style="height:${h}px" class="${i<n?'f':''}"></i>`).join('')+'</span>'}
/* ---------- Navigation ---------- */
function go(v){view=v;$$('.view').forEach(e=>e.classList.toggle('on',e.id==='v-'+v));$$('[data-v]').forEach(b=>b.classList.toggle('on',b.dataset.v===v));window.scrollTo(0,0);if(v==='settings')loadCfg();if(v==='home')refresh(true);if(v==='system')refresh(false)}
$$('[data-v]').forEach(b=>b.onclick=()=>go(b.dataset.v));
$$('#snav button').forEach(b=>b.onclick=()=>sec(b.dataset.s));
function sec(s){$$('#snav button').forEach(b=>b.classList.toggle('on',b.dataset.s===s));$$('.sec').forEach(e=>e.classList.toggle('on',e.id==='s-'+s))}
$$('#range button').forEach(b=>b.onclick=()=>{range=+b.dataset.r;$$('#range button').forEach(x=>x.classList.toggle('on',x===b));drawAll()});
/* ---------- Diagramme ---------- */
function chart(cv,o){ // o: pts[{t,v}], color, dec, unit, bars, span
  const dpr=devicePixelRatio||1,w=cv.clientWidth,h=cv.clientHeight;cv.width=w*dpr;cv.height=h*dpr;
  const g=cv.getContext('2d');g.setTransform(dpr,0,0,dpr,0,0);g.clearRect(0,0,w,h);cv._o=o;
  const fg=css('--mut'),gr=css('--line'),pts=o.pts;g.font='11.5px -apple-system,Segoe UI,sans-serif';
  if(pts.length<2){g.fillStyle=fg;g.textAlign='center';g.fillText('Noch zu wenige Messpunkte – die Aufzeichnung läuft …',w/2,h/2);cv._p=null;return}
  let mn=Math.min(...pts.map(p=>p.v)),mx=Math.max(...pts.map(p=>p.v));
  if(o.bars)mn=0;else{if(mn===mx){mn-=1;mx+=1}const p=(mx-mn)*.15;mn-=p;mx+=p}
  if(!o.bars&&o.dec===0&&mx-mn<4){const c=(mx+mn)/2;mn=c-2;mx=c+2}   // ganzzahlige Achse: keine doppelten Beschriftungen
  if(mx===mn)mx=mn+1;if(o.bars)mx*=1.12;
  let t0=o.t0,t1=o.t1;if(o.bars&&o.ms){t0=Math.floor(t0/o.ms)*o.ms;t1=Math.ceil(t1/o.ms)*o.ms}
  const L=o.small?42:52,R=8,T=8,B=22,W=w-L-R,Hh=h-T-B;
  const X=t=>L+W*(t-t0)/(t1-t0),Y=v=>T+Hh*(1-(v-mn)/(mx-mn));
  g.strokeStyle=gr;g.fillStyle=fg;g.lineWidth=1;g.textAlign='right';
  const ticks=o.small?3:4;for(let i=0;i<=ticks;i++){const v=mn+(mx-mn)*i/ticks,y=Math.round(Y(v))+.5;g.beginPath();g.moveTo(L,y);g.lineTo(w-R,y);g.stroke();g.fillText(nf(v,o.dec),L-7,y+4)}
  g.textAlign='center';const nt=w<420?3:5,day=(t1-t0)>9e7;for(let i=0;i<=nt;i++){const t=t0+(t1-t0)*i/nt;g.textAlign=i===0?'left':i===nt?'right':'center';g.fillText(new Date(t).toLocaleString('de-DE',day?{day:'2-digit',month:'2-digit',hour:'2-digit',minute:'2-digit'}:{hour:'2-digit',minute:'2-digit'}),L+W*i/nt+(i===0?-0:0),h-5)}
  g.save();g.beginPath();g.rect(L,T,W,Hh+1);g.clip();
  if(o.bars){const bw=o.ms?Math.max(2,W*o.ms/(t1-t0)*.78):Math.max(2,Math.min(26,W/pts.length*.7));g.fillStyle=o.color;pts.forEach(p=>{const x=X(p.t),y=Y(p.v);g.globalAlpha=.9;g.beginPath();g.roundRect(x-bw/2,y,bw,Y(0)-y,[3,3,0,0]);g.fill()});g.globalAlpha=1}
  else{const gr2=g.createLinearGradient(0,T,0,T+Hh);gr2.addColorStop(0,o.color+'55');gr2.addColorStop(1,o.color+'00');
   g.beginPath();pts.forEach((p,i)=>i?g.lineTo(X(p.t),Y(p.v)):g.moveTo(X(p.t),Y(p.v)));
   g.strokeStyle=o.color;g.lineWidth=2.2;g.lineJoin='round';g.lineCap='round';g.stroke();
   g.lineTo(X(pts[pts.length-1].t),T+Hh);g.lineTo(X(pts[0].t),T+Hh);g.closePath();g.fillStyle=gr2;g.fill();
   const q=pts[pts.length-1];g.fillStyle=css('--card');g.strokeStyle=o.color;g.lineWidth=2.4;g.beginPath();g.arc(X(q.t),Y(q.v),4,0,7);g.fill();g.stroke()}
  g.restore();cv._p={X,Y,L,W,T,Hh};
  if(!cv._h){cv._h=1;const mv=e=>{const o=cv._o,P=cv._p;if(!P)return;const r=cv.getBoundingClientRect(),cx=(e.touches?e.touches[0].clientX:e.clientX)-r.left;let b=0,bd=1e9;o.pts.forEach((p,i)=>{const d=Math.abs(P.X(p.t)-cx);if(d<bd){bd=d;b=i}});
    const p=o.pts[b],tip=$('#tip');tip.innerHTML=`${nf(p.v,o.dec)} ${o.unit}<small>${new Date(p.t).toLocaleString('de-DE',{weekday:'short',day:'2-digit',month:'2-digit',hour:'2-digit',minute:'2-digit'})}${o.bars?' · '+o.step:''}</small>`;
    tip.style.opacity=1;const tw=tip.offsetWidth;tip.style.left=Math.min(innerWidth-tw-8,Math.max(8,(e.touches?e.touches[0].clientX:e.clientX)-tw/2))+'px';tip.style.top=(r.top+P.Y(p.v)-52)+'px'};
   cv.addEventListener('mousemove',mv);cv.addEventListener('touchmove',mv,{passive:true});cv.addEventListener('touchstart',mv,{passive:true});
   ['mouseleave','touchend'].forEach(ev=>cv.addEventListener(ev,()=>$('#tip').style.opacity=0))}
}
function series(i){ // Rohdaten -> Punkte im gewaehlten Bereich
  const h=H[i];if(!h)return[];const out=[];for(let k=0;k<h.t.length;k++)if(h.t[k]<=range)out.push({t:fetchedAt-h.t[k]*1000,v:h.v[k],r:h.r[k]});return out}
function usage(pts,dec){ // Verbrauch je Zeitfenster
  if(pts.length<2)return{ms:0,b:[],step:''};const step=range<=21600?900e3:range<=86400?3600e3:10800e3,m=new Map();
  for(let k=1;k<pts.length;k++){const d=Math.max(0,pts[k].v-pts[k-1].v),key=Math.floor(pts[k].t/step)*step;m.set(key,(m.get(key)||0)+d)}
  return{ms:step,b:[...m].map(([t,v])=>({t:t+step/2,v:+v.toFixed(dec)})),step:step===900e3?'je 15 min':step===3600e3?'je Stunde':'je 3 Std'}}
function drawMeter(i){
  const m=S.m[i];if(!m.en)return;
  const pts=series(i),c=css(m.water?'--water':'--heat'),dec=m.water?3:0,t1=fetchedAt,t0=t1-range*1000,cv=$('#a'+i),cs=$('#b'+i);
  if(!cv)return;
  if(mode[i]===0)chart(cv,{pts,color:c,dec,unit:m.unit,t0,t1});
  else{const u=usage(pts,m.water?3:0);chart(cv,{pts:u.b,color:c,dec:m.water?2:0,unit:m.unit,bars:true,step:u.step,ms:u.ms,t0,t1})}
  chart(cs,{pts:pts.map(p=>({t:p.t,v:p.r})),color:css('--mut'),dec:0,unit:'dBm',t0,t1,small:true})}
function drawAll(){if(S)S.m.forEach((_,i)=>drawMeter(i))}
/* ---------- Uebersicht ---------- */
function build(){
  const cards=S.m.map((m,i)=>!m.en?'':`<article class="card mc ${m.water?'':'heat'}" id="m${i}" data-slot="${i}">
   <div class="mh"><div class="ic">${ICON[m.water?'water':'heat']}</div><div><h3 data-f="name"></h3><div class="id" data-f="id"></div></div><div class="live" data-f="live"></div></div>
   <div class="val"><b data-f="total">–</b><span data-f="unit"></span></div>
   <div class="tiles" data-f="tiles"></div>
   <div class="ch"><div class="chh"><span data-f="ct">Zählerstand</span><div class="seg"><button class="on" data-md="0">Stand</button><button data-md="1">Verbrauch</button></div></div><canvas id="a${i}"></canvas>
   <div class="chh" style="margin-top:14px"><span>Signalstärke</span><a href="/api/csv?m=${i}" style="margin-left:auto;font-size:12.5px;font-weight:600;color:var(--acc);text-decoration:none">CSV-Export</a></div><canvas class="s" id="b${i}"></canvas></div></article>`).join('');
  $('#meters').classList.toggle('one',S.m.filter(m=>m.en).length===1);
  $('#meters').innerHTML=cards||`<div class="card" style="grid-column:1/-1;text-align:center;padding:38px 20px"><h3 style="margin:0 0 6px">Noch kein Zähler eingerichtet</h3><p style="color:var(--mut);margin:0 0 18px">Trage die Seriennummer deines Zählers ein, dann erscheinen hier die Werte.</p><button class="btn" onclick="go('settings');sec('meters')">Zähler einrichten</button></div>`;
  $$('.mc').forEach(el=>{const i=+el.dataset.slot;$$('[data-md]',el).forEach(b=>b.onclick=()=>{mode[i]=+b.dataset.md;$$('[data-md]',el).forEach(x=>x.classList.toggle('on',x===b));drawMeter(i)})});built=true;builtKey=slotKey()}
function countUp(el,from,to,dec){const t0=performance.now(),d=600;if(from==null||from===to||matchMedia('(prefers-reduced-motion:reduce)').matches){el.textContent=nf(to,dec);return}
  (function f(t){const k=Math.min(1,(t-t0)/d),e=1-Math.pow(1-k,3);el.textContent=nf(from+(to-from)*e,dec);if(k<1)requestAnimationFrame(f)})(t0)}
function update(){
  S.m.forEach((m,i)=>{if(!m.en)return;const el=$('#m'+i),dec=m.water?3:0,f=k=>$(`[data-f=${k}]`,el);
   f('name').textContent=m.name;f('id').textContent='Nr. '+m.id;f('unit').textContent=m.unit;
   const fresh=m.have&&m.age<1800,st=!m.have?'Wartet auf Telegramm':(fresh?'Live · vor '+dur(m.age):'Zuletzt vor '+dur(m.age));
   f('live').innerHTML=`${m.have?sigBars(m.rssi):''}<span class="dot ${!m.have?'':fresh?'ok':'warn'}"></span>${st}`;
   if(m.total!=null&&shown[i]!==m.total){countUp(f('total'),shown[i],m.total,dec);shown[i]=m.total}
   const tile=(a,b)=>`<div class="tile"><small>${a}</small><b>${b}</b></div>`,u=` <em>${m.unit}</em>`;
   f('tiles').innerHTML=tile('Seit '+(m.yd||'Jahreswert'),m.yv==null?'–':nf(m.total-m.yv,dec)+u)+tile('Seit '+(m.md||'Monatswert'),m.mv==null?'–':nf(m.total-m.mv,dec)+u)+tile('Stichtag '+(m.yd||'Jahr'),m.yv==null?'–':nf(m.yv,dec)+u)+tile('Signal',m.have?m.rssi+' <em>dBm</em>':'–')});
  const ch=[];const dot=(c,t)=>`<span class="chip"><span class="dot ${c}"></span>${t}</span>`;
  ch.push(dot(S.sta?'ok':'','WLAN'));ch.push(dot(S.mqtt?'ok':S.mqttOn?'bad':'','MQTT<span class="hm"> '+(S.mqtt?'verbunden':S.mqttOn?'getrennt':'aus')+'</span>'));ch.push(dot(S.radio?'ok':'bad','Funk'));
  $('#chips').innerHTML=ch.join('');$('#pwm').classList.toggle('on',!!S.mustChange);
  const al=[],a=(c,t,l,go)=>al.push(`<div class="alert ${c}"><span class="dot ${c||'warn'}"></span><div>${t}</div>${l?`<a onclick="${go}">${l}</a>`:''}</div>`);
  if(S.setup)a('','<b>Es ist noch kein Zähler eingerichtet.</b> Ohne Seriennummer kann der Empfänger nichts zuordnen.','Einrichten',"go('settings');sec('meters')");
  if(!S.radio)a('bad','<b>Funkchip antwortet nicht.</b> Verdrahtung des CC1101 prüfen.','', '');
  if(S.apDefault&&!S.mustChange)a('','<b>Das WLAN des Geräts nutzt noch das Anfangspasswort.</b> Lege unter Sicherheit ein neues AP-Passwort fest.','Jetzt ändern',"go('settings');sec('sec')");
  if(!S.sta)a('','<b>Nicht mit dem Heimnetz verbunden.</b> Für Home Assistant wird das WLAN benötigt.','Einrichten',"go('settings');sec('wifi')");
  else if(!S.mqttOn)a('','<b>Home Assistant ist noch nicht verbunden.</b> Trage den MQTT-Broker ein.','Einrichten',"go('settings');sec('mqtt')");
  else if(!S.timeOk)a('','<b>Uhrzeit nicht verfügbar.</b> Ohne Internet wird der Verlauf nicht dauerhaft gespeichert.','','');
  $('#alerts').innerHTML=al.join('');
  $('#updated').textContent='Aktualisiert '+new Date().toLocaleTimeString('de-DE',{hour:'2-digit',minute:'2-digit',second:'2-digit'});
  $('#foot').textContent='wMBus Zählerempfänger · Firmware '+S.fw+' · wmbus.local'}
async function refresh(full){
  try{S=await api('/api/state')}catch(e){$('#chips').innerHTML='<span class="chip"><span class="dot bad"></span>Keine Verbindung</span>';return}
  if(!built||builtKey!==slotKey())build();update();renderSys();if(view==='system')loadLog();
  if(view==='home'&&(full||!refresh.h||Date.now()-refresh.h>30000)){refresh.h=Date.now();for(let i=0;i<S.m.length;i++){if(!S.m[i].en)continue;try{H[i]=await api('/api/hist?m='+i)}catch(e){}}fetchedAt=Date.now();drawAll()}}
/* ---------- System ---------- */
function renderSys(){if(!S)return;const k=(a,b)=>`<div><small>${a}</small><b>${b}</b></div>`;
  $('#k-dev').innerHTML=k('Firmware',S.fw+' <span style="font-weight:500;color:var(--mut);font-size:12.5px">(gebaut '+esc(S.build||'')+')</span>')+k('Laufzeit',dur(S.up))+k('Freier Speicher',nf(S.heap/1024,0)+' KB')+k('WLAN',S.sta?esc(S.ip)+' · '+S.wrssi+' dBm':'nicht verbunden')+k('Adresse','wmbus.local')+k('Uhrzeit (NTP)',S.timeOk?'synchron':'nicht verfügbar');
  const ok=S.ok,cr=S.crc,fo=S.foreign,tt=Math.max(1,ok+cr+fo);
  const pc=v=>tt?Math.round(v/tt*100)+' %':'–',lg=(c,t,v)=>`<span class="lg"><i style="background:${c}"></i><b>${t}</b> ${v} (${pc(v)})</span>`;
  $('#k-rx').innerHTML=`<div class="q"><i style="width:${ok/tt*100}%;background:var(--ok)"></i><i style="width:${fo/tt*100}%;background:var(--mut)"></i><i style="width:${cr/tt*100}%;background:var(--bad)"></i></div>
   <div class="legend">${lg('var(--ok)','Deine Zähler',ok)}${lg('var(--mut)','Fremde Zähler',fo)}${lg('var(--bad)','Verworfen (Prüfsummenfehler)',cr)}</div>
   <div class="kv" style="margin-top:14px">${k('Funkchip',S.radio?'in Ordnung':'Fehler')+k('Funk-Neustarts',S.resets)+k('Empfangene Telegramme gesamt',S.frames)}</div>
   <div class="help">Der Balken zeigt alle empfangenen Funktelegramme. Verworfene Telegramme sind meist zu schwach empfangene Telegramme aus der Nachbarschaft und kein Zeichen für ein Problem. Wichtig ist, dass deine Zähler regelmäßig ankommen (siehe „Live" auf der Übersicht).</div>`}
async function loadLog(){let l;try{l=await api('/api/log')}catch(e){return}
  const now=Date.now(),ty={120:'Qundis-Block',114:'Standard (OMS)',122:'Standard kurz'};
  $('#k-log').innerHTML=l.length?`<div class="lw"><table class="lt"><thead><tr><th>Uhrzeit</th><th>Zähler</th><th>Telegrammart</th><th>Stand</th><th>Signal</th></tr></thead><tbody>`+l.map(e=>{const m=S.m[e.m],dec=m.water?3:0;return `<tr><td>${new Date(now-e.a*1000).toLocaleTimeString('de-DE')}</td><td><span class="dot" style="background:${e.m?'var(--heat)':'var(--water)'}"></span>${esc(m.name)}</td><td>${ty[e.c]||'0x'+e.c.toString(16)}</td><td>${nf(e.v,dec)} ${m.unit}</td><td>${e.r} dBm</td></tr>`}).join('')+'</tbody></table></div>':'<div class="help" style="padding:10px 0">Seit dem Start wurde noch kein gültiges Telegramm empfangen.</div>'}
/* ---------- Einstellungen ---------- */
async function loadCfg(){let c;try{c=await api('/api/config')}catch(e){return}
  $$('.sec').forEach(f=>[...f.elements].forEach(el=>{if(el.name&&c[el.name]!==undefined&&el.type!=='password')el.value=c[el.name];if(el.type==='password')el.placeholder=(el.name==='sp'&&c.hasSp)||(el.name==='mw'&&c.hasMw)?'gespeichert (leer = behalten)':'unverändert'}))}
function overlay(t,s){$('#ovt').textContent=t;$('#ovs').textContent=s;$('#ov').classList.add('on')}
function waitBack(){let n=0;const t=setInterval(async()=>{n++;try{await api('/api/state');clearInterval(t);location.reload()}catch(e){if(e.message==='401'){clearInterval(t);location.reload();return}if(n>40){clearInterval(t);$('#ovt').textContent='Keine Verbindung';$('#ovs').textContent='Verbinde dich ggf. neu mit dem WLAN des Empfängers und lade die Seite neu.'}}},1500)}
$$('form.sec').forEach(f=>f.onsubmit=async e=>{e.preventDefault();const b=$('.btn[type=submit]',f);b.disabled=true;
  try{const r=await fetch('/api/config',{method:'POST',headers:POSTH,body:new URLSearchParams(new FormData(f))});const t=await r.json();
   if(t.ok){overlay('Gespeichert – der Empfänger startet neu …','Das dauert etwa 10 Sekunden.');setTimeout(waitBack,4000)}else{toast(t.err||'Fehler beim Speichern',1);b.disabled=false}}
  catch(x){toast('Keine Verbindung zum Empfänger',1);b.disabled=false}});
$('#bscan').onclick=async()=>{const b=$('#bscan');b.disabled=true;b.textContent='Suche läuft …';
  try{const l=await api('/api/scan');$('#scan').innerHTML='<option value="">'+(l.length?'Netzwerk auswählen …':'Keine Netzwerke gefunden')+'</option>'+l.sort((a,b)=>b.r-a.r).map(n=>`<option value="${esc(n.s)}">${esc(n.s)} (${n.r} dBm)</option>`).join('')}catch(e){toast('Suche fehlgeschlagen',1)}
  b.disabled=false;b.textContent='Netzwerke suchen'};
$('#scan').onchange=e=>{if(e.target.value)$('#s-wifi').elements.ss.value=e.target.value};
$('#brs').onclick=async()=>{if(!confirm('Empfänger jetzt neu starten?'))return;try{await fetch('/api/restart',{method:'POST',headers:POSTH})}catch(e){}overlay('Der Empfänger startet neu …','Das dauert etwa 10 Sekunden.');setTimeout(waitBack,3000)};
$('#bfac').onclick=async()=>{if(!confirm('Wirklich alle Einstellungen und den Verlauf löschen?'))return;try{await fetch('/api/factory',{method:'POST',headers:POSTH})}catch(e){}overlay('Werkseinstellungen werden hergestellt …','Danach gilt wieder das Anfangspasswort wmbus1234, und du legst beim ersten Login ein neues fest.')};
/* ---------- Firmware-Update ---------- */
function upload(file){if(!file)return;if(!/\.bin$/i.test(file.name)){toast('Bitte eine .bin-Datei wählen',1);return}
  const x=new XMLHttpRequest(),fd=new FormData();fd.append('fw',file,file.name);const p=$('#prog');p.style.display='block';$('#dropt').textContent='Lade '+file.name+' hoch …';
  x.upload.onprogress=e=>{if(e.lengthComputable)$('i',p).style.width=(e.loaded/e.total*100)+'%'};
  x.onload=()=>{try{const t=JSON.parse(x.responseText);if(t.ok){overlay('Update wird installiert …','Der Empfänger startet gleich neu.');setTimeout(waitBack,4000)}else{toast(t.err||'Update fehlgeschlagen',1)}}catch(e){toast('Update fehlgeschlagen',1)}};
  x.onerror=()=>toast('Verbindung unterbrochen',1);x.open('POST','/api/update');x.setRequestHeader('X-Requested-With','wmbus');x.send(fd)}
$('#fw').onchange=e=>upload(e.target.files[0]);
const dz=$('#drop');['dragover','dragenter'].forEach(v=>dz.addEventListener(v,e=>{e.preventDefault();dz.classList.add('dz')}));['dragleave','drop'].forEach(v=>dz.addEventListener(v,e=>{e.preventDefault();dz.classList.remove('dz')}));dz.addEventListener('drop',e=>upload(e.dataTransfer.files[0]));
$('#pwf').onsubmit=async e=>{e.preventDefault();const f=e.target,er=$('#pwerr');er.textContent='';
  if(f.wp.value!==$('#wp2').value){er.textContent='Die beiden Passwörter sind nicht gleich.';return}
  try{const r=await fetch('/api/config',{method:'POST',headers:POSTH,body:new URLSearchParams({wp:f.wp.value})});const t=await r.json();
   if(t.ok){$('#pwm').classList.remove('on');overlay('Passwort gespeichert – der Empfänger startet neu …','Melde dich danach mit dem Benutzer admin und dem neuen Passwort an.');setTimeout(waitBack,4000)}
   else er.textContent=t.err||'Fehler beim Speichern'}catch(x){er.textContent='Keine Verbindung zum Empfänger'}};
addEventListener('resize',()=>drawAll());
refresh(true);setInterval(()=>refresh(false),5000);
</script></body></html>)rawliteral";
