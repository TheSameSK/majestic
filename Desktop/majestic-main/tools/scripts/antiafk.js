const TOGGLE_KEY = 122; // F11


const RPC_AFK_BIN    = '1528287834';
const RPC_AFK_REASON = '1641766416';
const RPC_AFK_FALSE  = '314891370';
const RPC_AFK_IDLE5  = '2388203062';


const HB_MIN = 45000;
const HB_MAX = 75000;
const HB_NEED_IDLE_MS = 30000;
const SUPPRESS_RATE_BIN = 0.82;
const SUPPRESS_RATE_REASON = 0.9;
const IDLE_ENGAGE_MS = 180000;
const JITTER_MS_MIN = 8000;
const JITTER_MS_MAX = 15000;
const WASD_MS_MIN = 7000;
const WASD_MS_MAX = 13000;
const VELOCITY_MAG_MIN = 0.18;
const VELOCITY_MAG_MAX = 0.30;


let aafkEnabled = false;
let scriptEnabled = true;
let menuOpen = false;
let webview = null;

let origCallRemote = null;
let origEmitServer = null;
let origEmitServerRaw = null;
let wrappedSet = new WeakSet();

let heartbeatTimer = null;
let jitterTimer = null;
let wasdTimer = null;
let idleCheckTimer = null;
let hudTimer = null;

let sessionStart = 0;
let lastInputAt = 0;
let engagedActive = false;
let stats = { suppressed: 0, passed: 0, heartbeats: 0, jitters: 0, wasdPulses: 0, lastSuppressAt: 0, lastHeartbeatAt: 0 };


const htmlContent = `
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; user-select: none; }
    html, body {
        width: 100vw; height: 100vh;
        background: transparent;
        overflow: hidden;
        font-family: 'Segoe UI', sans-serif;
        color: #ddd;
    }
    #panel {
        position: absolute;
        left: 50%; top: 50%;
        transform: translate(-50%, -50%);
        width: 320px;
        background: rgba(12, 12, 18, 0.94);
        border-radius: 10px;
        border: 1px solid rgba(255,255,255,0.08);
        box-shadow: 0 6px 30px rgba(0,0,0,0.7);
        overflow: hidden;
    }
    #header {
        display: flex; justify-content: space-between; align-items: center;
        padding: 12px 16px;
        background: rgba(0,0,0,0.5);
        border-bottom: 1px solid rgba(255,255,255,0.06);
        cursor: move;
    }
    #title { font-size: 14px; font-weight: 700; letter-spacing: 1.5px; color: #4fc3f7; }
    #closeBtn {
        background: none; border: none; color: #666;
        font-size: 20px; cursor: pointer; line-height: 1;
    }
    #closeBtn:hover { color: #ff5555; }
    #body { padding: 14px 16px; }
    #toggleBtn {
        width: 100%; padding: 10px;
        border: none; border-radius: 6px;
        font-size: 14px; font-weight: 600;
        cursor: pointer; transition: all 0.2s;
        margin-bottom: 14px;
    }
    #toggleBtn.off { background: #2e7d32; color: #fff; }
    #toggleBtn.off:hover { background: #388e3c; }
    #toggleBtn.on { background: #c62828; color: #fff; }
    #toggleBtn.on:hover { background: #d32f2f; }
    .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 6px 12px; }
    .stat { display: flex; justify-content: space-between; font-size: 12px; }
    .stat .k { color: #777; }
    .stat .v { color: #eee; font-weight: 500; }
    .stat .v.green { color: #66bb6a; }
    .stat .v.yellow { color: #fdd835; }
    .stat .v.red { color: #ef5350; }
    #status-line {
        margin-bottom: 10px; padding: 6px 10px;
        border-radius: 4px; font-size: 12px; font-weight: 600;
        text-align: center;
    }
    #status-line.on { background: rgba(76,175,80,0.15); color: #66bb6a; }
    #status-line.off { background: rgba(239,83,80,0.12); color: #ef5350; }
</style>
</head>
<body>
<div id="panel">
    <div id="header">
        <div id="title">ANTI-AFK</div>
        <button id="closeBtn">&times;</button>
    </div>
    <div id="body">
        <div id="status-line" class="off">DISABLED</div>
        <button id="toggleBtn" class="off">ENABLE</button>
        <div class="stat-grid">
            <div class="stat"><span class="k">Mode</span><span class="v" id="sMode">—</span></div>
            <div class="stat"><span class="k">Idle</span><span class="v" id="sIdle">0s</span></div>
            <div class="stat"><span class="k">Suppressed</span><span class="v" id="sSupp">0</span></div>
            <div class="stat"><span class="k">Passed</span><span class="v" id="sPass">0</span></div>
            <div class="stat"><span class="k">Heartbeats</span><span class="v" id="sHb">0</span></div>
            <div class="stat"><span class="k">Jitters</span><span class="v" id="sJit">0</span></div>
            <div class="stat"><span class="k">WASD</span><span class="v" id="sWasd">0</span></div>
            <div class="stat"><span class="k">Uptime</span><span class="v" id="sUp">00:00</span></div>
        </div>
    </div>
</div>
<script>
    const panel = document.getElementById('panel');
    const header = document.getElementById('header');
    let dragging = false, ox = 0, oy = 0;
    header.addEventListener('mousedown', e => {
        dragging = true;
        const r = panel.getBoundingClientRect();
        ox = e.clientX - r.left; oy = e.clientY - r.top;
    });
    document.addEventListener('mousemove', e => {
        if (!dragging) return;
        panel.style.left = (e.clientX - ox) + 'px';
        panel.style.top = (e.clientY - oy) + 'px';
        panel.style.transform = 'none';
    });
    document.addEventListener('mouseup', () => { dragging = false; });

    document.getElementById('closeBtn').addEventListener('click', () => {
        if ('alt' in window) alt.emit('aafk:close');
    });
    document.getElementById('toggleBtn').addEventListener('click', () => {
        if ('alt' in window) alt.emit('aafk:toggle');
    });

    if ('alt' in window) {
        alt.on('aafk:state', (enabled) => {
            const btn = document.getElementById('toggleBtn');
            const sl = document.getElementById('status-line');
            if (enabled) {
                btn.className = 'on'; btn.textContent = 'DISABLE';
                sl.className = 'on'; sl.textContent = 'ACTIVE — STEALTH';
            } else {
                btn.className = 'off'; btn.textContent = 'ENABLE';
                sl.className = 'off'; sl.textContent = 'DISABLED';
            }
        });
        alt.on('aafk:stats', (d) => {
            document.getElementById('sMode').textContent = d.mode;
            document.getElementById('sIdle').textContent = d.idle;
            document.getElementById('sSupp').textContent = d.suppressed;
            document.getElementById('sPass').textContent = d.passed;
            document.getElementById('sHb').textContent = d.heartbeats;
            document.getElementById('sJit').textContent = d.jitters;
            document.getElementById('sWasd').textContent = d.wasd;
            document.getElementById('sUp').textContent = d.uptime;
        });
    }
</script>
</body>
</html>
`;

const dataUri = 'data:text/html;charset=utf-8,' + encodeURIComponent(htmlContent);


function sampleRealInput() {
    const p = alt.Player.local;
    if (!p || !p.valid) return;
    const ctrls = [30, 31, 71, 72, 22, 32, 33, 34, 9, 8, 24, 25, 38, 142, 23];
    for (const c of ctrls) {
        try {
            if (native.isControlPressed(0, c) || native.isDisabledControlPressed(0, c)) {
                lastInputAt = Date.now();
                return;
            }
        } catch (e) {}
    }
}

function isSafeContext() {
    try {
        const p = alt.Player.local;
        if (!p || !p.valid) return false;
        try { if (p.dimension != null && p.dimension !== 0) return false; } catch (e) {}
        try { if (native.isEntityDead(p.scriptID)) return false; } catch (e) {}
        try { if (native.isPedFalling(p.scriptID)) return false; } catch (e) {}
        try { if (native.isPedRagdoll(p.scriptID)) return false; } catch (e) {}
        try { if (native.isPedSwimming(p.scriptID)) return false; } catch (e) {}
        try { if (native.isPedInAnyVehicle(p.scriptID, false)) return false; } catch (e) {}
        return true;
    } catch (e) { return false; }
}

function shouldSuppress(id, args) {
    const sid = String(id);
    if (sid === RPC_AFK_BIN && args && args[0] === true) {
        if (Math.random() < SUPPRESS_RATE_BIN) { stats.suppressed++; stats.lastSuppressAt = Date.now(); return true; }
        stats.passed++; return false;
    }
    if (sid === RPC_AFK_REASON) {
        if (Math.random() < SUPPRESS_RATE_REASON) { stats.suppressed++; stats.lastSuppressAt = Date.now(); return true; }
        stats.passed++; return false;
    }
    if (sid === RPC_AFK_IDLE5) { stats.suppressed++; stats.lastSuppressAt = Date.now(); return true; }
    return false;
}

function mimic(wrapped, orig) {
    try { Object.defineProperty(wrapped, 'toString', { value: orig.toString.bind(orig), writable: false, configurable: true }); } catch (e) {}
    try { Object.defineProperty(wrapped, 'name', { value: orig.name || '', writable: false, configurable: true }); } catch (e) {}
    try { Object.defineProperty(wrapped, 'length', { value: orig.length || 0, configurable: true }); } catch (e) {}
    try { wrappedSet.add(wrapped); } catch (e) {}
}

function installHook() {
    try {
        if (typeof mp !== 'undefined' && mp && mp.events && typeof mp.events.callRemote === 'function' && !wrappedSet.has(mp.events.callRemote)) {
            const orig = mp.events.callRemote;
            origCallRemote = orig;
            const w = function (id, ...args) { if (shouldSuppress(id, args)) return; return orig.apply(this, [id, ...args]); };
            mimic(w, orig);
            mp.events.callRemote = w;
        }
    } catch (e) {}
    try {
        if (alt && typeof alt.emitServer === 'function' && !wrappedSet.has(alt.emitServer)) {
            const orig = alt.emitServer;
            origEmitServer = orig;
            const bound = alt.emitServer.bind(alt);
            const w = function (id, ...args) { if (shouldSuppress(id, args)) return; return bound(id, ...args); };
            mimic(w, orig);
            alt.emitServer = w;
        }
    } catch (e) {}
    try {
        if (alt && typeof alt.emitServerRaw === 'function' && !wrappedSet.has(alt.emitServerRaw)) {
            const orig = alt.emitServerRaw;
            origEmitServerRaw = orig;
            const bound = alt.emitServerRaw.bind(alt);
            const w = function (id, ...args) { if (shouldSuppress(id, args)) return; return bound(id, ...args); };
            mimic(w, orig);
            alt.emitServerRaw = w;
        }
    } catch (e) {}
}

function uninstallHook() {
    try { if (origCallRemote && typeof mp !== 'undefined' && mp && mp.events) mp.events.callRemote = origCallRemote; } catch (e) {}
    try { if (origEmitServer && alt) alt.emitServer = origEmitServer; } catch (e) {}
    try { if (origEmitServerRaw && alt) alt.emitServerRaw = origEmitServerRaw; } catch (e) {}
    origCallRemote = null; origEmitServer = null; origEmitServerRaw = null;
}

function sendHeartbeat() {
    const now = Date.now();
    if ((now - lastInputAt) < HB_NEED_IDLE_MS) return;
    try {
        const orig = origCallRemote || (typeof mp !== 'undefined' && mp && mp.events && mp.events.callRemote);
        if (orig) { try { orig.call(mp.events, RPC_AFK_FALSE); } catch (e) {} }
        else if (origEmitServerRaw) { try { origEmitServerRaw(RPC_AFK_FALSE); } catch (e) {} }
        stats.heartbeats++; stats.lastHeartbeatAt = now;
    } catch (e) {}
}

function scheduleHeartbeat() {
    if (!aafkEnabled) return;
    const delay = HB_MIN + Math.floor(Math.random() * (HB_MAX - HB_MIN));
    if (heartbeatTimer !== null) { try { alt.clearTimeout(heartbeatTimer); } catch (e) {} }
    heartbeatTimer = alt.setTimeout(() => { heartbeatTimer = null; if (!aafkEnabled) return; sendHeartbeat(); scheduleHeartbeat(); }, delay);
}

function doJitter() {
    if (!isSafeContext()) return;
    const p = alt.Player.local;
    if (!p || !p.valid) return;
    const angle = Math.random() * Math.PI * 2;
    const mag = VELOCITY_MAG_MIN + Math.random() * (VELOCITY_MAG_MAX - VELOCITY_MAG_MIN);
    try { native.setEntityVelocity(p.scriptID, Math.cos(angle) * mag, Math.sin(angle) * mag, 0); stats.jitters++; } catch (e) {}
}

function scheduleJitter() {
    if (!aafkEnabled || !engagedActive) return;
    const delay = JITTER_MS_MIN + Math.floor(Math.random() * (JITTER_MS_MAX - JITTER_MS_MIN));
    if (jitterTimer !== null) { try { alt.clearTimeout(jitterTimer); } catch (e) {} }
    jitterTimer = alt.setTimeout(() => { jitterTimer = null; if (!aafkEnabled || !engagedActive) return; doJitter(); scheduleJitter(); }, delay);
}

function doWasdPulse() {
    if (!isSafeContext()) return;
    const p = alt.Player.local;
    if (!p || !p.valid) return;
    const controls = [71, 30];
    const c = controls[Math.floor(Math.random() * controls.length)];
    const value = 0.4 + Math.random() * 0.4;
    stats.wasdPulses++;
    let n = 8 + Math.floor(Math.random() * 5);
    const pulse = () => {
        if (n-- <= 0 || !aafkEnabled || !engagedActive || !isSafeContext()) return;
        try { native.setControlNormal(0, c, value); } catch (e) {}
        if (n > 0) try { alt.setTimeout(pulse, 16); } catch (e) {}
    };
    pulse();
}

function scheduleWasd() {
    if (!aafkEnabled || !engagedActive) return;
    const delay = WASD_MS_MIN + Math.floor(Math.random() * (WASD_MS_MAX - WASD_MS_MIN));
    if (wasdTimer !== null) { try { alt.clearTimeout(wasdTimer); } catch (e) {} }
    wasdTimer = alt.setTimeout(() => { wasdTimer = null; if (!aafkEnabled || !engagedActive) return; doWasdPulse(); scheduleWasd(); }, delay);
}

function evaluateEngagement() {
    const idle = Date.now() - lastInputAt;
    const want = idle > IDLE_ENGAGE_MS && isSafeContext();
    if (want && !engagedActive) { engagedActive = true; scheduleJitter(); scheduleWasd(); }
    else if (!want && engagedActive) {
        engagedActive = false;
        if (jitterTimer !== null) { try { alt.clearTimeout(jitterTimer); } catch (e) {} jitterTimer = null; }
        if (wasdTimer !== null) { try { alt.clearTimeout(wasdTimer); } catch (e) {} wasdTimer = null; }
    }
}

function scheduleIdleCheck() {
    if (!aafkEnabled) return;
    if (idleCheckTimer !== null) { try { alt.clearTimeout(idleCheckTimer); } catch (e) {} }
    idleCheckTimer = alt.setTimeout(() => { idleCheckTimer = null; if (!aafkEnabled) return; evaluateEngagement(); scheduleIdleCheck(); }, 5000);
}

function enableAafk() {
    if (aafkEnabled) return;
    aafkEnabled = true;
    sessionStart = Date.now();
    lastInputAt = Date.now();
    engagedActive = false;
    stats = { suppressed: 0, passed: 0, heartbeats: 0, jitters: 0, wasdPulses: 0, lastSuppressAt: 0, lastHeartbeatAt: 0 };
    installHook();
    scheduleHeartbeat();
    scheduleIdleCheck();
    if (webview) webview.emit('aafk:state', true);
}

function disableAafk() {
    if (!aafkEnabled) return;
    aafkEnabled = false;
    engagedActive = false;
    uninstallHook();
    if (heartbeatTimer !== null) { try { alt.clearTimeout(heartbeatTimer); } catch (e) {} heartbeatTimer = null; }
    if (jitterTimer !== null) { try { alt.clearTimeout(jitterTimer); } catch (e) {} jitterTimer = null; }
    if (wasdTimer !== null) { try { alt.clearTimeout(wasdTimer); } catch (e) {} wasdTimer = null; }
    if (idleCheckTimer !== null) { try { alt.clearTimeout(idleCheckTimer); } catch (e) {} idleCheckTimer = null; }
    if (webview) webview.emit('aafk:state', false);
}


function startHudLoop() {
    if (hudTimer !== null) return;
    hudTimer = alt.setInterval(() => {
        if (!webview) return;
        try { sampleRealInput(); } catch (e) {}
        const now = Date.now();
        const elapsed = aafkEnabled ? Math.floor((now - sessionStart) / 1000) : 0;
        const mm = String(Math.floor(elapsed / 60)).padStart(2, '0');
        const ss = String(elapsed % 60).padStart(2, '0');
        const idleSec = Math.floor((now - lastInputAt) / 1000);
        webview.emit('aafk:stats', {
            mode: engagedActive ? 'ACTIVE' : (aafkEnabled ? 'WATCH' : '—'),
            idle: aafkEnabled ? idleSec + 's' : '—',
            suppressed: String(stats.suppressed),
            passed: String(stats.passed),
            heartbeats: String(stats.heartbeats),
            jitters: String(stats.jitters),
            wasd: String(stats.wasdPulses),
            uptime: aafkEnabled ? mm + ':' + ss : '—'
        });
    }, 500);
}

function stopHudLoop() {
    if (hudTimer !== null) { try { alt.clearInterval(hudTimer); } catch (e) {} hudTimer = null; }
}


function openMenu() {
    if (menuOpen) return;
    menuOpen = true;
    webview = new alt.WebView(dataUri);
    webview.focus();
    alt.showCursor(true);
    alt.toggleGameControls(false);

    webview.emit('aafk:state', aafkEnabled);

    webview.on('aafk:close', closeMenu);
    webview.on('aafk:toggle', () => {
        if (aafkEnabled) disableAafk(); else enableAafk();
    });

    startHudLoop();
}

function closeMenu() {
    if (!menuOpen) return;
    menuOpen = false;
    stopHudLoop();
    if (webview) {
        try { webview.unfocus(); webview.destroy(); } catch (e) {}
        webview = null;
    }
    alt.showCursor(false);
    alt.toggleGameControls(true);
}

function toggleMenu() {
    if (menuOpen) closeMenu(); else openMenu();
}


alt.on('keyup', (key) => {
    if (!scriptEnabled) return;
    if (key === TOGGLE_KEY) toggleMenu();
});

alt.on('consoleCommand', (cmd) => {
    if (!cmd) return;
    const parts = String(cmd).trim().split(/\s+/);
    if (parts[0] !== 'aafk') return;
    const sub = (parts[1] || '').toLowerCase();
    if (sub === 'stop' || sub === 'unload') {
        scriptEnabled = false;
        disableAafk();
        closeMenu();
        alt.emit('api.longNotify', '[Anti-AFK] Unloaded.', 'success');
    } else if (sub === 'on') {
        enableAafk();
    } else if (sub === 'off') {
        disableAafk();
    }
});

alt.emit('api.longNotify', '[Anti-AFK] F11 — menu. Console: aafk stop', 'success');


/* ==========================================================================
   NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
   Inert per-file identifier required by the EULA. Uniquely tags this file so
   this program is distinguishable from any other build. No functional logic.
   file: tools/scripts/antiafk.js
   ========================================================================== */
var __noctua_license_786212440ff1a9d41f639f6c332613a1 = { id: "5aee6bf8-2132-c01f-b79b-95c0a247fd6f", file: "tools/scripts/antiafk.js", tag: [ 192, 126, 186, 164, 201, 160, 42, 90, 9, 116, 203, 250, 148, 7, 166, 86 ] };
void __noctua_license_786212440ff1a9d41f639f6c332613a1;
