

# STM32 Ultrasonic Distance Measurement with TFT Display

[中文](README_CN.md) | English

An embedded distance measurement project built around the STM32F103C8T6, a CS100A ultrasonic sensor, and an ST7735S color TFT display. External interrupts and a timer measure the echo pulse width, convert it to distance, and display the result in millimeters. The project provides continuous measurement, sample filtering, and timeout indication, serving as a starting point for embedded development and distance sensing in smart-lock inspection applications.


## Features

- EXTI0 detects rising and falling echo edges; TIM2 measures the high pulse duration.
- Single measurements and multiple-sample calculation: collect 10 distance values, sort them, discard the two smallest and two largest, and average the remaining six.
- 32-bit intermediate values prevent 16-bit overflow in distance conversion and summation.
- SPI1-based ST7735S driver with RGB565 colors, screen clearing, rectangle filling, pixel drawing, and integer character scaling.
- 8×16 printable ASCII characters, selected 16×16 Chinese glyphs, and a UTF-8 string display interface.
- Red and white startup test screens, distance updates, and a red `---` indicator when the initial measurement times out.

## Hardware and Wiring

Use an STM32F103C8T6 development board, a CS100A ultrasonic sensor, an ST7735S TFT module, and a compatible SWD programmer. The panel has 128×160 pixels; the current driver uses a **160×128 logical coordinate range**. Verify supply voltages and signal levels against the actual module specifications, and connect all grounds together.

| Module | Signal | STM32 Pin | Purpose |
| --- | --- | --- | --- |
| CS100A | TRIG | PB0 | Measurement trigger; currently a 50 μs pulse |
| CS100A | ECHO | PA0 | Echo input connected to EXTI0 |
| ST7735S | SCK / SCL | PA5 | SPI1 clock |
| ST7735S | SDA / MOSI | PA7 | SPI1 data output |
| ST7735S | CS | PB12 | Active-low chip select |
| ST7735S | DC / A0 | PB13 | Command/data selection |
| ST7735S | RST / RES | PB14 | Active-low reset |
| ST7735S | BL / LED | PB10 | Backlight; active-high by default |
| All modules | GND | GND | Common ground |

The driver configures PA6 as an input, but a display MISO connection is not required; the code does not read display status. Logical dimensions, scan direction, offsets, and backlight polarity are configured in `Hardware/st7735.h`.

## Build and Run

1. Open `tem_mis_disp.uvprojx` in Keil µVision.
2. The project targets `STM32F103C8` and records ARM Compiler 5.06 update 5 and the `Keil.STM32F1xx_DFP.2.2.0` device pack. Configure the compiler and device support for your installation.
3. Keep the project definitions `USE_STDPERIPH_DRIVER` and `STM32F10X_MD`. Include paths already cover `Start`, `Library`, `System`, `Hardware`, and `User`.
4. Check that your board clock matches `Start/system_stm32f10x.c`. The current configuration selects a 72 MHz system clock. TIM2 uses a `72 - 1` prescaler, producing a 1 μs tick with a 72 MHz timer clock.
5. Build the project, configure the SWD programmer, and download the firmware. Build artifacts are stored in `Objects/` and excluded from version control.
6. On startup, check the red and white screens and welcome text. Place a target in front of the sensor and check that the displayed distance updates.

The main loop delays for 300 ms after each display update. The complete refresh period also includes measurement, sampling delays, and display transfer time; it is not a fixed 300 ms.

## Measurement Flow

Send a trigger pulse → wait for the rising echo edge → reset TIM2 → capture the counter on the falling edge → convert the pulse width → update the display.

The current conversion is:

```text
distance_mm = echo_time_us × 343 / 2000
```

TIM2 has an auto-reload value of `0xFFFF`. With a 1 MHz counter clock, timer overflow while waiting for a rising edge or measuring the high pulse causes a timeout. Each counter window is approximately 65.536 ms. Millimeters are the output unit, not a claim of calibrated millimeter-level accuracy.

| API | Purpose |
| --- | --- |
| `cs100a_init()` | Initialize GPIO, EXTI0, and TIM2 |
| `cs100a_start()` | Start one measurement; ignore requests while busy |
| `cs100a_isFinished()` / `cs100a_isTimeout()` | Query completion and timeout status |
| `cs100a_getDistanceMm()` | Read the distance from a completed measurement |
| `cs100a_CalDistanceMm()` | Collect 10 distance values synchronously and return their trimmed mean |

## Repository Layout

```text
.
├── main.c                   Main loop and distance display
├── tem_mis_disp.uvprojx      Keil project
├── Hardware/                Ultrasonic, TFT, font, and other peripheral drivers
├── System/                  Delay and TIM2 drivers
├── Library/                 STM32F10x Standard Peripheral Library
├── Start/                   CMSIS, startup files, and system clock setup
├── User/                    Peripheral library setup and system exceptions
├── tests/                   Host LCD tests and mock peripheral headers
├── _analysis/               Font analysis and preview script
└── docs/                    Troubleshooting and repository audit records
```

Button, LED, and OLED drivers are also included. The current main application uses the ultrasonic sensor and ST7735S TFT.

## Host Tests and Font Preview

The host LCD tests require Windows, an MSVC x64 toolchain, 64-bit Python, and PowerShell. The script loads the test DLL with Python `ctypes`. Replace the placeholder below with your MSVC version directory containing `bin`, `include`, and related folders:

```powershell
.\tests\run_lcd_tests.ps1 -MsvcRoot "<local MSVC version directory>"
```

Outputs are written to `Objects/lcd_tests/`. During the latest check, compilation and linking succeeded, but an LCD coordinate assertion failed: the test expects 128×160 endpoints, while the driver is configured for 160×128. The version before the author-comment changes failed at the same assertion. The test suite is therefore not currently passing. See the [audit record, in Chinese](docs/仓库上传前敏感信息检查.md).

After installing Pillow, generate font previews from the repository root:

```powershell
python .\_analysis\render_font.py
```

## Current Limitations

- The main loop and multiple-sample function use blocking waits. Additional real-time tasks require scheduling changes.
- Timed-out measurements in the sample batch become `0` values and enter the sort; invalid samples are not separately excluded.
- Chinese text is limited to glyphs included in the font table. Additional characters require bitmap data.
- Different ST7735S modules may require changes to scan direction, offsets, or initialization parameters.
- No physical flashing or distance calibration was performed as part of this documentation update.

Display initialization and troubleshooting details are available in the [LCD troubleshooting document, in Chinese](docs/LCD显示故障排查与修改说明.md).

## License and Publication

The root [LICENSE](LICENSE) contains the Apache License 2.0 text. Bundled third-party sources from STMicroelectronics, ARM, and others retain their own copyright and license notices; see the corresponding files.

`.gitignore` excludes build artifacts, local Keil settings, and common sensitive configuration files. The audit on October 7, 2026 found no common credential patterns, but historical commits still contained the original author's personal information. Updating author comments does not remove that history. Review the [pre-publication audit, in Chinese](docs/仓库上传前敏感信息检查.md) before publishing.
