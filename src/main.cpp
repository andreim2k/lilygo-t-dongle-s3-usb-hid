#ifdef USB_HID_PROJECT

#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
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
      cfg.offset_rotation = 3;
      cfg.readable = true;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      cfg.offset_x = 26;
      cfg.offset_y = -1;
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

// USB Keyboard
USBHIDKeyboard keyboard;

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

// Function prototypes
void setupDisplay();
void setupUSB();
void setupLED();
void updateDisplay();
void pressShiftKey();
unsigned long getRandomDelay();
String formatTime(unsigned long seconds);

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

  Serial.println("USB-HID Shift Key Presser Started!");
}

void loop()
{
  currentTime = millis();

  // Check if it's time to press Shift
  if (currentTime - lastShiftTime >= nextShiftDelay)
  {
    pressShiftKey();
    lastShiftTime = currentTime;
    nextShiftDelay = getRandomDelay();
    shiftPressCount++;
  }

  // Update display every 100ms
  static unsigned long lastDisplayUpdate = 0;
  if (currentTime - lastDisplayUpdate >= 100)
  {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }

  delay(10); // Small delay to prevent CPU hogging
}

void setupLED()
{
  // Set LED to green to indicate ready
  colors[0] = rgb_color{0, 50, 0}; // Dim green
  ledStrip.write(colors, NUM_LEDS);
}

void setupDisplay()
{
  // Initialize display
  lcd.init();
  lcd.setBrightness(128);
  lcd.clear(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);

  // Initial screen
  lcd.setCursor(0, 0);
  lcd.setTextSize(1);
  lcd.println("USB-HID Shift Presser");
  lcd.println("Initializing...");
  lcd.display();

  delay(1000);
}

void setupUSB()
{
  // Initialize USB HID Keyboard
  keyboard.begin();
  USB.begin();
  delay(1000); // Wait for USB to be recognized
}

void updateDisplay()
{
  static unsigned long lastUptimeSeconds = 0;
  static unsigned long lastTimeUntilShift = 9999;
  static unsigned long lastShiftCount = 0;
  static bool firstRun = true;
  static bool needsRedraw = false;

  // Get actual display dimensions
  int dispWidth = lcd.width();
  int dispHeight = lcd.height();

  // Calculate uptime
  unsigned long uptimeSeconds = (currentTime - startTime) / 1000;

  // Calculate time until next shift press
  unsigned long timeUntilShift = (nextShiftDelay - (currentTime - lastShiftTime)) / 1000;
  if (currentTime - lastShiftTime > nextShiftDelay)
  {
    timeUntilShift = 0;
  }

  // Only redraw on first run or if something changed
  if (firstRun)
  {
    firstRun = false;

    // Clear entire display properly
    lcd.clear(TFT_BLACK);

    // Title bar - White background with black text (static, draw once)
    lcd.fillRect(0, 0, dispWidth, 12, TFT_WHITE);
    lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    lcd.setCursor(2, 2);
    lcd.setTextSize(1);
    lcd.print("SHIFT KEY PRESSER");

    // Draw static labels with background
    lcd.setTextSize(1);
    lcd.setCursor(2, 20);
    lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    lcd.print("Uptime:");

    lcd.setCursor(2, 34);
    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.print("Next in:");

    lcd.setCursor(2, dispHeight - 10);
    lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
    lcd.print("Pressed:");

    lcd.display();
    needsRedraw = false;
  }

  // Update uptime if changed
  if (uptimeSeconds != lastUptimeSeconds)
  {
    lcd.fillRect(50, 20, dispWidth - 50, 8, TFT_BLACK); // Clear old time
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.setCursor(50, 20);
    lcd.print(formatTime(uptimeSeconds));
    lastUptimeSeconds = uptimeSeconds;
    needsRedraw = true;
  }

  // Update countdown if changed
  if (timeUntilShift != lastTimeUntilShift)
  {
    lcd.fillRect(0, 46, dispWidth, 18, TFT_BLACK); // Clear entire countdown area
    lcd.setCursor(4, 48);
    lcd.setTextSize(2);
    if (timeUntilShift < 10)
    {
      lcd.setTextColor(TFT_RED, TFT_BLACK); // Red when close
    }
    else
    {
      lcd.setTextColor(TFT_GREEN, TFT_BLACK); // Green otherwise
    }
    char timeBuffer[12];
    sprintf(timeBuffer, "%02lu sec", timeUntilShift);
    lcd.print(timeBuffer);
    lastTimeUntilShift = timeUntilShift;
    needsRedraw = true;
  }

  // Update press count if changed
  if (shiftPressCount != lastShiftCount)
  {
    lcd.fillRect(60, dispHeight - 10, dispWidth - 60, 8, TFT_BLACK); // Clear old count
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setCursor(60, dispHeight - 10);
    lcd.print(shiftPressCount);
    lcd.print(" times");
    lastShiftCount = shiftPressCount;
    needsRedraw = true;
  }

  // Only call display() if something changed
  if (needsRedraw)
  {
    lcd.display();
    needsRedraw = false;
  }
}

void pressShiftKey()
{
  Serial.println("Pressing Shift key...");

  // Flash LED blue when pressing
  colors[0] = rgb_color{0, 0, 255}; // Blue
  ledStrip.write(colors, NUM_LEDS);

  // Press and release Shift key
  keyboard.press(KEY_LEFT_SHIFT);
  delay(50);
  keyboard.release(KEY_LEFT_SHIFT);

  // Return LED to green
  colors[0] = rgb_color{0, 50, 0}; // Green
  ledStrip.write(colors, NUM_LEDS);

  Serial.printf("Shift pressed! Total count: %lu\n", shiftPressCount + 1);
}

unsigned long getRandomDelay()
{
  // Random delay between 7 and 60 seconds (7000 to 60000 ms)
  unsigned long delayMs = random(7000, 60001);
  Serial.printf("Next shift in %lu ms (%lu seconds)\n", delayMs, delayMs / 1000);
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
