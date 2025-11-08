// TFT_eSPI User Setup for USB-HID Project
// LILYGO T-Dongle-S3 Display Configuration

#define USER_SETUP_INFO "User_Setup"

#define ST7735_DRIVER      // Display driver
#define ST7735_GREENTAB160x80  // For 160 x 80 display

#define TFT_WIDTH  80
#define TFT_HEIGHT 160

#define TFT_MOSI 3
#define TFT_SCLK 5
#define TFT_CS   4
#define TFT_DC   2
#define TFT_RST  1
#define TFT_BL   38  // LED back-light

#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
