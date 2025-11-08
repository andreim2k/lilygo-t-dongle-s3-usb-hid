# USB-HID Shift Key Presser

A simple USB HID project for the LILYGO T-Dongle-S3 that randomly presses the Shift key at intervals between 7 and 60 seconds.

## Features

- **Random Shift Key Press**: Automatically presses the Shift key at random intervals (7-60 seconds)
- **Visual Display**: 160x80 pixel display showing:
  - Current uptime (HH:MM:SS format)
  - Countdown to next Shift key press
  - Total number of Shift keys pressed
- **LED Indicator**:
  - Green: Ready/Idle state
  - Blue: Pressing Shift key
- **USB HID**: Functions as a USB keyboard device

## Display Layout

```
┌──────────────────────┐
│ SHIFT KEY PRESSER    │ ← White title bar
├──────────────────────┤
│ Uptime: 00:12:34     │ ← Cyan label, white time
│                      │
│ Next in:             │ ← Yellow label
│ 23 sec               │ ← Large green text (red when < 10s)
│                      │
│ Pressed: 42 times    │ ← Magenta label, white count
└──────────────────────┘
```

## Hardware Requirements

- LILYGO T-Dongle-S3 (ESP32-S3 based)
- 160x80 TFT Display (ST7735)
- APA102 LED
- USB connection to host computer

## Pin Configuration

- **Display**:
  - RST: GPIO 1
  - DC: GPIO 2
  - MOSI: GPIO 3
  - CS: GPIO 4
  - SCLK: GPIO 5
  - LED: GPIO 38

- **LED**:
  - Data: GPIO 40
  - Clock: GPIO 39

- **Button**: GPIO 0

## Building and Uploading

### Using PlatformIO

1. Open the project in PlatformIO
2. Select the `USB-HID` environment
3. Build the project:
   ```bash
   pio run -e USB-HID
   ```
4. Upload to device:
   ```bash
   pio run -e USB-HID -t upload
   ```

### Note

The USB-HID environment is now the only and default environment in `platformio.ini`.

## Usage

1. Upload the firmware to your LILYGO T-Dongle-S3
2. Connect the device to a computer via USB
3. The device will be recognized as a USB keyboard
4. The display will show the current status
5. The Shift key will be pressed automatically at random intervals

## How It Works

- The device initializes as a USB HID keyboard
- A random delay between 7-60 seconds is generated
- When the timer expires:
  - The Shift key is pressed and released
  - The LED flashes blue
  - The press counter increments
  - A new random delay is generated
- The display updates every 100ms to show current status

## Use Cases

- Preventing screen savers from activating
- Keeping computer sessions active
- Testing keyboard input handling
- Anti-idle application

## Code Structure

- `src/main.cpp`: Main application code
- `include/User_Setup.h`: TFT_eSPI display configuration
- `platformio.ini`: Build configuration for USB-HID environment

## Dependencies

- `bodmer/TFT_eSPI`: Display driver library
- `pololu/apa102-arduino`: LED control library
- `espressif/esp32-usb`: USB functionality for ESP32-S3
- ESP32 Arduino framework 3.0.5

## Customization

### Change Time Range

Edit the `getRandomDelay()` function in `src/main.cpp`:

```cpp
unsigned long getRandomDelay() {
  // Change 7000 (7s) and 60001 (60s) to your preferred range
  unsigned long delayMs = random(7000, 60001);
  return delayMs;
}
```

### Change Display Colors

Modify the color constants in `updateDisplay()`:

```cpp
tft.setTextColor(TFT_CYAN, TFT_BLACK);  // Change TFT_CYAN to any color
```

Available colors: `TFT_BLACK`, `TFT_WHITE`, `TFT_RED`, `TFT_GREEN`, `TFT_BLUE`, `TFT_CYAN`, `TFT_MAGENTA`, `TFT_YELLOW`

### Press Different Keys

Modify the `pressShiftKey()` function:

```cpp
keyboard.press(KEY_LEFT_SHIFT);  // Change to other keys like KEY_SPACE, etc.
```

## Troubleshooting

### Device Not Recognized as Keyboard

- Ensure USB cable supports data transfer
- Check that ARDUINO_USB_MODE=0 in build flags
- Try a different USB port

### Display Not Working

- Verify pin connections match configuration
- Check TFT_eSPI User_Setup.h is correct
- Ensure display backlight (GPIO 38) is enabled

### LED Not Working

- Verify APA102 LED connections (Data: GPIO 40, Clock: GPIO 39)
- Check power supply to LED

## License

Same as the parent USBArmyKnife project.

## Credits

Based on the USBArmyKnife project hardware configuration.
