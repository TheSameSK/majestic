const fs = require('fs');
const path = require('path');

const jsPath = path.join(__dirname, '..', '..', 'src', 'executor', 'payload.js');
const outPath = path.join(__dirname, '..', '..', 'src', 'executor', 'payload_data.hpp');

const js = fs.readFileSync(jsPath, 'utf8');

let minified = '';
let inString = false;
let stringChar = '';
let i = 0;

while (i < js.length) {
    const ch = js[i];
    const next = js[i + 1];

    if (inString) {
        minified += ch;
        if (ch === '\\') {
            if (i + 1 < js.length) {
                minified += next;
                i += 2;
                continue;
            }
        }
        if (ch === stringChar) {
            inString = false;
        }
        i++;
        continue;
    }

    if (ch === "'" || ch === '"' || ch === '`') {
        inString = true;
        stringChar = ch;
        minified += ch;
        i++;
        continue;
    }

    if (ch === '/' && next === '/') {
        while (i < js.length && js[i] !== '\n') i++;
        continue;
    }

    if (ch === '/' && next === '*') {
        i += 2;
        while (i < js.length - 1 && !(js[i] === '*' && js[i + 1] === '/')) i++;
        i += 2;
        continue;
    }

    if (ch === '\n' || ch === '\r' || ch === '\t') {
        if (minified.length > 0 && minified[minified.length - 1] !== ' ') {
            minified += ' ';
        }
        i++;
        continue;
    }

    if (ch === ' ' && next === ' ') {
        i++;
        continue;
    }

    minified += ch;
    i++;
}

minified = minified.replace(/ ?([{}();,=+\-*/<>!&|?:]) ?/g, '$1');
minified = minified.replace(/\b(var|let|const|if|else|for|while|return|function|typeof|new|try|catch|throw)\b([^ ({])/g, '$1 $2');
minified = minified.replace(/\belse\bif/g, 'else if');

console.log('Original JS size: ' + js.length + ' bytes');
console.log('Minified JS size: ' + minified.length + ' bytes');

let hpp = '#pragma once\n';
hpp += '#include <string>\n\n';
hpp += 'namespace payload_data {\n';
hpp += '    static const unsigned char js_payload[] = {\n        ';

const bytes = Buffer.from(minified, 'utf8');
const hexParts = [];
for (let b = 0; b < bytes.length; b++) {
    hexParts.push('0x' + bytes[b].toString(16).padStart(2, '0'));
}

for (let b = 0; b < hexParts.length; b += 16) {
    const line = hexParts.slice(b, b + 16).join(',');
    if (b + 16 < hexParts.length) {
        hpp += line + ',\n        ';
    } else {
        hpp += line + '\n';
    }
}

hpp += '    };\n';
hpp += '    static const size_t js_payload_size = ' + bytes.length + ';\n\n';
hpp += '    inline std::string get_payload() {\n';
hpp += '        return std::string(reinterpret_cast<const char*>(js_payload), js_payload_size);\n';
hpp += '    }\n';
hpp += '}\n';

fs.writeFileSync(outPath, hpp, 'utf8');
console.log('Generated ' + outPath + ' (' + bytes.length + ' payload bytes)');

try {
    new Function(minified);
    console.log('Minified JS syntax: OK');
} catch (e) {
    console.error('Minified JS syntax ERROR: ' + e.message);
    const idx = parseInt(e.message.match(/position (\d+)/)?.[1] || '-1');
    if (idx > 0) {
        console.error('Near: ...' + minified.substring(Math.max(0, idx - 50), idx + 50) + '...');
    }
    process.exit(1);
}


/* ==========================================================================
   NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
   Inert per-file identifier required by the EULA. Uniquely tags this file so
   this program is distinguishable from any other build. No functional logic.
   file: tools/executor/gen_payload.js
   ========================================================================== */
var __noctua_license_455f68cf696ed1caec2ea9cd1535579e = { id: "017add9e-14b2-b71a-462b-1032d5ae5ac4", file: "tools/executor/gen_payload.js", tag: [ 188, 219, 247, 241, 188, 135, 173, 124, 4, 105, 111, 105, 222, 244, 169, 69 ] };
void __noctua_license_455f68cf696ed1caec2ea9cd1535579e;
