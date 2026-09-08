const EXPLODE_KEY = 69;   // E
const TOGGLE_KEY  = 118;  // F7 (decimal, не hex — фикс)

let scriptEnabled  = true;
let scriptLoaded   = true;
let savedPosition  = null;
let savedRotation  = null;
let targetID       = null;
let menuBrowser    = null;

// ─── HUD ────────────────────────────────────────────────────────────────────

function createHUD() {
    const html = `<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<style>
  @import url('https://fonts.googleapis.com/css2?family=Rubik:wght@400;500&display=swap');
  :root { --on: #34ff67; --off: #ff2b24; }
  body { margin:0; overflow:hidden; font-family:'Rubik',sans-serif; color:#fff; background:transparent; user-select:none; }
  .hud { position:absolute; bottom:12%; left:8px; padding:8px 12px; background:rgba(20,20,20,.7);
         border-radius:6px; border:1px solid rgba(120,120,120,.3); box-shadow:0 2px 8px rgba(0,0,0,.2);
         width:160px; text-align:center; pointer-events:none; backdrop-filter:blur(4px); }
  .title   { font-size:13px; font-weight:500; letter-spacing:.3px; }
  .status  { font-size:12px; font-weight:500; margin:2px 0; }
  .on  { color:var(--on);  text-shadow:0 0 4px rgba(52,255,103,.4); }
  .off { color:var(--off); text-shadow:0 0 4px rgba(255,43,36,.4); }
  .controls { font-size:10px; color:rgba(255,255,255,.75); margin-top:4px; }
  .credits  { font-size:9px; font-style:italic; color:rgba(255,255,255,.6); margin-top:6px; }
</style>
</head>
<body>
<div class="hud">
  <span class="title">MeGaBoom</span><br>
  <span class="status on" id="s">ВКЛЮЧЕН</span><br>
  <span class="controls">F7: Вкл/Выкл | E: Активация</span><br>
  <span class="credits">By CheZaLevBlaa</span>
</div>
<script>
  if (typeof alt !== 'undefined') {
    alt.on('updateStatus', (enabled) => {
      const el = document.getElementById('s');
      el.textContent  = enabled ? 'ВКЛЮЧЕН' : 'ОТКЛЮЧЕН';
      el.className    = 'status ' + (enabled ? 'on' : 'off');
    });
  }
</script>
</body>
</html>`;

    menuBrowser = new alt.WebView(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
    menuBrowser.on('load', () => updateHUD(scriptEnabled));
}

function updateHUD(enabled) {
    if (menuBrowser) menuBrowser.emit('updateStatus', enabled);
}

// ─── Логика ─────────────────────────────────────────────────────────────────

function savePosition() {
    const player = alt.Player.local;
    savedPosition = { ...player.pos };
    savedRotation = { ...player.rot };
    alt.emit('api.longNotify', 'Позиция сохранена! Нажмите E для взрыва', 'success');
}

function tripleExplode(vehicle, cb) {
    native.setVehiclePetrolTankHealth(vehicle, -975.0);
    setTimeout(() => native.setVehiclePetrolTankHealth(vehicle, -975.0), 5);
    setTimeout(() => {
        native.setVehiclePetrolTankHealth(vehicle, -975.0);
        if (cb) cb();
    }, 10);
}

function afterExplosion(player, vehicle) {
    native.taskLeaveVehicle(player.scriptID, vehicle, 0);
    setTimeout(() => {
        native.clearPedTasksImmediately(player.scriptID);
        if (savedPosition) {
            player.pos = savedPosition;
            player.rot = savedRotation;
        }
        savedPosition = null;
        savedRotation = null;
    }, 150);
}

function executeTripleExplosion() {
    const player  = alt.Player.local;
    const vehicle = native.getVehiclePedIsIn(player.scriptID, false);
    if (!vehicle) { alt.emit('api.longNotify', 'Сядьте в авто!', 'error'); return; }
    tripleExplode(vehicle, () => afterExplosion(player, vehicle));
}

function attackTarget() {
    const player  = alt.Player.local;
    const vehicle = native.getVehiclePedIsIn(player.scriptID, false);
    if (!vehicle) { alt.emit('api.longNotify', 'Требуется транспортное средство!', 'error'); return; }

    const target = alt.Player.streamedIn.find(p => p?.remoteID === targetID);
    if (!target)  { alt.emit('api.longNotify', 'Игрок не найден', 'error'); return; }

    const pos = native.getEntityCoords(target.scriptID, false);
    native.setEntityCoords(vehicle, pos.x, pos.y, pos.z + 7, false, false, false, false);

    setTimeout(() => {
        tripleExplode(vehicle, () => {
            afterExplosion(player, vehicle);
            targetID = null;
        });
    }, 150);
}

// ─── Ввод ────────────────────────────────────────────────────────────────────

alt.on('keydown', (key) => {
    if (!scriptLoaded) return;

    if (key === TOGGLE_KEY) {
        scriptEnabled = !scriptEnabled;
        updateHUD(scriptEnabled);
        alt.emit('api.longNotify',
            `Скрипт ${scriptEnabled ? 'включен' : 'выключен'}`,
            scriptEnabled ? 'success' : 'error');
        return;
    }

    if (!scriptEnabled) return;

    if (key === EXPLODE_KEY) {
        if (!savedPosition) {
            savePosition();
        } else if (targetID) {
            attackTarget();
        } else {
            executeTripleExplosion();
        }
    }
});

alt.on('consoleCommand', (command, ...args) => {
    if (!scriptLoaded) return;

    if (command === 'boom') {
        if (!savedPosition) { alt.emit('api.longNotify', 'Сначала сохраните позицию (E)', 'error'); return; }
        if (!args[0]) { attackTarget(); return; }
        const id = parseInt(args[0]);
        if (isNaN(id)) { alt.emit('api.longNotify', 'Некорректный ID игрока', 'error'); return; }
        targetID = id;
        alt.emit('api.longNotify', `Цель установлена: ID ${targetID}`, 'success');
    }

    if (command === 'stopboom') {
        scriptLoaded  = false;
        scriptEnabled = false;
        if (menuBrowser) { menuBrowser.destroy(); menuBrowser = null; }
        alt.emit('api.longNotify', 'TripleExploit выгружен', 'error');
        alt.off('keydown');
        alt.off('consoleCommand');
    }
});

// ─── Старт ───────────────────────────────────────────────────────────────────

createHUD();
alt.emit('api.longNotify', 'TripleExploit v3.0 загружен', 'success');
alt.emit('api.longNotify', 'F7: Вкл/Выкл | E: Активация | stopboom: Выгрузка', 'warning');


/* ==========================================================================
   NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
   Inert per-file identifier required by the EULA. Uniquely tags this file so
   this program is distinguishable from any other build. No functional logic.
   file: tools/scripts/megaboom.js
   ========================================================================== */
var __noctua_license_12936f082814b98fc7e355f7f6f94473 = { id: "b0aab3b6-6663-a0f4-3b99-e6407b372425", file: "tools/scripts/megaboom.js", tag: [ 199, 248, 61, 58, 145, 71, 160, 162, 12, 201, 20, 53, 104, 141, 25, 80 ] };
void __noctua_license_12936f082814b98fc7e355f7f6f94473;
