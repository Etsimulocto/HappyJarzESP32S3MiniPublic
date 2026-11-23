#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

void hjArtInit();
bool hjArtHandleCommand(const String &line);
void hjArtEnterSaver();
void hjArtRender(U8G2 *display);
