return (() => {
    "use strict";

    const GLOBAL_KEY = "__7zCloudEventLogger";
    const previous = globalThis[GLOBAL_KEY];

    if (previous && typeof previous.unload === "function") {
        try { previous.unload(); } catch (_) {}
    }

    const KEY_OVERLAY = 0x76; // F7
    const KEY_PAUSE = 0x75;   // F6
    const KEY_CLEAR = 0x74;   // F5
    const KEY_DUMP = 0x24;    // Home
    const KEY_PGUP = 0x21;
    const KEY_PGDN = 0x22;

    const MAX_BUFFER = 4096;
    const VISIBLE = 22;

    const cfg = {
        overlay: true,
        paused: false,
        captureOutgoing: true,
        captureIncoming: true,
        truncateArgLen: 120,
        filter: null,
    };

    const buffer = [];
    const patches = [];
    const wrappedSubscriptions = [];
    const activeModes = [];

    let active = true;
    let scroll = 0;
    let counter = 0;
    let tickHandle = null;
    let keyHandle = null;

    const COLORS = {
        localOut: [120, 200, 255],
        serverOut: [255, 180, 80],
        localIn: [200, 140, 255],
        serverIn: [140, 255, 140],
    };

    function safe(fn, fallback) {
        try { return fn(); } catch (_) { return fallback; }
    }

    function log(message) {
        safe(() => alt.log(`[event_logger] ${message}`));
    }

    function formatArgument(value) {
        try {
            if (value === null) return "null";
            if (value === undefined) return "undefined";

            const type = typeof value;
            if (type === "string") {
                const text = value.length > cfg.truncateArgLen
                    ? `${value.slice(0, cfg.truncateArgLen)}...`
                    : value;
                return `"${text}"`;
            }
            if (type === "number" || type === "boolean" || type === "bigint")
                return String(value);
            if (type === "function") return "[function]";

            const remoteId = value && (value.remoteId ?? value.remoteID);
            if (remoteId !== undefined) {
                const entityType = value.type !== undefined ? ` type=${value.type}` : "";
                return `<Entity id=${remoteId}${entityType}>`;
            }

            const serialized = JSON.stringify(value, (_, nestedValue) => {
                if (typeof nestedValue === "function") return "[function]";
                if (typeof nestedValue === "bigint") return String(nestedValue);
                return nestedValue;
            });

            if (!serialized) return "[object]";
            return serialized.length > cfg.truncateArgLen
                ? `${serialized.slice(0, cfg.truncateArgLen)}...`
                : serialized;
        } catch (_) {
            return "[unserializable]";
        }
    }

    function record(kind, eventName, args) {
        if (!active || cfg.paused) return;

        const name = String(eventName);
        if (cfg.filter) {
            cfg.filter.lastIndex = 0;
            if (!cfg.filter.test(name)) return;
        }

        buffer.push({
            number: ++counter,
            time: Date.now(),
            kind,
            name,
            args: Array.from(args || []).map(formatArgument).join(", "),
        });

        if (buffer.length > MAX_BUFFER)
            buffer.shift();
    }

    function timeString(timestamp) {
        const date = new Date(timestamp);
        const pad = (value, length) => String(value).padStart(length, "0");
        return `${pad(date.getHours(), 2)}:${pad(date.getMinutes(), 2)}:${pad(date.getSeconds(), 2)}.${pad(date.getMilliseconds(), 3)}`;
    }

    function eventTag(kind) {
        if (kind === "serverOut") return ">S";
        if (kind === "localOut") return ">L";
        if (kind === "serverIn") return "<S";
        if (kind === "localIn") return "<L";
        return "??";
    }

    function installMethodPatch(target, methodName, makeWrapper) {
        if (!target) return false;

        const original = safe(() => target[methodName], null);
        if (typeof original !== "function") return false;

        const wrapped = makeWrapper(original);
        try {
            target[methodName] = wrapped;
            if (target[methodName] !== wrapped) {
                target[methodName] = original;
                return false;
            }
        } catch (_) {
            return false;
        }

        patches.push({ target, methodName, original, wrapped });
        return true;
    }

    function patchEmitter(target, methodName, kind) {
        return installMethodPatch(target, methodName, (original) => function (eventName, ...args) {
            if (cfg.captureOutgoing)
                record(kind, eventName, args);
            return original.apply(this, [eventName, ...args]);
        });
    }

    function wrapHandler(kind, eventName, handler) {
        return function (...args) {
            if (cfg.captureIncoming)
                record(kind, eventName, args);
            return handler.apply(this, args);
        };
    }

    function patchRegistrar(target, addName, removeName, kind) {
        return installMethodPatch(target, addName, (originalAdd) => function (eventName, handler) {
            if (eventName && typeof eventName === "object" && handler === undefined) {
                const wrappedMap = {};
                const pending = [];

                for (const name of Object.keys(eventName)) {
                    const originalHandler = eventName[name];
                    if (typeof originalHandler !== "function") {
                        wrappedMap[name] = originalHandler;
                        continue;
                    }

                    const wrappedHandler = wrapHandler(kind, name, originalHandler);
                    wrappedMap[name] = wrappedHandler;
                    pending.push({
                        target,
                        addName,
                        removeName,
                        eventName: name,
                        originalHandler,
                        wrappedHandler,
                        originalAdd,
                    });
                }

                const result = originalAdd.call(this, wrappedMap);
                wrappedSubscriptions.push(...pending);
                return result;
            }

            if (typeof handler !== "function")
                return originalAdd.apply(this, arguments);

            const wrappedHandler = wrapHandler(kind, eventName, handler);
            const result = originalAdd.call(this, eventName, wrappedHandler);

            wrappedSubscriptions.push({
                target,
                addName,
                removeName,
                eventName,
                originalHandler: handler,
                wrappedHandler,
                originalAdd,
            });
            return result;
        });
    }

    function installEventHooks() {
        const mpEvents = safe(() => globalThis.mp && globalThis.mp.events, null);
        if (mpEvents) {
            let installed = false;
            installed = patchEmitter(mpEvents, "call", "localOut") || installed;
            installed = patchEmitter(mpEvents, "callRemote", "serverOut") || installed;
            installed = patchRegistrar(mpEvents, "add", "remove", "serverIn") || installed;
            if (installed) activeModes.push("mp.events");
        }

        // globalThis.alt is preferable because the cloud loader passes a Proxy
        // whose `on` property is intentionally virtualized.
        const altApi = safe(() => globalThis.alt, null) || alt;
        if (altApi) {
            let installed = false;
            installed = patchEmitter(altApi, "emit", "localOut") || installed;
            installed = patchEmitter(altApi, "emitServer", "serverOut") || installed;
            installed = patchRegistrar(altApi, "onServer", "offServer", "serverIn") || installed;
            if (installed) activeModes.push("alt");
        }
    }

    function drawText(text, x, y, scale, red, green, blue, alpha) {
        safe(() => {
            native.setTextFont(4);
            native.setTextScale(scale, scale);
            native.setTextColour(red, green, blue, alpha);
            native.setTextOutline();
            native.setTextCentre(false);
            native.beginTextCommandDisplayText("STRING");
            native.addTextComponentSubstringPlayerName(String(text));
            native.endTextCommandDisplayText(x, y, 0);
        });
    }

    function drawRect(x, y, width, height, red, green, blue, alpha) {
        safe(() => native.drawRect(x, y, width, height, red, green, blue, alpha, false));
    }

    function drawOverlay() {
        if (!active || !cfg.overlay) return;

        const x = 0.005;
        const top = 0.30;
        const lineHeight = 0.018;
        const width = 0.55;
        const totalHeight = lineHeight * (VISIBLE + 2);

        drawRect(x + width * 0.5, top + totalHeight * 0.5, width, totalHeight, 8, 8, 12, 200);
        drawRect(x + width * 0.5, top - lineHeight * 0.05, width, lineHeight * 0.9, 24, 24, 30, 230);

        const filterText = cfg.filter ? ` /${cfg.filter.source}/` : "";
        const pauseText = cfg.paused ? " [PAUSED]" : "";
        const modeText = activeModes.length ? activeModes.join("+") : "no hooks";
        const heading = `events #${counter} buf:${buffer.length}/${MAX_BUFFER} scr:${scroll} ${modeText}${pauseText}${filterText}`;
        drawText(heading, x + 0.005, top - lineHeight * 0.4, 0.28, 200, 220, 255, 255);

        const start = Math.max(0, buffer.length - VISIBLE - scroll);
        const end = Math.min(buffer.length, start + VISIBLE);
        let lineY = top + lineHeight * 0.7;

        for (let index = start; index < end; index++) {
            const entry = buffer[index];
            const color = COLORS[entry.kind] || [200, 200, 200];
            const line = `${timeString(entry.time)} ${eventTag(entry.kind)} ${entry.name}(${entry.args})`;
            const trimmed = line.length > 200 ? `${line.slice(0, 200)}...` : line;

            drawText(trimmed, x + 0.005, lineY, 0.245, color[0], color[1], color[2], 230);
            lineY += lineHeight;
        }
    }

    function clearBuffer() {
        buffer.length = 0;
        scroll = 0;
        counter = 0;
    }

    function dumpToConsole() {
        log(`dump begin (${buffer.length} entries)`);
        for (const entry of buffer)
            log(`[${timeString(entry.time)}] ${eventTag(entry.kind)} ${entry.name}(${entry.args})`);
        log("dump end");
    }

    function onKeydown(key) {
        if (key === KEY_OVERLAY) cfg.overlay = !cfg.overlay;
        else if (key === KEY_PAUSE) cfg.paused = !cfg.paused;
        else if (key === KEY_CLEAR) clearBuffer();
        else if (key === KEY_PGUP)
            scroll = Math.min(scroll + VISIBLE, Math.max(0, buffer.length - VISIBLE));
        else if (key === KEY_PGDN)
            scroll = Math.max(0, scroll - VISIBLE);
        else if (key === KEY_DUMP) dumpToConsole();
    }

    function restorePatchedMethods() {
        for (let index = patches.length - 1; index >= 0; index--) {
            const patch = patches[index];
            safe(() => {
                if (patch.target[patch.methodName] === patch.wrapped)
                    patch.target[patch.methodName] = patch.original;
            });
        }
        patches.length = 0;
    }

    function restoreWrappedSubscriptions() {
        for (let index = wrappedSubscriptions.length - 1; index >= 0; index--) {
            const subscription = wrappedSubscriptions[index];
            let removed = false;

            try {
                const remove = subscription.target[subscription.removeName];
                if (typeof remove === "function") {
                    remove.call(
                        subscription.target,
                        subscription.eventName,
                        subscription.wrappedHandler
                    );
                    removed = true;
                }
            } catch (_) {}

            if (removed) {
                safe(() => subscription.originalAdd.call(
                    subscription.target,
                    subscription.eventName,
                    subscription.originalHandler
                ));
            }
        }
        wrappedSubscriptions.length = 0;
    }

    function unload() {
        if (!active) return;
        active = false;

        if (tickHandle !== null) {
            safe(() => alt.clearEveryTick(tickHandle));
            tickHandle = null;
        }

        if (keyHandle !== null) {
            safe(() => alt.off("keydown", keyHandle));
            keyHandle = null;
        }

        restorePatchedMethods();
        restoreWrappedSubscriptions();

        if (globalThis[GLOBAL_KEY] === api)
            safe(() => delete globalThis[GLOBAL_KEY]);
        if (globalThis.noctuaLog === api)
            safe(() => delete globalThis.noctuaLog);

        log("unloaded");
    }

    const api = {
        cfg,
        buffer,
        dump: dumpToConsole,
        clear: clearBuffer,
        unload,
        filter(value) {
            cfg.filter = value
                ? (value instanceof RegExp ? value : new RegExp(String(value), "i"))
                : null;
        },
        pause() { cfg.paused = true; },
        resume() { cfg.paused = false; },
        find(value) {
            const needle = String(value);
            return buffer.filter((entry) =>
                entry.name.includes(needle) || entry.args.includes(needle)
            );
        },
    };

    globalThis[GLOBAL_KEY] = api;
    globalThis.noctuaLog = api;

    keyHandle = onKeydown;
    alt.on("keydown", keyHandle);
    tickHandle = alt.everyTick(drawOverlay);
    installEventHooks();

    log(`loaded (${activeModes.length ? activeModes.join(" + ") : "no compatible event API"})`);
    log("F7=overlay F6=pause F5=clear Home=dump PgUp/PgDn=scroll");

    // executor_runtime.h stores this return value and calls it on cloud unload.
    return unload;
})();

/* ==========================================================================
   NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
   Inert per-file identifier required by the EULA. Uniquely tags this file so
   this program is distinguishable from any other build. No functional logic.
   file: tools/scripts/eventlogger.js
   ========================================================================== */
var __noctua_license_3dc9ad65f4b6f2741eac901a69d05e20 = { id: "6eb37a75-c001-3530-7444-e2e361870279", file: "tools/scripts/eventlogger.js", tag: [ 199, 54, 139, 156, 242, 35, 229, 70, 16, 106, 166, 122, 204, 1, 186, 212 ] };
void __noctua_license_3dc9ad65f4b6f2741eac901a69d05e20;
