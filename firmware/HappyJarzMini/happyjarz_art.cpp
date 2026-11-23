#include "happyjarz_art.h"
#include <SPIFFS.h>

namespace {
constexpr uint8_t kMaxArt = 50;
constexpr size_t kFrameBytes = 1024;
constexpr size_t kMaxFrames = 2;
constexpr size_t kChunkBytes = 48;
bool storageReady = false;
File incoming;
int incomingSlot = -1;
uint8_t incomingFrames = 0;
size_t incomingOffset = 0;
uint8_t saverSlot = 0;
uint8_t saverFrame = 0;
uint32_t lastFrameMs = 0;
uint32_t lastArtMs = 0;
uint8_t gallerySlots[kMaxArt];
uint8_t galleryCount = 0;

String artPath(uint8_t slot, const char *suffix) {
  char path[20];
  snprintf(path, sizeof(path), "/art%02u.%s", slot, suffix);
  return String(path);
}

void error(const char *message) {
  Serial.print("HJ|ERR|message=ART_");
  Serial.println(message);
}

void ack(const char *event, uint8_t slot = 255) {
  Serial.print("HJ|ART|ACK|event=");
  Serial.print(event);
  if (slot != 255) { Serial.print("|slot="); Serial.print(slot); }
  Serial.println();
}

bool parseSlot(const String &value, uint8_t &slot) {
  char *end = nullptr;
  long parsed = strtol(value.c_str(), &end, 10);
  if (!end || *end || parsed < 0 || parsed >= kMaxArt) return false;
  slot = (uint8_t)parsed;
  return true;
}

void refreshGallery() {
  galleryCount = 0;
  if (!storageReady) return;
  for (uint8_t slot = 0; slot < kMaxArt; ++slot) {
    if (SPIFFS.exists(artPath(slot, "dat"))) gallerySlots[galleryCount++] = slot;
  }
}

bool findNext(uint8_t start, uint8_t &found) {
  if (!galleryCount) return false;
  for (uint8_t i = 0; i < galleryCount; ++i) {
    if (gallerySlots[i] >= start) { found = gallerySlots[i]; return true; }
  }
  found = gallerySlots[0];
  return true;
}

bool decodeHexByte(char a, char b, uint8_t &value) {
  auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  int hi = digit(a), lo = digit(b);
  if (hi < 0 || lo < 0) return false;
  value = (uint8_t)((hi << 4) | lo);
  return true;
}
}

void hjArtInit() {
  storageReady = SPIFFS.begin(false);
  refreshGallery();
  Serial.print("HJ|ART|STORAGE|ready=");
  Serial.println(storageReady ? 1 : 0);
}

bool hjArtHandleCommand(const String &line) {
  if (!line.startsWith("ART ")) return false;
  String rest = line.substring(4);
  rest.trim();

  if (rest == "READY") {
    Serial.print("HJ|ART|ACK|event=READY|ready=");
    Serial.println(storageReady ? 1 : 0);
    return true;
  }
  if (rest == "FORMAT") {
    if (incoming) incoming.close();
    incomingSlot = -1;
    storageReady = SPIFFS.format() && SPIFFS.begin(false);
    if (!storageReady) { error("FORMAT_FAILED"); return true; }
    refreshGallery();
    ack("FORMAT");
    return true;
  }
  if (!storageReady) { error("STORAGE_NOT_READY"); return true; }
  if (rest == "COUNT") {
    Serial.print("HJ|ART|ACK|event=COUNT|count="); Serial.println(galleryCount);
    return true;
  }
  if (rest.startsWith("TRIM ")) {
    int keep = rest.substring(5).toInt();
    if (keep < 0 || keep > kMaxArt) { error("BAD_TRIM"); return true; }
    for (uint8_t slot = (uint8_t)keep; slot < kMaxArt; ++slot) {
      SPIFFS.remove(artPath(slot, "dat"));
      SPIFFS.remove(artPath(slot, "tmp"));
      SPIFFS.remove(artPath(slot, "bak"));
    }
    refreshGallery();
    ack("TRIM");
    return true;
  }
  if (rest.startsWith("BEGIN ")) {
    int slotEnd = rest.indexOf(' ', 6);
    if (slotEnd < 0) { error("BAD_BEGIN"); return true; }
    int framesEnd = rest.indexOf(' ', slotEnd + 1);
    String slotText = rest.substring(6, slotEnd);
    String frameText = framesEnd < 0 ? rest.substring(slotEnd + 1) : rest.substring(slotEnd + 1, framesEnd);
    uint8_t slot = 0;
    int frames = frameText.toInt();
    if (!parseSlot(slotText, slot) || frames < 1 || frames > kMaxFrames || framesEnd >= 0) {
      error("BAD_BEGIN"); return true;
    }
    if (incoming) incoming.close();
    incomingSlot = slot;
    incomingFrames = (uint8_t)frames;
    incomingOffset = 0;
    SPIFFS.remove(artPath(slot, "tmp"));
    incoming = SPIFFS.open(artPath(slot, "tmp"), FILE_WRITE);
    if (!incoming) { incomingSlot = -1; error("OPEN_FAILED"); return true; }
    ack("BEGIN", slot);
    return true;
  }
  if (rest.startsWith("DATA ")) {
    int slotEnd = rest.indexOf(' ', 5);
    int offsetEnd = slotEnd < 0 ? -1 : rest.indexOf(' ', slotEnd + 1);
    if (slotEnd < 0 || offsetEnd < 0 || incomingSlot < 0) { error("BAD_DATA"); return true; }
    uint8_t slot = 0;
    String slotText = rest.substring(5, slotEnd);
    String offsetText = rest.substring(slotEnd + 1, offsetEnd);
    String hex = rest.substring(offsetEnd + 1);
    long offset = offsetText.toInt();
    if (!parseSlot(slotText, slot) || slot != incomingSlot || offset < 0 ||
        (size_t)offset != incomingOffset || hex.length() == 0 || (hex.length() & 1) ||
        hex.length() > kChunkBytes * 2 || incomingOffset + hex.length()/2 > incomingFrames * kFrameBytes) {
      error("BAD_DATA"); return true;
    }
    uint8_t decoded[kChunkBytes];
    size_t count = hex.length() / 2;
    for (size_t i = 0; i < count; ++i) {
      if (!decodeHexByte(hex[i*2], hex[i*2+1], decoded[i])) { error("BAD_HEX"); return true; }
    }
    if (incoming.write(decoded, count) != count) { error("WRITE_FAILED"); return true; }
    incomingOffset += count;
    Serial.print("HJ|ART|ACK|event=DATA|slot="); Serial.print(slot);
    Serial.print("|offset="); Serial.println(incomingOffset);
    return true;
  }
  if (rest.startsWith("END ")) {
    uint8_t slot = 0;
    if (!parseSlot(rest.substring(4), slot) || slot != incomingSlot || !incoming) { error("BAD_END"); return true; }
    incoming.flush();
    size_t expected = (size_t)incomingFrames * kFrameBytes;
    bool complete = incomingOffset == expected && incoming.size() == expected;
    incoming.close();
    if (!complete) { SPIFFS.remove(artPath(slot, "tmp")); incomingSlot = -1; error("SIZE_MISMATCH"); return true; }

    String target = artPath(slot, "dat"), backup = artPath(slot, "bak"), temp = artPath(slot, "tmp");
    SPIFFS.remove(backup);
    bool hadOld = SPIFFS.exists(target);
    if (hadOld && !SPIFFS.rename(target, backup)) { incomingSlot = -1; error("BACKUP_FAILED"); return true; }
    if (!SPIFFS.rename(temp, target)) {
      if (hadOld) SPIFFS.rename(backup, target);
      incomingSlot = -1; error("COMMIT_FAILED"); return true;
    }
    SPIFFS.remove(backup);
    incomingSlot = -1;
    refreshGallery();
    ack("END", slot);
    return true;
  }
  error("UNKNOWN_COMMAND");
  return true;
}

void hjArtEnterSaver() {
  saverFrame = 0;
  lastFrameMs = millis();
  lastArtMs = lastFrameMs;
  uint8_t found = 0;
  if (storageReady && findNext(0, found)) saverSlot = found;
}

void hjArtRender(U8G2 *display) {
  if (!display) return;
  if (!storageReady) { display->drawStr(0, 30, "ART STORAGE OFF"); return; }
  uint32_t now = millis();
  uint8_t current = 0;
  if (!findNext(saverSlot, current)) {
    display->drawStr(0, 30, "NO ART YET");
    display->drawStr(0, 48, "SEND FROM PI APP");
    return;
  }
  saverSlot = current;
  if (now - lastArtMs >= 5000UL) {
    uint8_t next = (uint8_t)((saverSlot + 1U) % kMaxArt);
    if (findNext(next, current)) saverSlot = current;
    saverFrame = 0;
    lastArtMs = now;
  }
  String path = artPath(saverSlot, "dat");
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) { display->drawStr(0, 30, "ART READ ERROR"); return; }
  bool animated = file.size() >= kFrameBytes * 2;
  if (animated && now - lastFrameMs >= 400UL) {
    saverFrame = 1U - saverFrame;
    lastFrameMs = now;
  }
  if (!animated) saverFrame = 0;
  uint8_t pixels[kFrameBytes];
  bool readOk = file.seek((size_t)saverFrame * kFrameBytes) && file.read(pixels, sizeof(pixels)) == sizeof(pixels);
  file.close();
  if (readOk) display->drawXBM(0, 0, 128, 64, pixels);
}
