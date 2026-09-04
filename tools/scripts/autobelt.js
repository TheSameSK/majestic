(function () {
  const group = nct.ui.group("auto-belt");
  const enabled = group.checkbox("enable", true);

  let interval = null;
  let lastVehicle = null;
  let enteredAt = 0;
  let autoDelay = 0;
  let lastTry = 0;
  let prevBeltState = false;
  let manualDisabled = false;
  let autoRequested = false;

  function randomDelay() {
    return 1500 + Math.floor(Math.random() * 2001);
  }

  function getLocalPlayer() {
    if (typeof mp === "undefined" || !mp.players || !mp.players.local) return null;
    return mp.players.local;
  }

  function isBeltActive() {
    return !!globalThis.isSeatbeltActive;
  }

  function isBlocked() {
    if (typeof globalThis === "undefined") return false;

    if (globalThis.interfaces) {
      if (globalThis.interfaces.chat) return true;
      if (globalThis.interfaces.adminReport) return true;
      if (globalThis.interfaces.iphone) return true;
    }

    if (typeof mp !== "undefined" && mp.gui && mp.gui.cursor && mp.gui.cursor.visible) return true;

    return !!(globalThis.piano || globalThis.playableInstrument);
  }

  function canUseSeatbelt(player, vehicle) {
    if (!player || !vehicle) return false;
    if (isBlocked()) return false;

    if (typeof player.isDead === "function" && player.isDead()) return false;
    if (player.minigame) return false;

    return true;
  }

  function resetVehicleState(vehicle) {
    lastVehicle = vehicle;
    enteredAt = Date.now();
    autoDelay = randomDelay();
    lastTry = 0;
    prevBeltState = isBeltActive();
    manualDisabled = false;
    autoRequested = false;
  }

  function tryFastenSeatbelt() {
    const player = getLocalPlayer();
    if (!player) return;

    const vehicle = player.vehicle;
    const beltNow = isBeltActive();

    if (!enabled.get()) {
      prevBeltState = beltNow;
      return;
    }

    if (!vehicle) {
      lastVehicle = null;
      enteredAt = 0;
      autoDelay = 0;
      lastTry = 0;
      prevBeltState = beltNow;
      manualDisabled = false;
      autoRequested = false;
      return;
    }

    if (vehicle !== lastVehicle) {
      resetVehicleState(vehicle);
      return;
    }

    if (prevBeltState && !beltNow) {
      manualDisabled = true;
    }

    prevBeltState = beltNow;

    if (manualDisabled) return;
    if (beltNow) return;
    if (!canUseSeatbelt(player, vehicle)) return;
    if (Date.now() - enteredAt < autoDelay) return;
    if (Date.now() - lastTry < 1500) return;

    lastTry = Date.now();
    autoRequested = true;

    if (mp.events && typeof mp.events.call === "function") {
      mp.events.call("seatbelt.toggle");
    }
  }

  function start() {
    if (interval) return;
    interval = setInterval(tryFastenSeatbelt, 250);
  }

  function stop() {
    if (interval) {
      clearInterval(interval);
      interval = null;
    }

    lastVehicle = null;
    enteredAt = 0;
    autoDelay = 0;
    lastTry = 0;
    prevBeltState = false;
    manualDisabled = false;
    autoRequested = false;
  }

  function updateState() {
    if (enabled.get()) {
      start();
    } else {
      stop();
    }
  }

  enabled.set_callback(updateState, true);

  if (typeof onUnload === "function") {
    onUnload(function () {
      stop();
    });
  }
})();

/* ==========================================================================
   NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
   Inert per-file identifier required by the EULA. Uniquely tags this file so
   this program is distinguishable from any other build. No functional logic.
   file: tools/scripts/autobelt.js
   ========================================================================== */
var __noctua_license_4beb6582d0291a729941b4db29492fbe = { id: "f95f9d2e-e6f9-0a90-7595-3e1f22f5317f", file: "tools/scripts/autobelt.js", tag: [ 12, 0, 38, 58, 203, 1, 242, 59, 247, 9, 223, 77, 214, 41, 44, 156 ] };
void __noctua_license_4beb6582d0291a729941b4db29492fbe;
