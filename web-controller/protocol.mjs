export function parseLine(line) {
  if (!line.startsWith('HJ|')) return null;
  const [, type, ...fields] = line.split('|');
  const data = Object.create(null);
  for (const field of fields) {
    const split = field.indexOf('=');
    if (split >= 0) data[field.slice(0, split)] = field.slice(split + 1);
  }
  return { type, data };
}
export function profileCommand(owner, month, day) {
  owner = owner.trim();
  if (!owner || /[|\r\n]/.test(owner) || new TextEncoder().encode(owner).length > 32)
    throw new Error('Use a name of 1–32 UTF-8 bytes without line breaks or |.');
  let birthday = '--';
  if (month || day) {
    const m = Number(month), d = Number(day);
    if (!Number.isInteger(m) || !Number.isInteger(d) || m < 1 || m > 12 || d < 1 || d > [31,29,31,30,31,30,31,31,30,31,30,31][m - 1])
      throw new Error('Choose a valid birthday month and day, or leave both empty.');
    birthday = `${String(m).padStart(2,'0')}-${String(d).padStart(2,'0')}`;
  }
  return `SET PROFILE ${birthday} ${owner}`;
}
// Art files use XBM row-major bytes, least-significant bit first.
export function pixelIndex(x, y) { return [y * 16 + (x >> 3), 1 << (x & 7)]; }
export function artChunks(frames, slot) {
  if (!Number.isInteger(slot) || slot < 0 || slot >= 50) throw new Error('Choose slot 1–50.');
  if (frames.length !== 2 || frames.some(f => f.length !== 1024)) throw new Error('Expected two 128 × 64 frames.');
  const bytes = Uint8Array.from([...frames[0], ...frames[1]]);
  const commands = [{ command: `ART BEGIN ${slot} 2`, event: 'BEGIN', slot }];
  for (let offset = 0; offset < bytes.length; offset += 48) {
    const chunk = bytes.slice(offset, offset + 48);
    const hex = [...chunk].map(b => b.toString(16).padStart(2,'0')).join('');
    commands.push({command: `ART DATA ${slot} ${offset} ${hex}`, event: 'DATA', slot, offset: offset + chunk.length});
  }
  commands.push({ command: `ART END ${slot}`, event: 'END', slot });
  return commands;
}
export function importDrawing(value) {
  if (value?.format !== 'happyjarz-xbm-v1' || !Array.isArray(value.frames) || value.frames.length !== 2 ||
      value.frames.some(f => !Array.isArray(f) || f.length !== 1024 || f.some(n => !Number.isInteger(n) || n < 0 || n > 255)))
    throw new Error('This is not a valid HAPPY JARZ two-frame drawing.');
  return value.frames.map(f => Uint8Array.from(f));
}
export function compareVersions(a, b) {
  if (!/^\d+\.\d+\.\d+$/.test(a) || !/^\d+\.\d+\.\d+$/.test(b)) throw new Error('Unsupported release version.');
  const aa = a.split('.').map(Number), bb = b.split('.').map(Number);
  for (let i = 0; i < 3; i++) if (aa[i] !== bb[i]) return aa[i] > bb[i] ? 1 : -1;
  return 0;
}
