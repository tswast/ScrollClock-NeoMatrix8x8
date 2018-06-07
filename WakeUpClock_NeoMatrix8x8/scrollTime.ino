
void scrollTime(void) {
  int prevClockX = clockX;
  int prevClockY = clockY;
  clockX = ((millis() / 1000) % (CLOCKWIDTH + 8)) - 8;
  
  if (random(1000) == 0) {
    clockY = clockY + random(-1, 2);
    if (clockY < 0) {
      clockY = 0;
    }
    if (clockY >= 4) {
      clockY = 3;
    }
  }

  if (prevClockX != clockX || prevClockY != clockY) {
    // Scroll, so darken all pixels for legibility.
    darkenScreenBuffer();
  }

  
  renderClock();

  int color = PICO_WHITE;
  int theHour = theTime.hour();
  int theMinute = theTime.minute();
  int theSecond = theTime.second();
  int theDay = theTime.dayOfTheWeek();
  int wakeUpHour = 5;
  if (theDay == 0 or theDay == 6) {
    wakeUpHour = 6;  // Sleep in on the weekends.
  }
  if (theHour > 21 || theHour < wakeUpHour) {
    color = PICO_BLACK;
  } else if (theHour == 21) {
    // Color should get darker from 9pm to 10pm.
    int minutesTo10 = 60 - theMinute;
    // Subtract the seconds because we already counted those seconds
    // as part of the minutes.
    int secondsTo10 = (minutesTo10 * 60) - theSecond;
    // Add 1 to account for remainder when we divide the hour.
    // We don't want there to be any remainder because that would
    // be beyond the color palette.
    // Subtract 1 from PICO_WHITE because we want to cycle through
    // all colors except for full black.
    int secondsPerColor = 1 + ((60 * 60) / (PICO_WHITE - 1));
    color = 1 + (secondsTo10 / secondsPerColor);
  } else if (theHour == wakeUpHour) {
    // Color should get brighter from 5:45am to 6am.
    if (theMinute < 45) {
      color = 0;
    } else {
      int minutesTo6 = 60 - theMinute;
      int secondsTo6 = (minutesTo6 * 60) - theSecond;
      int secondsPerColor = 1 + (15 * 60) / (PICO_WHITE - 1);
      color = PICO_WHITE - (secondsTo6 / secondsPerColor);
    }
  }
  blitClock(clockX, clockY, color);
  //matrix.fillScreen(0);
  drawScreenBuffer();
  matrix.show();
}

void renderClock(void) {
  clearClockBuffer();
  renderClockDigit(0, char(0x30 + theTime.hour() / 10));
  renderClockDigit(4, char(0x30 + theTime.hour() % 10));
  renderClockDigit(7,':');
  renderClockDigit(10, char(0x30 + theTime.minute() / 10));
  renderClockDigit(14, char(0x30 + theTime.minute() % 10));
}

void clearClockBuffer(void) {
  for (int y=0; y < CLOCKHEIGHT; y++) {
    for (int x=0; x < CLOCKWIDTH; x++) {
      clockBuffer[y][x] = 0;
    }
  }
}

void renderClockDigit(int startX, char digit) {
  unsigned int mask = 0;
  switch (digit) {
    case '0':
      mask = PICO_ZERO;
      break;
    case '1':
      mask = PICO_ONE;
      break;
    case '2':
      mask = PICO_TWO;
      break;
    case '3':
      mask = PICO_THREE;
      break;
    case '4':
      mask = PICO_FOUR;
      break;
    case '5':
      mask = PICO_FIVE;
      break;
    case '6':
      mask = PICO_SIX;
      break;
    case '7':
      mask = PICO_SEVEN;
      break;
    case '8':
      mask = PICO_EIGHT;
      break;
    case '9':
      mask = PICO_NINE;
      break;
    case ':':
      mask = PICO_COLON;
      break;
  }
  unsigned int row = 7 << (4*3);
  for (int rowIndex=0; rowIndex < 5; rowIndex++) {
    unsigned int rowbits = (row & mask) >> (3 * (4 - rowIndex));
    unsigned int col = 1 << 2;
    for (int colIndex=startX; colIndex < startX + 3; colIndex++) {
      if (col & rowbits) {
        clockBuffer[rowIndex][colIndex] = 1;
      }
      col >>= 1;
    }
    row >>= 3;
  }
}

void darkenScreenBuffer(void) {
  for (int sy=0; sy < 8; sy++) {
    for (int sx=0; sx < 8; sx++) {
      if (screenBuffer[sy][sx] > 0) {
        screenBuffer[sy][sx] -= 1;
      }
    }
  }
}

int clockXFromScreenX(int clockStartX, int screenX) {
  return clockStartX + screenX;
}

int clockYFromScreenY(int screenStartY, int screenY) {
  return screenY - screenStartY;
}

int maxNeighborColor(int sx, int sy) {
  int maxColor = 0;
  int color = 0;
  if (sx > 0) {
    maxColor = screenBuffer[sy][sx-1]; // left
  }
  if (sy > 0) {
    color = screenBuffer[sy-1][sx]; // up
    if (color > maxColor) {
      maxColor = color;
    }
  }
  if (sx < 7) {
    color = screenBuffer[sy][sx+1]; // right
    if (color > maxColor) {
      maxColor = color;
    }
  }
  if (sy < 7) {
    color = screenBuffer[sy+1][sx]; // down
    if (color > maxColor) {
      maxColor = color;
    }
  }
  return maxColor;
}

void drawClockBit(int sx, int sy, int clockBit, int color) {
  if (clockBit) {
    screenBuffer[sy][sx] = color;
  } else if (screenBuffer[sy][sx] > 0) {
    screenBuffer[sy][sx] -= 1;
  } else {
    int neighbors = maxNeighborColor(sx, sy);
    if (neighbors >= color / 2) {
      screenBuffer[sy][sx] = neighbors / 2;  // random glow
    }
  }
}

void blitClock(int clockStartX, int screenStartY, int color) {
  int clockBit = 0;
  int sy = 0;
  int sx = 0;
  int cy = 0;
  int cx = 0;

  for (int i=0; i < 4; i++) {
    sy = random(8);
    sx = random(8);
    cy = clockYFromScreenY(screenStartY, sy);
    cx = clockXFromScreenX(clockStartX, sx);

    if (cy < 0 || cy >= CLOCKHEIGHT || cx < 0 || cx >= CLOCKWIDTH) {
      drawClockBit(sx, sy, 0, color);
      continue;
    }
    drawClockBit(sx, sy, clockBuffer[cy][cx], color);
  }
}

void drawScreenBuffer(void) {
  for (int sy=0; sy < 8; sy++) {
    for (int sx=0; sx < 8; sx++) {
      matrix.drawPixel(
        7 - sx, 7 - sy,  // Rotate 180 degrees.
        screenPalette[screenBuffer[sy][sx]]);
    }
  }
}

