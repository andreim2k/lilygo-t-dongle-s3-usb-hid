#ifdef USB_HID_PROJECT

#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>
#include <LovyanGFX.hpp>
#include <APA102.h>

// Display setup using LovyanGFX (same as LILYGO-T-Dongle-S3)
class LGFX_USB_HID : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7735S _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

public:
  LGFX_USB_HID(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_mode = 0;
      cfg.freq_write = 27000000;
      cfg.freq_read = 16000000;
      cfg.pin_sclk = DISPLAY_SCLK;
      cfg.pin_mosi = DISPLAY_MOSI;
      cfg.pin_miso = DISPLAY_MISO;
      cfg.pin_dc = DISPLAY_DC;
      cfg.spi_host = SPI3_HOST;
      cfg.spi_3wire = true;
      cfg.use_lock = false;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = DISPLAY_CS;
      cfg.pin_rst = DISPLAY_RST;
      cfg.pin_busy = DISPLAY_BUSY;
      cfg.panel_width = DISPLAY_HEIGHT;
      cfg.panel_height = DISPLAY_WIDTH;
      cfg.offset_rotation = 1; // Changed from 3 to 1 for right-side USB port
      cfg.readable = true;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      cfg.offset_x = 26;
      cfg.offset_y = 1;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.memory_width = 132;
      cfg.memory_height = 160;
      _panel_instance.config(cfg);
    }
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = DISPLAY_LEDA;
      cfg.invert = true;
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    setPanel(&_panel_instance);
  }
};

static LGFX_USB_HID lcd;

// USB Keyboard and Mouse
USBHIDKeyboard keyboard;
USBHIDMouse mouse;

// LED setup
APA102<LED_DI_PIN, LED_CI_PIN> ledStrip;
rgb_color colors[NUM_LEDS];

// Timing variables
unsigned long lastShiftTime = 0;
unsigned long nextShiftDelay = 0;
unsigned long shiftPressCount = 0;

// Time tracking
unsigned long startTime = 0;
unsigned long currentTime = 0;

// Animation variables
uint8_t pulsePhase = 0;
bool justPressed = false;
unsigned long pressAnimationStart = 0;

// Color definitions for modern UI
#define COLOR_BG 0x0841       // Dark blue-gray background
#define COLOR_PANEL 0x2124    // Lighter panel color
#define COLOR_ACCENT 0x05FF   // Cyan accent
#define COLOR_SUCCESS 0x07E0  // Green
#define COLOR_WARNING 0xFD20  // Orange
#define COLOR_DANGER 0xF800   // Red
#define COLOR_TEXT 0xFFFF     // White text
#define COLOR_TEXT_DIM 0x8410 // Dim text

// Function prototypes
void setupDisplay();
void setupUSB();
void setupLED();
void updateDisplay();
void moveMouse();
unsigned long getRandomDelay();
String formatTime(unsigned long seconds);
void drawHeader();
void drawUptimePanel(unsigned long seconds);
void drawCountdownPanel(unsigned long timeLeft, unsigned long totalTime);
void drawStatsPanel();
void drawProgressBar(int x, int y, int width, int height, float percentage, uint16_t color);
void drawRoundRect(int x, int y, int width, int height, int radius, uint16_t color);
void fillRoundRect(int x, int y, int width, int height, int radius, uint16_t color);

void setup()
{
  Serial.begin(115200);

  // Initialize random seed
  randomSeed(analogRead(0));

  // Setup components
  setupLED();
  setupDisplay();
  setupUSB();

  // Initialize timing
  startTime = millis();
  lastShiftTime = millis();
  nextShiftDelay = getRandomDelay();

  Serial.println("USB-HID Mouse Mover Started!");
}

void loop()
{
  currentTime = millis();

  // Check if it's time to move mouse
  if (currentTime - lastShiftTime >= nextShiftDelay)
  {
    moveMouse();
    lastShiftTime = currentTime;
    nextShiftDelay = getRandomDelay();
    shiftPressCount++;
    justPressed = true;
    pressAnimationStart = currentTime;
  }

  // Update display every 50ms for smooth animations
  static unsigned long lastDisplayUpdate = 0;
  if (currentTime - lastDisplayUpdate >= 50)
  {
    updateDisplay();
    lastDisplayUpdate = currentTime;
    pulsePhase = (pulsePhase + 1) % 255;
  }

  // Reset press animation after 500ms
  if (justPressed && (currentTime - pressAnimationStart > 500))
  {
    justPressed = false;
  }

  delay(10);
}

void setupLED()
{
  // Set LED to green to indicate ready
  colors[0] = rgb_color{0, 50, 0};
  ledStrip.write(colors, NUM_LEDS);
}

void setupDisplay()
{
  lcd.init();
  lcd.setBrightness(255);
  lcd.fillScreen(COLOR_BG);
  lcd.setTextColor(COLOR_TEXT);

  // Splash screen
  lcd.setTextSize(1);
  lcd.setCursor(10, 30);
  lcd.setTextColor(COLOR_ACCENT);
  lcd.println("USB HID KEYBOARD");
  lcd.setCursor(30, 45);
  lcd.setTextColor(COLOR_TEXT_DIM);
  lcd.println("Initializing...");

  delay(1500);
  lcd.fillScreen(COLOR_BG);
}

void setupUSB()
{
  // Initialize keyboard and mouse FIRST, then USB
  Serial.println("Initializing keyboard...");
  keyboard.begin();
  delay(250);
  
  Serial.println("Initializing mouse...");
  mouse.begin();
  delay(250);
  
  // Then start USB
  Serial.println("Starting USB with keyboard and mouse...");
  USB.begin();
  delay(3000);  // Give USB time to enumerate and be recognized by host
  
  Serial.println("USB HID Keyboard and Mouse initialized!");
  Serial.println("Waiting for host to recognize device...");
  delay(2000);
  Serial.println("Ready to move mouse!");
}

void updateDisplay()
{
  static unsigned long lastUptimeSeconds = 0;
  static unsigned long lastTimeUntilShift = 9999;
  static unsigned long lastShiftCount = 0;
  static bool needsFullRedraw = true;

  int dispWidth = lcd.width();
  int dispHeight = lcd.height();

  unsigned long uptimeSeconds = (currentTime - startTime) / 1000;
  unsigned long timeUntilShift = 0;

  if (currentTime - lastShiftTime < nextShiftDelay)
  {
    timeUntilShift = (nextShiftDelay - (currentTime - lastShiftTime)) / 1000;
  }

  // Full redraw on first run
  if (needsFullRedraw)
  {
    lcd.fillScreen(COLOR_BG);
    drawHeader();
    needsFullRedraw = false;
    lastUptimeSeconds = uptimeSeconds;
    lastTimeUntilShift = timeUntilShift;
    lastShiftCount = shiftPressCount;
  }

  // Update uptime panel
  if (uptimeSeconds != lastUptimeSeconds || needsFullRedraw)
  {
    drawUptimePanel(uptimeSeconds);
    lastUptimeSeconds = uptimeSeconds;
  }

  // Update countdown panel (always update for animations)
  if (timeUntilShift != lastTimeUntilShift || needsFullRedraw)
  {
    drawCountdownPanel(timeUntilShift, nextShiftDelay / 1000);
    lastTimeUntilShift = timeUntilShift;
  }

  // Update stats panel
  if (shiftPressCount != lastShiftCount || needsFullRedraw)
  {
    drawStatsPanel();
    lastShiftCount = shiftPressCount;
  }
}

void drawHeader()
{
  int dispWidth = lcd.width();

  // Gradient header background
  for (int y = 0; y < 16; y++)
  {
    uint16_t color = lcd.color565(0, 40 + y * 2, 60 + y * 3);
    lcd.drawFastHLine(0, y, dispWidth, color);
  }

  // Header border
  lcd.drawFastHLine(0, 15, dispWidth, COLOR_ACCENT);

  // Title with keyboard icon
  lcd.setTextSize(1);
  lcd.setTextColor(COLOR_TEXT);
  lcd.setCursor(4, 4);
  lcd.print((char)0x10); // Triangle icon
  lcd.print(" AUTO-PRESSER");

  // Status indicator (top right)
  int statusX = dispWidth - 10;
  lcd.fillCircle(statusX, 8, 3, COLOR_SUCCESS);
  lcd.drawCircle(statusX, 8, 4, COLOR_TEXT);
}

void drawUptimePanel(unsigned long seconds)
{
  int panelY = 18;
  int panelH = 16;
  int dispWidth = lcd.width();

  // Clear panel area
  lcd.fillRect(2, panelY, dispWidth - 4, panelH, COLOR_PANEL);

  // Panel border
  drawRoundRect(2, panelY, dispWidth - 4, panelH, 2, COLOR_ACCENT);

  // Clock icon and label
  lcd.setTextSize(1);
  lcd.setTextColor(COLOR_ACCENT);
  lcd.setCursor(5, panelY + 4);
  lcd.print((char)0x0F); // Clock symbol

  lcd.setTextColor(COLOR_TEXT_DIM);
  lcd.setCursor(15, panelY + 4);
  lcd.print("UP:");

  // Time value
  lcd.setTextColor(COLOR_TEXT);
  lcd.setCursor(35, panelY + 4);
  lcd.print(formatTime(seconds));
}

void drawCountdownPanel(unsigned long timeLeft, unsigned long totalTime)
{
  int panelY = 36;
  int panelH = 30;
  int dispWidth = lcd.width();

  // Clear panel area
  lcd.fillRect(2, panelY, dispWidth - 4, panelH, COLOR_PANEL);

  // Determine color based on time left
  uint16_t accentColor = COLOR_SUCCESS;
  if (timeLeft < 5)
    accentColor = COLOR_DANGER;
  else if (timeLeft < 15)
    accentColor = COLOR_WARNING;

  // Panel border with pulsing effect when near zero
  if (timeLeft < 5 && (pulsePhase % 64) < 32)
  {
    drawRoundRect(2, panelY, dispWidth - 4, panelH, 3, accentColor);
    drawRoundRect(3, panelY + 1, dispWidth - 6, panelH - 2, 3, accentColor);
  }
  else
  {
    drawRoundRect(2, panelY, dispWidth - 4, panelH, 3, accentColor);
  }

  // Calculate total width for centering both label and number together
  char timeStr[12];
  sprintf(timeStr, "%lu", timeLeft);

  // "NEXT IN:" label (size 1) = 8 chars * 6 = 48 pixels
  // Space = 4 pixels
  // Number (size 2) = roughly strlen * 12 pixels
  // "s" (size 1) = 6 pixels

  int labelWidth = 8 * 6; // "NEXT IN:"
  int spaceWidth = 4;
  int numberWidth = strlen(timeStr) * 12; // Size 2 for number
  int suffixWidth = 6;                    // "s"
  int totalWidth = labelWidth + spaceWidth + numberWidth + suffixWidth;

  int startX = (dispWidth - totalWidth) / 2;
  int textY = panelY + 10; // Vertically centered in 30px panel

  // Draw "NEXT IN:" label
  lcd.setTextSize(1);
  lcd.setTextColor(COLOR_TEXT_DIM);
  lcd.setCursor(startX, textY);
  lcd.print("NEXT IN:");

  // Draw countdown number (larger)
  lcd.setTextSize(2);
  lcd.setTextColor(accentColor);
  lcd.setCursor(startX + labelWidth + spaceWidth, textY - 2); // -2 to align baseline
  lcd.print(timeLeft);

  // Draw "s" suffix
  lcd.setTextSize(1);
  lcd.setTextColor(COLOR_TEXT_DIM);
  lcd.setCursor(startX + labelWidth + spaceWidth + numberWidth, textY);
  lcd.print("s");

  // Progress bar at bottom of panel
  float percentage = 1.0 - ((float)timeLeft / (float)totalTime);
  if (percentage < 0)
    percentage = 0;
  if (percentage > 1)
    percentage = 1;

  drawProgressBar(6, panelY + panelH - 6, dispWidth - 12, 3, percentage, accentColor);
}

void drawStatsPanel()
{
  int panelY = 68;
  int panelH = 11;
  int dispWidth = lcd.width();

  // Clear panel area
  lcd.fillRect(2, panelY, dispWidth - 4, panelH, COLOR_PANEL);

  // Panel border
  lcd.drawRect(2, panelY, dispWidth - 4, panelH, COLOR_ACCENT);

  // Checkmark icon
  lcd.setTextSize(1);
  lcd.setTextColor(COLOR_SUCCESS);
  lcd.setCursor(5, panelY + 2);
  lcd.print((char)0xFB); // Check mark

  // Label
  lcd.setTextColor(COLOR_TEXT_DIM);
  lcd.setCursor(15, panelY + 2);
  lcd.print("TOTAL:");

  // Count value
  lcd.setTextColor(COLOR_TEXT);
  lcd.setCursor(55, panelY + 2);
  lcd.print(shiftPressCount);

  // Animation flash on new press
  if (justPressed)
  {
    lcd.drawRect(1, panelY - 1, dispWidth - 2, panelH + 2, COLOR_SUCCESS);
  }
}

void drawProgressBar(int x, int y, int width, int height, float percentage, uint16_t color)
{
  // Background
  lcd.fillRect(x, y, width, height, COLOR_BG);
  lcd.drawRect(x, y, width, height, COLOR_TEXT_DIM);

  // Filled portion
  int fillWidth = (int)((width - 2) * percentage);
  if (fillWidth > 0)
  {
    lcd.fillRect(x + 1, y + 1, fillWidth, height - 2, color);
  }
}

void drawRoundRect(int x, int y, int width, int height, int radius, uint16_t color)
{
  // Simplified rounded rectangle (just draws corners)
  lcd.drawRect(x + radius, y, width - 2 * radius, height, color);
  lcd.drawRect(x, y + radius, width, height - 2 * radius, color);

  // Corner pixels (simple version)
  lcd.drawPixel(x + radius, y, color);
  lcd.drawPixel(x + width - radius - 1, y, color);
  lcd.drawPixel(x + radius, y + height - 1, color);
  lcd.drawPixel(x + width - radius - 1, y + height - 1, color);
}

void fillRoundRect(int x, int y, int width, int height, int radius, uint16_t color)
{
  lcd.fillRect(x + radius, y, width - 2 * radius, height, color);
  lcd.fillRect(x, y + radius, radius, height - 2 * radius, color);
  lcd.fillRect(x + width - radius, y + radius, radius, height - 2 * radius, color);
}

void moveMouse()
{
  Serial.println("\n=== MOVING MOUSE ===");
  Serial.printf("Time: %lu ms, Move count: %lu\n", millis(), shiftPressCount + 1);

  // Flash LED purple when moving
  colors[0] = rgb_color{128, 0, 255};
  ledStrip.write(colors, NUM_LEDS);
  Serial.println("LED set to purple");

  // Move mouse 1 pixel to the right
  Serial.println("Moving mouse 1 pixel right...");
  mouse.move(1, 0);
  delay(20);
  
  // Quickly move mouse 1 pixel to the left
  Serial.println("Moving mouse 1 pixel left...");
  mouse.move(-1, 0);
  delay(20);
  
  Serial.println("Mouse movement complete!");

  // Return LED to green
  colors[0] = rgb_color{0, 50, 0};
  ledStrip.write(colors, NUM_LEDS);
  Serial.println("LED set to green");

  Serial.printf("=== Mouse movement complete! Total: %lu ===\n\n", shiftPressCount + 1);
}

unsigned long getRandomDelay()
{
  unsigned long delayMs = random(7000, 60001);
  Serial.printf("Next mouse move in %lu ms (%lu seconds)\n", delayMs, delayMs / 1000);
  return delayMs;
}

String formatTime(unsigned long seconds)
{
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;

  char buffer[12];
  sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, secs);
  return String(buffer);
}

#endif // USB_HID_PROJECT
