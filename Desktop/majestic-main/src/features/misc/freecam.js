let freecamActive = false;
let freecamCam = null;
let freecamPos = { x: 0, y: 0, z: 0 };
let freecamRot = { x: 0, y: 0, z: 0 };
let freecamVelocity = { x: 0, y: 0, z: 0 };
let freecamTick = null;
let freecamKeydown = null;
let freecamKeyup = null;
let freecamPedLock = null;
let freecamPedControlsLocked = false;
const SPEED_MIN = 0.05;
const SPEED_MAX = 1.0;
const SPEED_SCROLL_STEP = 0.05;
let freecamSpeed =
  typeof globalThis.noctuaFreecamSpeed === "number"
    ? Math.max(SPEED_MIN, Math.min(SPEED_MAX, globalThis.noctuaFreecamSpeed))
    : 0.1;
const FOV =
  typeof globalThis.noctuaFreecamFov === "number"
    ? globalThis.noctuaFreecamFov
    : 60;
const SENSITIVITY =
  typeof globalThis.noctuaFreecamSensitivity === "number"
    ? globalThis.noctuaFreecamSensitivity
    : 4;
const LOOK_MULTIPLIER = 3.0;
function requireApproval() {
  return globalThis.noctuaFreecamRequireApproval === true;
}
function teleportKey() {
  return globalThis.noctuaFreecamTeleportKey || 0x54;
}
function safe(fn) {
  try {
    return fn();
  } catch (e) {
    try {
      alt.logError("[noctua_freecam] " + e.message);
    } catch (_) {}
    return undefined;
  }
}
function lerp(a, b, t) {
  return a + (b - a) * t;
}
function clamp(v, min, max) {
  return Math.max(min, Math.min(max, v));
}
function exists(entity) {
  return !!entity && safe(() => native.doesEntityExist(entity)) === true;
}
function callNative(name, ...args) {
  if (typeof native[name] === "function") safe(() => native[name](...args));
}
function localPlayerId() {
  return typeof native.playerId === "function"
    ? safe(() => native.playerId())
    : 0;
}
function isPedInVehicle(ped) {
  return safe(() => native.isPedInAnyVehicle(ped, false)) === true;
}
function stabilizePed() {
  const lock = freecamPedLock;
  if (!lock || !lock.ped || !exists(lock.ped) || isPedInVehicle(lock.ped))
    return;
  if (freecamPedControlsLocked) return;
  callNative("disablePlayerFiring", localPlayerId(), true);
  callNative("setPlayerControl", localPlayerId(), false, 256);
  freecamPedControlsLocked = true;
}
function capturePedLock(ped) {
  if (isPedInVehicle(ped)) {
    freecamPedLock = null;
    return;
  }
  freecamPedLock = { ped: ped };
  stabilizePed();
}
function releasePedLock(ped) {
  if (freecamPedControlsLocked) {
    callNative("disablePlayerFiring", localPlayerId(), false);
    callNative("setPlayerControl", localPlayerId(), true, 0);
  }
  freecamPedControlsLocked = false;
  freecamPedLock = null;
}
function createFreecam() {
  const player = alt.Player.local;
  if (!player || !player.valid) return false;
  const ped = player.scriptID;
  if (!exists(ped)) return false;
  const pos = player.pos;
  const rot = safe(() => native.getGameplayCamRot(2)) || {
    x: 0,
    y: 0,
    z: player.rot ? player.rot.z : 0,
  };
  freecamPos = { x: pos.x, y: pos.y, z: pos.z + 2 };
  freecamRot = { x: rot.x || 0, y: 0, z: rot.z || 0 };
  freecamVelocity = { x: 0, y: 0, z: 0 };
  freecamCam = safe(() =>
    native.createCamWithParams(
      "DEFAULT_SCRIPTED_CAMERA",
      freecamPos.x,
      freecamPos.y,
      freecamPos.z,
      freecamRot.x,
      freecamRot.y,
      freecamRot.z,
      FOV,
      true,
      2,
    ),
  );
  if (!freecamCam) return false;
  safe(() => native.setCamActive(freecamCam, true));
  safe(() => native.renderScriptCams(true, false, 0, true, false, 0));
  capturePedLock(ped);
  freecamActive = true;
  return true;
}
function destroyFreecam() {
  const player = alt.Player.local;
  const ped = player && player.valid ? player.scriptID : 0;
  if (freecamCam) {
    safe(() => native.renderScriptCams(false, false, 0, true, false, 0));
    safe(() => native.setCamActive(freecamCam, false));
    safe(() => native.destroyCam(freecamCam, false));
    freecamCam = null;
  }
  releasePedLock(ped);
  if (ped) {
    safe(() => native.setEntityAlpha(ped, 255, false));
    safe(() => native.setEntityCollision(ped, true, true));
  }
  freecamActive = false;
}
function updateRotation() {
  const mouseX = safe(() => native.getDisabledControlNormal(0, 1)) || 0;
  const mouseY = safe(() => native.getDisabledControlNormal(0, 2)) || 0;
  freecamRot.z -= mouseX * SENSITIVITY * LOOK_MULTIPLIER;
  freecamRot.x -= mouseY * SENSITIVITY * LOOK_MULTIPLIER;
  freecamRot.x = clamp(freecamRot.x, -89, 89);
  if (freecamRot.z > 180) freecamRot.z -= 360;
  if (freecamRot.z < -180) freecamRot.z += 360;
}
function saveSpeed() {
  globalThis.noctuaFreecamSpeed = freecamSpeed;
}
function updateSpeed() {
  const up =
    safe(() => native.isDisabledControlJustPressed(0, 15)) ||
    safe(() => native.isControlJustPressed(0, 15));
  const down =
    safe(() => native.isDisabledControlJustPressed(0, 14)) ||
    safe(() => native.isControlJustPressed(0, 14));
  const old = freecamSpeed;
  if (up)
    freecamSpeed = clamp(
      freecamSpeed + SPEED_SCROLL_STEP,
      SPEED_MIN,
      SPEED_MAX,
    );
  if (down)
    freecamSpeed = clamp(
      freecamSpeed - SPEED_SCROLL_STEP,
      SPEED_MIN,
      SPEED_MAX,
    );
  if (freecamSpeed !== old) saveSpeed();
}
function updatePosition() {
  updateSpeed();
  let speed = freecamSpeed;
  if (
    safe(() => native.isDisabledControlPressed(0, 21)) ||
    safe(() => native.isControlPressed(0, 21)) ||
    alt.isKeyDown(0x10)
  )
    speed = freecamSpeed * 4;
  const radZ = (freecamRot.z * Math.PI) / 180;
  const cosZ = Math.cos(radZ);
  const sinZ = Math.sin(radZ);
  let forward = 0,
    right = 0,
    up = 0;
  if (safe(() => native.isDisabledControlPressed(0, 32))) forward = 1;
  if (safe(() => native.isDisabledControlPressed(0, 33))) forward = -1;
  if (safe(() => native.isDisabledControlPressed(0, 34))) right = -1;
  if (safe(() => native.isDisabledControlPressed(0, 35))) right = 1;
  if (safe(() => native.isDisabledControlPressed(0, 22)) || alt.isKeyDown(0x20))
    up = 1;
  if (safe(() => native.isDisabledControlPressed(0, 36)) || alt.isKeyDown(0x11))
    up = -1;
  const target = {
    x: (-sinZ * forward + cosZ * right) * speed,
    y: (cosZ * forward + sinZ * right) * speed,
    z: up * speed,
  };
  freecamVelocity.x = lerp(freecamVelocity.x, target.x, 0.15);
  freecamVelocity.y = lerp(freecamVelocity.y, target.y, 0.15);
  freecamVelocity.z = lerp(freecamVelocity.z, target.z, 0.15);
  freecamPos.x += freecamVelocity.x;
  freecamPos.y += freecamVelocity.y;
  freecamPos.z += freecamVelocity.z;
}
function applyFreecam() {
  if (!freecamCam) return;
  safe(() =>
    native.setCamCoord(freecamCam, freecamPos.x, freecamPos.y, freecamPos.z),
  );
  safe(() =>
    native.setCamRot(freecamCam, freecamRot.x, freecamRot.y, freecamRot.z, 2),
  );
}
function getForwardPoint(distance) {
  const radZ = (freecamRot.z * Math.PI) / 180;
  const radX = (freecamRot.x * Math.PI) / 180;
  const cosX = Math.cos(radX);
  return {
    x: freecamPos.x + -Math.sin(radZ) * cosX * distance,
    y: freecamPos.y + Math.cos(radZ) * cosX * distance,
    z: freecamPos.z + Math.sin(radX) * distance,
  };
}
function isHitValue(value) {
  return value === true || value === 1;
}
function getShapeHit(result) {
  if (!Array.isArray(result)) return null;
  let hit = false,
    coords = null,
    normal = null;
  if (
    result.length >= 4 &&
    (typeof result[1] === "boolean" || typeof result[1] === "number")
  ) {
    if (result[0] === 1) return null;
    hit = isHitValue(result[1]);
    coords = result[2];
    normal = result[3];
  } else if (typeof result[0] === "boolean" || typeof result[0] === "number") {
    hit = isHitValue(result[0]);
    coords = result[1];
    normal = result[2];
  }
  if (!hit || !coords) return null;
  return { coords: coords, normal: normal || { x: 0, y: 0, z: 1 } };
}
function startRay(from, to, ignored) {
  if (typeof native.startExpensiveSynchronousShapeTestLosProbe === "function")
    return native.startExpensiveSynchronousShapeTestLosProbe(
      from.x,
      from.y,
      from.z,
      to.x,
      to.y,
      to.z,
      -1,
      ignored,
      17,
    );
  if (typeof native.startShapeTestRay === "function")
    return native.startShapeTestRay(
      from.x,
      from.y,
      from.z,
      to.x,
      to.y,
      to.z,
      -1,
      ignored,
      17,
    );
  if (typeof native.startShapeTestLosProbe === "function")
    return native.startShapeTestLosProbe(
      from.x,
      from.y,
      from.z,
      to.x,
      to.y,
      to.z,
      -1,
      ignored,
      17,
    );
  if (typeof native._startShapeTestRay === "function")
    return native._startShapeTestRay(
      from.x,
      from.y,
      from.z,
      to.x,
      to.y,
      to.z,
      -1,
      ignored,
      17,
    );
  return 0;
}
function castRay(from, to) {
  const player = alt.Player.local;
  if (!player || !player.valid) return null;
  const ray = safe(() => startRay(from, to, player.scriptID));
  if (!ray) return null;
  return getShapeHit(safe(() => native.getShapeTestResult(ray)));
}
function getTeleportTarget() {
  const fallback = getForwardPoint(1000);
  const hit = castRay(freecamPos, fallback);
  if (!hit) return null;
  const pos = hit.coords;
  const normal = hit.normal;
  if (normal.z > 0.5) return { x: pos.x, y: pos.y, z: pos.z + 1 };
  const high = { x: pos.x, y: pos.y, z: pos.z + 300 };
  const low = { x: pos.x, y: pos.y, z: pos.z - 0.3 };
  const ground = castRay(high, low);
  if (!ground) return null;
  return { x: ground.coords.x, y: ground.coords.y, z: ground.coords.z + 1 };
}
function drawTeleportPreview() {
  const target = getTeleportTarget();
  if (!target) return;
  safe(() =>
    native.drawMarker(
      1,
      target.x,
      target.y,
      target.z - 1,
      0,
      0,
      0,
      0,
      0,
      0,
      1.05,
      1.05,
      0.35,
      255,
      45,
      45,
      135,
      false,
      true,
      2,
      false,
      null,
      null,
      false,
    ),
  );
}
function notifyFreecamStopped() {
  safe(() => {
    if (typeof globalThis.noctuaBridgeSend === "function")
      globalThis.noctuaBridgeSend({ type: "FREECAM_STOPPED" });
  });
}
function teleportToCamera() {
  const player = alt.Player.local;
  if (!player || !player.valid) return;
  const target = getTeleportTarget();
  if (!target) return;
  const ped = player.scriptID;
  if (!exists(ped)) return;
  safe(() => native.freezeEntityPosition(ped, true));
  safe(() => native.setEntityVelocity(ped, 0, 0, 0));
  alt.setTimeout(function () {
    if (!player.valid || !exists(ped)) return;
    safe(() =>
      native.setEntityCoordsNoOffset(
        ped,
        target.x,
        target.y,
        target.z,
        false,
        false,
        false,
      ),
    );
    safe(() => native.setEntityVelocity(ped, 0, 0, 0));
    alt.setTimeout(function () {
      if (player.valid && exists(ped)) {
        safe(() => native.freezeEntityPosition(ped, false));
        notifyFreecamStopped();
        stopFreecam();
      }
    }, 40);
  }, 10);
}
function onTick() {
  if (!freecamActive || !freecamCam) return;
  safe(() => native.disableAllControlActions(0));
  safe(() => native.disableAllControlActions(1));
  safe(() => native.disableAllControlActions(2));
  stabilizePed();
  updateRotation();
  updatePosition();
  applyFreecam();
  stabilizePed();
  if (alt.isKeyDown(teleportKey())) {
    drawTeleportPreview();
    if (
      requireApproval() &&
      (safe(() => native.isDisabledControlJustPressed(0, 25)) ||
        safe(() => native.isControlJustPressed(0, 25)))
    )
      teleportToCamera();
  }
}
function onKeydown(key) {}
function onKeyup(key) {
  if (!requireApproval() && key === teleportKey()) teleportToCamera();
}
function startFreecam() {
  if (freecamActive) return;
  if (!createFreecam()) return;
  freecamTick = alt.everyTick(onTick);
  freecamKeydown = onKeydown;
  freecamKeyup = onKeyup;
  alt.on("keydown", freecamKeydown);
  alt.on("keyup", freecamKeyup);
}
function stopFreecam() {
  saveSpeed();
  if (freecamTick) {
    alt.clearEveryTick(freecamTick);
    freecamTick = null;
  }
  if (freecamKeydown) {
    try {
      alt.off("keydown", freecamKeydown);
    } catch (e) {}
    freecamKeydown = null;
  }
  if (freecamKeyup) {
    try {
      alt.off("keyup", freecamKeyup);
    } catch (e) {}
    freecamKeyup = null;
  }
  destroyFreecam();
}
function unloadFreecam() {
  stopFreecam();
  freecamPos = { x: 0, y: 0, z: 0 };
  freecamRot = { x: 0, y: 0, z: 0 };
  freecamVelocity = { x: 0, y: 0, z: 0 };
  freecamPedLock = null;
  freecamPedControlsLocked = false;
}
if (typeof onUnload === "function") onUnload(unloadFreecam);
startFreecam();


/* ==========================================================================
   NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
   Inert per-file identifier required by the EULA. Uniquely tags this file so
   this program is distinguishable from any other build. No functional logic.
   file: src/features/misc/freecam.js
   ========================================================================== */
var __noctua_license_df7a40e35bae6996477cff4ed60fabbd = { id: "a9c66c28-986e-3277-5882-32e09b26b3ef", file: "src/features/misc/freecam.js", tag: [ 153, 124, 221, 0, 122, 21, 99, 18, 5, 196, 111, 142, 0, 84, 111, 216 ] };
void __noctua_license_df7a40e35bae6996477cff4ed60fabbd;
