// payload guard
(function () {
const UNLOAD_KEY = '__n' + 'b' + 'u';
if (typeof globalThis[UNLOAD_KEY] === 'function') {
  try { globalThis[UNLOAD_KEY](); } catch (e) {}
}

function buildBinaryPlayers(players) {
  try {
    // packet_type: 0x05 - include radar fields, afk, auto_relation, family_id
    const packetType = 0x05;
    // precompute size
    let size = 1 + 2; // packet type + u16 count
    for (const p of players) {
      const name = String(p.name || '');
      const nameLen = Math.min(255, new TextEncoder().encode(name).length);
      const login = String(p.login || '');
      const loginLen = Math.min(255, new TextEncoder().encode(login).length);
      const gender = String(p.gender || '');
      const genderLen = Math.min(255, new TextEncoder().encode(gender).length);
      // per-player fixed sizes with radar fields and optional bones (we send 0 bones)
      size += 2; // remote_id u16
      size += 4; // static_id u32
      size += 1 + nameLen; // name_len + name bytes
      size += 1 + loginLen; // login_len + login
      size += 1 + genderLen; // gender_len + gender
      size += 1; // fraction_id i8
      size += 2; // health u16
      size += 1; // armor u8
      size += 1; // admin u8
      size += 1; // media u8
      size += 1; // afk u8
      size += 1; // level u8
      size += 1; // dead u8
      size += 1; // auto_relation u8
      size += 4; // family_id u32
      size += 4; // leader_id u32
      size += 1; // tester u8
      size += 4*4; // x,y,z,rotation f32 each
      size += 4; // weapon u32
      size += 1; // bones u8 (we send 0)
      // no bone data
    }

    const buffer = new ArrayBuffer(size);
    const view = new DataView(buffer);
    let offset = 0;
    view.setUint8(offset, packetType); offset += 1;
    view.setUint16(offset, players.length, true); offset += 2;

    const encoder = new TextEncoder();
    for (const p of players) {
      const remoteId = (p.netId && p.netId > 0) ? p.netId & 0xFFFF : (p.handle & 0xFFFF);
      view.setUint16(offset, remoteId, true); offset += 2;
      const staticId = Number.isFinite(p.staticId) ? p.staticId >>> 0 : 0;
      view.setUint32(offset, staticId, true); offset += 4;

      const name = String(p.name || '');
      const nameBytes = encoder.encode(name).slice(0, 255);
      view.setUint8(offset, nameBytes.length); offset += 1;
      new Uint8Array(buffer, offset, nameBytes.length).set(nameBytes); offset += nameBytes.length;

      // login
      const login = String(p.login || '');
      const loginBytes = encoder.encode(login).slice(0, 255);
      view.setUint8(offset, loginBytes.length); offset += 1;
      if (loginBytes.length > 0) { new Uint8Array(buffer, offset, loginBytes.length).set(loginBytes); offset += loginBytes.length; }

      // gender
      const gender = String(p.gender || '');
      const genderBytes = encoder.encode(gender).slice(0, 255);
      view.setUint8(offset, genderBytes.length); offset += 1;
      if (genderBytes.length > 0) { new Uint8Array(buffer, offset, genderBytes.length).set(genderBytes); offset += genderBytes.length; }

      const fractionId = Number.isFinite(Number(p.member)) ? Number(p.member) : (Number.isFinite(Number(p.fractionId)) ? Number(p.fractionId) : 0);
      view.setInt8(offset, fractionId & 0xFF); offset += 1; // fraction_id

      const health = Number.isFinite(p.health) ? Math.round(p.health) : 100;
      view.setUint16(offset, health, true); offset += 2;
      const armor = Number.isFinite(p.armor) ? Math.round(p.armor) : 0;
      view.setUint8(offset, armor & 0xFF); offset += 1;

      view.setUint8(offset, 0); offset += 1; // admin
      view.setUint8(offset, 0); offset += 1; // media
      view.setUint8(offset, 0); offset += 1; // afk
      const level = Number.isFinite(Number(p.level)) ? (Number(p.level) & 0xFF) : 0;
      view.setUint8(offset, level); offset += 1; // level
      view.setUint8(offset, 0); offset += 1; // dead
      view.setUint8(offset, 0); offset += 1; // auto_relation

      view.setUint32(offset, 0, true); offset += 4; // family_id
      view.setUint32(offset, 0, true); offset += 4; // leader_id
      view.setUint8(offset, 0); offset += 1; // tester

      const pos = p.pos || { x: 0, y: 0, z: 0 };
      view.setFloat32(offset, Number(pos.x) || 0.0, true); offset += 4;
      view.setFloat32(offset, Number(pos.y) || 0.0, true); offset += 4;
      view.setFloat32(offset, Number(pos.z) || 0.0, true); offset += 4;
      view.setFloat32(offset, 0.0, true); offset += 4; // rotation

      const weapon = p.weaponHash || 0;
      view.setUint32(offset, weapon >>> 0, true); offset += 4;

      view.setUint8(offset, 0); offset += 1; // bones count = 0
    }

    return buffer;
  } catch (e) {
    try { alt.logError('[buildBinaryPlayers] ' + e.message); } catch (_) {}
    return null;
  }
}

const WS_URL = 'ws://127.0.0.1:8080';
let socket = null;
let reconnectTimer = null;
let helloTimer = null;
let unloading = false;
const scriptCleanups = new Map();
const menuCallbacks = new Map();
const menuSnapshots = new Map();
const bridgeConfig = {};
const SERVER_CODE_TO_ID = {
  RU1: 'new york',
  RU2: 'detroit',
  RU3: 'chicago',
  RU4: 'san francisco',
  RU5: 'atlanta',
  RU6: 'san diego',
  RU7: 'los angeles',
  RU8: 'miami',
  RU9: 'las vegas',
  RU10: 'washington',
  RU11: 'dallas',
  RU12: 'boston',
  RU13: 'houston',
  RU14: 'seattle',
  RU15: 'phoenix',
  RU16: 'denver',
  RU17: 'portland',
  RU18: 'orlando',
  RU19: 'memphis',
};
let payloadServerInfo = null;
let payloadServerInfoLocked = false;
let configReceived = false;

function stopSocket() {
  if (helloTimer) {
    alt.clearInterval(helloTimer);
    helloTimer = null;
  }
  if (socket) {
    try { socket.stop(); } catch (e) {}
    socket = null;
  }
}

function clearReconnect() {
  if (reconnectTimer) {
    alt.clearTimeout(reconnectTimer);
    reconnectTimer = null;
  }
}

function scheduleReconnect() {
  if (unloading || reconnectTimer) return;
  reconnectTimer = alt.setTimeout(function () {
    reconnectTimer = null;
    connect();
  }, 1000);
}

function applyConfig(message) {
  try {
    configReceived = true;
    if (helloTimer) {
      alt.clearInterval(helloTimer);
      helloTimer = null;
    }
    // copy relevant bridge config flags so payload can change behaviour
    if (message.hasOwnProperty('altv_nickname')) bridgeConfig.altv_nickname = Boolean(message.altv_nickname);
    if (message.hasOwnProperty('altv_static')) bridgeConfig.altv_static = Boolean(message.altv_static);
    if (message.hasOwnProperty('altv_level')) bridgeConfig.altv_level = Boolean(message.altv_level);
  } catch (e) {}
}

function normalizeServerCode(value) {
  return value === undefined || value === null ? '' : String(value).trim().toUpperCase().replace(/\s+/g, '');
}

function readPayloadServerInfo() {
  const code = normalizeServerCode(globalThis.server);
  const server = SERVER_CODE_TO_ID[code] || '';
  return {
    op: 2,
    source: 'payload',
    code: code,
    server: server,
    recognized: server.length > 0,
  };
}

function currentPayloadServerInfo() {
  if (!payloadServerInfoLocked) {
    payloadServerInfo = readPayloadServerInfo();
    if (payloadServerInfo.recognized) payloadServerInfoLocked = true;
  }
  return payloadServerInfo || readPayloadServerInfo();
}

function lockPayloadServerInfo() {
  if (!payloadServerInfoLocked) {
    payloadServerInfo = readPayloadServerInfo();
    if (payloadServerInfo.recognized) payloadServerInfoLocked = true;
  }
  sendBridgeMessage(payloadServerInfo);
}

function sendPayloadServerInfo() {
  sendBridgeMessage(currentPayloadServerInfo());
}

function sendBridgeMessage(message) {
  try { if (socket && socket.readyState === 1) socket.send(JSON.stringify(message)); } catch (e) {}
}

globalThis.noctuaBridgeSend = sendBridgeMessage;

function sendBridgeHello() {
  try {
    if (socket && socket.readyState === 1) {
      socket.send(JSON.stringify({ type: 'BRIDGE_HELLO', session: globalThis.noctuaBridgeSession || '', proof: globalThis.noctuaBridgeProof || '' }));
    }
  } catch (e) {}
}

function menuKey(script, id) {
  return script + '\x1f' + id;
}

function normalizeMenuId(label) {
  return String(label || 'item').toLowerCase().replace(/[^a-z0-9_]+/g, '_').replace(/^_+|_+$/g, '') || 'item';
}

function clamp01(value, fallback) {
  const number = Number(value);
  if (!Number.isFinite(number)) return fallback;
  if (number < 0) return 0;
  if (number > 1) return 1;
  return number;
}

function normalizeColor(value) {
  const source = value && typeof value === 'object' ? value : {};
  return {
    r: clamp01(source.r, 1),
    g: clamp01(source.g, 1),
    b: clamp01(source.b, 1),
    a: clamp01(source.a, 1),
  };
}

function normalizeHotkey(value) {
  const source = value && typeof value === 'object' ? value : {};
  const mode = source.mode === 'toggle' || source.mode === 'always' ? source.mode : 'hold';
  return {
    key: Number(source.key) || 0,
    mode: mode,
    active: source.active === true,
  };
}

function normalizeOptions(options) {
  if (!Array.isArray(options)) return [];
  return options.map(function (option) {
    if (option && typeof option === 'object') {
      const id = option.id !== undefined ? String(option.id) : String(option.label || '');
      return { id: id, label: String(option.label !== undefined ? option.label : id) };
    }
    const id = String(option);
    return { id: id, label: id };
  }).filter(function (option) { return option.id.length > 0; });
}

function normalizeSelection(value, options) {
  const ids = new Set(options.map(function (option) { return option.id; }));
  if (Array.isArray(value)) {
    return value.map(String).filter(function (id, index, all) {
      return ids.has(id) && all.indexOf(id) === index;
    });
  }
  return [];
}

function normalizeItemValue(kind, value, extra) {
  if (kind === 'checkbox') return value === true;
  if (kind === 'slider') {
    const number = Number(value);
    return Number.isFinite(number) ? number : Number(extra && extra.min) || 0;
  }
  if (kind === 'combo') return Number.isInteger(value) ? value : 0;
  if (kind === 'input') return value === undefined || value === null ? '' : String(value);
  if (kind === 'color_picker') return normalizeColor(value);
  if (kind === 'multi_select') return normalizeSelection(value, extra && extra.options ? extra.options : []);
  if (kind === 'hotkey') return normalizeHotkey(value);
  if (kind === 'label') return String(value || '');
  return value;
}

function createNctApi(scriptName, resources) {
  let nextMenuId = 0;

  function registerItem(groupName, kind, label, value, extra) {
    const options = normalizeOptions(extra && extra.options);
    const id = (extra && extra.id) || normalizeMenuId(groupName + '_' + label) + '_' + (++nextMenuId);
    const state = {
      id: id,
      kind: kind,
      group: String(groupName || ''),
      label: String(label || id),
      value: normalizeItemValue(kind, value, { min: extra && extra.min, options: options }),
      visible: !extra || extra.visible !== false,
      disabled: extra && extra.disabled === true,
      tooltip: extra && extra.tooltip ? String(extra.tooltip) : '',
      parent: extra && extra.parent ? String(extra.parent) : '',
      options: options,
      min: extra && Number.isFinite(extra.min) ? extra.min : undefined,
      max: extra && Number.isFinite(extra.max) ? extra.max : undefined,
    };
    let callback = null;
    const handle = {
      get: function () { return state.value; },
      set: function (nextValue) {
        state.value = normalizeItemValue(kind, nextValue, { min: state.min, options: state.options || [] });
        sendBridgeMessage({ type: 'MENU_UPDATE', script: scriptName, id: id, patch: { value: state.value } });
        return handle;
      },
      setVisible: function (visible) {
        state.visible = visible === true;
        sendBridgeMessage({ type: 'MENU_UPDATE', script: scriptName, id: id, patch: { visible: state.visible } });
        return handle;
      },
      setDisabled: function (disabled) {
        state.disabled = disabled === true;
        sendBridgeMessage({ type: 'MENU_UPDATE', script: scriptName, id: id, patch: { disabled: state.disabled } });
        return handle;
      },
      setTooltip: function (text) {
        state.tooltip = text === undefined || text === null ? '' : String(text);
        sendBridgeMessage({ type: 'MENU_UPDATE', script: scriptName, id: id, patch: { tooltip: state.tooltip } });
        return handle;
      },
      set_callback: function (nextCallback, forceCall) {
        callback = typeof nextCallback === 'function' ? nextCallback : null;
        if (callback && forceCall === true) {
          try { callback(state.value); } catch (e) {}
        }
        return handle;
      },
      unset_callback: function () {
        callback = null;
        return handle;
      },
      color_picker: function (label, initial) {
        return registerItem(groupName, 'color_picker', label || 'color', initial || { r: 1, g: 1, b: 1, a: 1 }, { parent: id });
      },
      remove: function () {
        menuCallbacks.delete(menuKey(scriptName, id));
        sendBridgeMessage({ type: 'MENU_REMOVE', script: scriptName, id: id });
      },
    };
    menuCallbacks.set(menuKey(scriptName, id), {
      state: state,
      call: function (nextValue) {
        state.value = normalizeItemValue(kind, nextValue, { min: state.min, options: state.options || [] });
        if (callback) {
          try { callback(state.value); } catch (e) {}
        }
      },
    });
    resources.menuItems.push(id);
    sendBridgeMessage({ type: 'MENU_REGISTER', script: scriptName, item: state });
    return handle;
  }

  function createBuiltinHandle(item) {
    if (!item || !item.id) return null;
    const script = '__builtin';
    const id = String(item.id);
    const kind = String(item.kind || '');
    const state = {
      id: id,
      kind: kind,
      label: String(item.label || id),
      value: item.value,
      visible: item.visible !== false,
      disabled: item.disabled === true,
      tooltip: item.tooltip ? String(item.tooltip) : '',
      options: normalizeOptions(item.options || []),
    };
    let callback = null;
    const handle = {
      get: function () { return state.value; },
      set: function (nextValue) {
        state.value = normalizeItemValue(kind, nextValue, { options: state.options || [] });
        sendBridgeMessage({ type: 'MENU_UPDATE', script: script, id: id, patch: { value: state.value } });
        return handle;
      },
      setVisible: function (visible) {
        state.visible = visible === true;
        sendBridgeMessage({ type: 'MENU_UPDATE', script: script, id: id, patch: { visible: state.visible } });
        return handle;
      },
      setDisabled: function (disabled) {
        state.disabled = disabled === true;
        sendBridgeMessage({ type: 'MENU_UPDATE', script: script, id: id, patch: { disabled: state.disabled } });
        return handle;
      },
      setTooltip: function (text) {
        state.tooltip = text === undefined || text === null ? '' : String(text);
        sendBridgeMessage({ type: 'MENU_UPDATE', script: script, id: id, patch: { tooltip: state.tooltip } });
        return handle;
      },
      set_callback: function (nextCallback, forceCall) {
        callback = typeof nextCallback === 'function' ? nextCallback : null;
        if (callback && forceCall === true) {
          try { callback(state.value); } catch (e) {}
        }
        return handle;
      },
      unset_callback: function () {
        callback = null;
        return handle;
      },
      color_picker: function () { return null; },
      remove: function () {
        return handle;
      },
    };
    menuCallbacks.set(menuKey(script, id), {
      state: state,
      call: function (nextValue) {
        state.value = normalizeItemValue(kind, nextValue, { options: state.options || [] });
        if (callback) {
          try { callback(state.value); } catch (e) {}
        }
      },
    });
    return handle;
  }

  function group(groupName) {
    return {
      checkbox: function (label, initial, options) { return registerItem(groupName, 'checkbox', label, initial === true, options || {}); },
      slider: function (label, min, max, initial, options) {
        const minValue = Number(min);
        const maxValue = Number(max);
        const initialValue = Number.isFinite(Number(initial)) ? Number(initial) : minValue;
        const extra = options || {};
        extra.min = minValue;
        extra.max = maxValue;
        return registerItem(groupName, 'slider', label, initialValue, extra);
      },
      combo: function (label, options, initial, itemOptions) { return registerItem(groupName, 'combo', label, Number.isInteger(initial) ? initial : 0, Object.assign({}, itemOptions || {}, { options: options || [] })); },
      input: function (label, initial, options) { return registerItem(groupName, 'input', label, initial || '', options || {}); },
      color_picker: function (label, initial, options) { return registerItem(groupName, 'color_picker', label, normalizeColor(initial), options || {}); },
      multi_select: function (label, options, initial, itemOptions) { return registerItem(groupName, 'multi_select', label, initial || [], Object.assign({}, itemOptions || {}, { options: options || [] })); },
      button: function (label, options) { return registerItem(groupName, 'button', label, false, options || {}); },
      hotkey: function (label, initial, options) { return registerItem(groupName, 'hotkey', label, normalizeHotkey(initial), options || {}); },
      label: function (label, options) { return registerItem(groupName, 'label', label, String(label || ''), options || {}); },
    };
  }

  function find(tab, child, control) {
    const key = [tab, child, control].map(function (part) {
      return normalizeMenuId(part);
    }).join('/');
    const item = menuSnapshots.get(key);
    return item ? createBuiltinHandle(item) : null;
  }

  return { ui: { group: group, find: find } };
}

function applyMenuSnapshot(message) {
  menuSnapshots.clear();
  const items = Array.isArray(message.items) ? message.items : [];
  for (let i = 0; i < items.length; i++) {
    const item = items[i];
    if (!item || !Array.isArray(item.path) || item.path.length < 3) continue;
    const key = item.path.slice(0, 3).map(function (part) {
      return normalizeMenuId(part);
    }).join('/');
    menuSnapshots.set(key, item);
  }
}

function dispatchMenuEvent(message) {
  const key = menuKey(message.script || '', message.id || '');
  const entry = menuCallbacks.get(key);
  if (!entry) return;
  entry.call(message.value);
}

function executeScript(message) {
  lockPayloadServerInfo();
  const name = message.name || 'script';
  const code = message.code || '';
  if (scriptCleanups.has(name)) {
    try { scriptCleanups.get(name)(); } catch (e) {}
    scriptCleanups.delete(name);
  }
  const resources = { handlers: [], intervals: [], timeouts: [], everyTicks: [], onUnloadCallbacks: [], menuItems: [] };
  const altProxy = new Proxy(alt, {
    get(target, prop) {
      if (prop === 'on') return function (eventName, handler) {
        resources.handlers.push({ eventName: eventName, handler: handler });
        return target.on(eventName, handler);
      };
      if (prop === 'setInterval') return function (handler, ms) {
        const id = target.setInterval(handler, ms);
        resources.intervals.push(id);
        return id;
      };
      if (prop === 'setTimeout') return function (handler, ms) {
        const id = target.setTimeout(handler, ms);
        resources.timeouts.push(id);
        return id;
      };
      if (prop === 'everyTick') return function (handler) {
        const id = target.everyTick(handler);
        resources.everyTicks.push(id);
        return id;
      };
      return target[prop];
    },
  });
  const onUnload = function (fn) {
    if (typeof fn === 'function') resources.onUnloadCallbacks.push(fn);
  };
  const nct = createNctApi(name, resources);
  const runner = new Function('alt', 'native', 'onUnload', 'nct', code);
  runner(altProxy, native, onUnload, nct);
  scriptCleanups.set(name, function () {
    resources.onUnloadCallbacks.forEach(function (cb) { try { cb(); } catch (e) {} });
    resources.handlers.forEach(function (h) { alt.off(h.eventName, h.handler); });
    resources.intervals.forEach(function (id) { alt.clearInterval(id); });
    resources.timeouts.forEach(function (id) { alt.clearTimeout(id); });
    resources.everyTicks.forEach(function (id) { alt.clearEveryTick(id); });
    resources.menuItems.forEach(function (id) { menuCallbacks.delete(menuKey(name, id)); });
    sendBridgeMessage({ type: 'MENU_REMOVE', script: name });
  });
}

function unloadScript(name) {
  if (scriptCleanups.has(name)) {
    try { scriptCleanups.get(name)(); } catch (e) {}
    scriptCleanups.delete(name);
  }
}

function collectAltvPlayers() {
  const result = [];
  try {
    if (typeof alt === 'undefined' || !alt.Player) return result;
    const localPlayer = alt.Player.local || null;
    const streamed = Array.isArray(alt.Player.streamedIn) ? alt.Player.streamedIn : [];

    // Используем правильное API для сбора данных игроков
    if (Array.isArray(streamed)) {
    for (let i = 0; i < streamed.length; i++) {
        const player = streamed[i];
        if (player && typeof player === 'object') {
          try {
            let login = '';
            let metaId = null;
            let level = 0;
            let gender = '';
            let member = 0;
            try {
              if (typeof player.getStreamSyncedMeta === 'function') {
                // prefer stream-synced meta when available
                login = player.getStreamSyncedMeta('login') || '';
                metaId = player.getStreamSyncedMeta('id');
                level = player.getStreamSyncedMeta('level') || level;
                const g = player.getStreamSyncedMeta('gender');
                if (g !== undefined && g !== null && g !== '') {
                  if (typeof g === 'number') gender = (g === 0) ? 'Male' : 'Female';
                  else gender = String(g);
                }
                const m = player.getStreamSyncedMeta('member');
                if (Number.isFinite(Number(m))) member = Number(m);
              }
            } catch (e) {}

            const staticIdOut = (Number.isFinite(Number(metaId)) ? Number(metaId) : (Number.isFinite(Number(player.staticId)) ? player.staticId : -1));

            const outName = (bridgeConfig.altv_nickname) ? (login || player.name || 'Unknown') : (player.name || (login || 'Unknown'));
            result.push({
              handle: player.scriptID || 0,
              name: outName,
              login: String(login || ''),
              pos: player.pos || { x: 0, y: 0, z: 0 },
              health: player.health || 100,
              armor: player.armour || 0,
              staticId: staticIdOut,
              netId: player.netId || player.netID || -1,
              level: Number.isFinite(Number(level)) ? Number(level) : 0,
              gender: String(gender || ''),
              member: Number.isFinite(Number(member)) ? Number(member) : 0,
            });
          } catch (e) {
            result.push({
              handle: player.scriptID || 0,
              name: player.name || 'Unknown',
              pos: player.pos || { x: 0, y: 0, z: 0 },
              health: player.health || 100,
              armor: player.armour || 0,
              staticId: player.staticId || -1,
              netId: player.netID || -1,
            });
          }
        }
      }
    }
  } catch (e) {
    try { alt.logError('[collectAltvPlayers] ' + e.message); } catch (_) {}
  }
  return result;
}

function connect() {
  if (unloading) return;
  clearReconnect();
  stopSocket();
  socket = new alt.WebSocketClient(WS_URL);
  socket.on('open', function () {
    configReceived = false;
    sendBridgeHello();
    helloTimer = alt.setInterval(function () {
      if (!configReceived) sendBridgeHello();
    }, 1000);
    sendPayloadServerInfo();
    // Отправляем данные об игроках каждые 100ms
    alt.setInterval(function () {
      try {
        const players = collectAltvPlayers();
        if (players.length > 0 || configReceived) {
          try {
            // Отправляем бинарный пакет ESP (полный формат)
            const buf = buildBinaryPlayers(players);
            if (buf) {
              try {
                // socket.send может принимать ArrayBuffer
                if (socket && typeof socket.send === 'function') {
                  socket.send(buf);
                }
                try { alt.log('[ESP_SEND] players=' + players.length + ' bytes=' + (buf.byteLength || 0)); } catch (_) {}
              } catch (e) {
                try { alt.logError('[ESP_SEND] send binary failed: ' + e.message); } catch (_) {}
              }
            }
          } catch (e) {
            try { alt.logError('[ESP_BUILD] ' + e.message); } catch (_) {}
          }

          // Также отправляем краткий JSON-дебаг для удобства
          try {
            // build more detailed debug sample directly from streamedIn players (includes stream-synced meta if available)
            let sample = [];
            try {
              const streamed = Array.isArray(alt.Player.streamedIn) ? alt.Player.streamedIn : [];
              for (let i = 0; i < Math.min(6, streamed.length); i++) {
                const pl = streamed[i];
                if (!pl || typeof pl !== 'object') continue;
                let login = '';
                let metaId = null;
                let level = null;
                let gender = '';
                let member = null;
                try {
                  if (typeof pl.getStreamSyncedMeta === 'function') {
                    login = pl.getStreamSyncedMeta('login') || '';
                    metaId = pl.getStreamSyncedMeta('id');
                    level = pl.getStreamSyncedMeta('level');
                    gender = pl.getStreamSyncedMeta('gender');
                    member = pl.getStreamSyncedMeta('member');
                  }
                } catch (e) {}

                // respect bridge-config: if altv_nickname is enabled prefer login as displayed name
                const displayName = (bridgeConfig.altv_nickname) ? (login || pl.name || 'Unknown') : (pl.name || (login || 'Unknown'));

                sample.push({ handle: pl.scriptID || 0, name: displayName, login: login, id: metaId, level: level, gender: gender, member: member, static: (Number.isFinite(Number(metaId)) ? Number(metaId) : (Number.isFinite(Number(pl.staticId)) ? pl.staticId : -1)) });
              }
              if (sample.length === 0) sample = players.slice(0, 6).map(function (p) { return { handle: p.handle, name: (bridgeConfig.altv_nickname ? (p.login || p.name) : (p.name || p.login)), static: p.staticId }; });
            } catch (e) {
              sample = players.slice(0, 6).map(function (p) { return { handle: p.handle, name: (bridgeConfig.altv_nickname ? (p.login || p.name) : (p.name || p.login)), static: p.staticId }; });
            }
            sendBridgeMessage({ type: 'DEBUG_PLAYERS', count: players.length, sample: sample });
          } catch (e) {}
        }
      } catch (e) {}
    }, 100);
  });
  socket.on('message', function (raw) {
    try {
      const message = JSON.parse(raw);
      if (message.type === 'CONFIG') applyConfig(message);
      else if (message.type === 'EXECUTE_JS') executeScript(message);
      else if (message.type === 'UNLOAD_JS') unloadScript(message.name || 'script');
      else if (message.type === 'MENU_SNAPSHOT') applyMenuSnapshot(message);
      else if (message.type === 'MENU_EVENT') dispatchMenuEvent(message);
    } catch (e) {}
  });
  socket.on('close', function () {
    stopSocket();
    scheduleReconnect();
  });
  socket.on('error', function () {
    stopSocket();
    scheduleReconnect();
  });
  socket.start();
}

function unloadBridge() {
  unloading = true;
  clearReconnect();
  scriptCleanups.forEach(function (cleanup) { try { cleanup(); } catch (e) {} });
  scriptCleanups.clear();
  stopSocket();
}

globalThis[UNLOAD_KEY] = unloadBridge;
connect();
})();


/* ==========================================================================
   NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
   Inert per-file identifier required by the EULA. Uniquely tags this file so
   this program is distinguishable from any other build. No functional logic.
   file: src/executor/payload.js
   ========================================================================== */
var __noctua_license_0cbc119f830ebeb9154adb80089a9a51 = { id: "d6892e5c-1e5b-a96b-90c9-cebb005d1651", file: "src/executor/payload.js", tag: [ 229, 1, 145, 112, 209, 234, 231, 180, 24, 47, 111, 68, 211, 95, 57, 188 ] };
void __noctua_license_0cbc119f830ebeb9154adb80089a9a51;
