import { compareVersions } from '../protocol.mjs';

// Application-only OTA via Web Serial. A release is never installable until
// manufacturing signs off on this exact app partition and publishes hashes.
// No full-chip erase, bootloader or filesystem partition writes.
const MODEL = 'happyjarz-supermini-simple-4mb';
const LAYOUT = 'arduino-4mb-default-app-0x10000-0x140000';
const APP_START = 0x10000;
const APP_MAX = 0x140000;
const FLASH_SIZE = 0x400000;
const TOOL_URL = 'https://unpkg.com/esptool-js@0.7.1/bundle.js';

export function validRelease(r) {
  if (!r || r.channel !== 'stable' || r.model !== MODEL ||
      r.hw !== 'V1' || r.chip !== 'ESP32-S3' ||
      r.layout !== LAYOUT || r.flashSize !== 4194304 ||
      !/^\d+\.\d+\.\d+$/.test(r.version) ||
      !Array.isArray(r.images) || r.images.length !== 1) return false;
  const image = r.images[0];
  return image.address === APP_START && Number.isInteger(image.size) &&
    image.size > 0 && image.size <= APP_MAX &&
    image.address + image.size <= FLASH_SIZE &&
    /^[a-f0-9]{64}$/i.test(image.sha256 || '') &&
    typeof image.url === 'string' && image.url.length > 0;
}

async function getVerifiedImage(image) {
  // Prevent arbitrary off-site binaries from being installed by the site.
  const url = new URL(image.url, new URL('../', import.meta.url));
  if (url.origin !== location.origin || !url.pathname.endsWith('.bin'))
    throw Error('Release binary must be a same-origin .bin file.');
  const response = await fetch(url.href, {cache:'no-store', signal:AbortSignal.timeout(30000)});
  if (!response.ok) throw Error('Firmware download failed (HTTP ' + response.status + ').');
  const bytes = new Uint8Array(await response.arrayBuffer());
  if (bytes.length !== image.size) throw Error('Firmware size does not match manifest.');
  const digest = [...new Uint8Array(await crypto.subtle.digest('SHA-256', bytes))]
    .map(v => v.toString(16).padStart(2, '0')).join('');
  if (digest !== image.sha256.toLowerCase()) throw Error('Firmware SHA-256 mismatch; installation cancelled.');
  return bytes;
}

export function init(ctx) {
  const { $, log, message } = ctx;
  const installed = $('update-installed');
  const status = $('release-status');
  const button = $('install-firmware');
  const check = $('check-updates');
  const confirmation = $('update-confirm');
  const progress = $('update-progress');
  const progressLabel = $('update-progress-label');
  let catalog = null, selected = null, running = false, awaitingVersion = null;

  function progressText(text, percent) {
    progressLabel.textContent = text;
    if (percent !== undefined) {progress.hidden = false; progress.value = percent;}
    log(text, 'info', 'FIRMWARE');
  }

  function state() {
    const device = ctx.identity;
    installed.textContent = device
      ? 'Connected firmware: ' + (device.fw || 'unknown') + ' · Hardware: ' + (device.hw || 'unknown')
      : 'Connect your jar to read its installed firmware.';
    selected = null;
    if (running) return;
    const releases = (catalog?.releases || []).filter(validRelease)
      .sort((a,b) => compareVersions(b.version,a.version));
    const latest = releases[0];
    if (!latest) {
      status.textContent = 'No approved firmware download published yet. The jar can continue running normally.';
    } else if (!device) {
      status.textContent = 'Latest approved release: ' + latest.version + '. Connect your jar to check compatibility.';
    } else if (device.hw !== latest.hw || typeof device.fw !== 'string' ||
               !/^\d+\.\d+\.\d+$/.test(device.fw)) {
      status.textContent = 'Cannot verify this board against the release. Installation locked.';
    } else if (compareVersions(latest.version,device.fw) <= 0) {
      status.textContent = 'Installed firmware ' + device.fw + ' is current or newer than published ' + latest.version + '.';
    } else {
      selected = latest;
      status.textContent = 'Approved update available: ' + device.fw + ' → ' + latest.version +
        '. Application partition only. Verify your hardware before proceeding.';
    }
    confirmation.disabled = !selected;
    if (!selected) confirmation.checked = false;
    button.disabled = !selected || !confirmation.checked ||
      !navigator.serial?.requestPort || !crypto?.subtle || !isSecureContext;
  }

  async function checkUpdates() {
    check.disabled = true; status.textContent = 'Checking approved releases…';
    try {
      const response = await fetch('./releases.json', {cache:'no-store', signal:AbortSignal.timeout(10000)});
      if (!response.ok) throw Error('Release catalog unavailable (HTTP ' + response.status + ').');
      const result = await response.json();
      if (result.schema !== 1 || !Array.isArray(result.releases)) throw Error('Invalid release catalog.');
      // Validate all stable entries for our model. Reject partial or corrupt manifests.
      if (result.releases.some(r=>r?.model===MODEL && r?.channel==='stable' && !validRelease(r)))
        throw Error('An approved release has invalid compatibility or checksum metadata.');
      catalog = result;
    } catch(error) {
      catalog = null;
      status.textContent = error.message + ' Installation disabled.';
      log(error.message,'error','FIRMWARE');
    } finally {check.disabled = false; state();}
  }

  async function install() {
    if (running || !selected || !confirmation.checked) return;
    const release = selected;
    const device = ctx.identity;
    if (!device || device.hw !== release.hw || compareVersions(release.version,device.fw) <= 0)
      throw Error('Device changed; reconnect and check releases again.');
    running = true;
    button.disabled = true; confirmation.disabled = true; check.disabled = true;
    ctx.serviceBusy = true; ctx.controls();
    let transport = null, wrote = false;
    try {
      progressText('Downloading and checking firmware hash…',0);
      const image = await getVerifiedImage(release.images[0]);
      progressText('SHA-256 verified. Releasing controller serial port…',0);
      const flashPort = ctx.getPort();
      if (!flashPort) throw Error('Connected USB port was lost before flashing.');
      await ctx.disconnect();
      progressText('Reusing the verified jar USB port for the flasher.',0);
      const selectedPort = flashPort;
      progressText('Loading ESP32-S3 flasher…',0);
      const { Transport, ESPLoader } = await import(TOOL_URL);
      transport = new Transport(selectedPort, true);
      const loader = new ESPLoader({
        transport, baudrate:115200,
        terminal:{clean(){}, writeLine(t){log(t,'info','ESPTOOL');}, write(t){log(t,'info','ESPTOOL');}}
      });
      const chipName = await loader.main();
      if (!/ESP32-S3/i.test(String(chipName)))
        throw Error('Wrong chip detected: ' + chipName + '. Flash cancelled.');
      // The image is specifically an application binary for the existing
      // tested partition layout; SPIFFS/NVS/bootloader are never written.
      progressText('Flashing application (do not unplug)…',0);
      await loader.writeFlash({
        fileArray:[{data:image,address:APP_START}],
        flashMode:'dio', flashFreq:'40m', flashSize:'4MB',
        eraseAll:false, compress:true,
        reportProgress(_i,written,total) {
          const percent = Math.min(100,Math.round(100*written/Math.max(1,total)));
          progress.value = percent;
          progressLabel.textContent = 'Flashing application: ' + percent + '% — keep USB connected.';
        }
      });
      wrote = true;
      awaitingVersion = release.version;
      await loader.after('hard_reset');
      progressText('Flash finished. Reconnect using Connect jar to verify firmware ' + release.version + '.',100);
      message('Firmware written. Reconnect the jar to verify the installed version.');
    } catch(error) {
      progressText((wrote ? 'Firmware written, but reset or verification is incomplete: ' :
        'Update stopped before successful flashing: ') + error.message);
      log(error.stack || error.message,'error','FIRMWARE');
      message('Firmware update requires attention: ' + error.message,'error');
    } finally {
      try { await transport?.disconnect(); } catch(e) { log(String(e),'warn','ESPTOOL'); }
      running=false; ctx.serviceBusy=false; ctx.controls(); check.disabled=false;
      confirmation.checked=false; state();
    }
  }
  button.onclick = async () => {try {await install();} catch(error) {message(error.message,'error');}};
  confirmation.onchange = state;
  check.onclick = checkUpdates;
  void checkUpdates();

  return {
    receive({type,data}) {
      if (type !== 'IDENTITY') return;
      if (awaitingVersion) {
        const expected = awaitingVersion;
        awaitingVersion = null;
        if (data.fw === expected) {
          progressText('Verified firmware ' + expected + '. Check OLED, touch and LEDs.',100);
          message('Jar firmware ' + expected + ' verified after reconnect.');
        } else {
          progressText('Verification failed: expected ' + expected + ', jar reports ' + (data.fw||'unknown') + '.');
          message('Firmware version mismatch after reconnect. Check Service log.','error');
        }
      }
      state();
    },
    reset(){ if (!running) state(); }
  };
}
