# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this
repository.

## Rules

- Never commit to this repository (no `git commit`, `git push`, amend, merge, or rebase), even when
  a task seems finished. The user reviews every change and commits it themselves, so leave all
  changes uncommitted in the working tree. The same rule holds in psg-tools.
- Before a commit, the user runs the `prepare-commit` skill (`.claude/skills/prepare-commit/`).

## What this is

AYM Streamer is a USB device that plays a real AY-3-8910 / AY-3-8912 / YM2149F / AY8930 sound chip
(PSG). A PC streams register writes to it over a serial port, the firmware writes them to the chip,
and the chip's analog output goes to a stereo jack. The PC side is **psg-tools** (see below).

- `firmware/`: the current firmware (Arduino sketch, ATmega328P). This is where work happens.
- `firmware_old/`: earlier attempts, reference only; do not build or change them.
  - `firmware-v1.0`: plain UART receive (register, data) into a ring buffer, no frames.
  - `firmware-v2.1`: 57600 baud, a 5 ms Timer1 ISR drains the buffer, `0xFF` marks a frame and
    calls `Update()`, text commands and a chip info banner. The current `firmware/` grew out of it.
  - `firmware-pio`: a PlatformIO sketch of a `PowerSG` library.
- `hardware/v1.2/`: the current board (schematic PDF, BOM, Gerbers). `v1.1` is the previous one.
- `datasheet/`: MCU, CH340N, NE5532 and all supported PSG datasheets (`datasheet/sound/`).
- `software/`: old prebuilt Windows tools (AYPlayer, an old PSGTools build), reference only.

## Hardware (v1.2)

- ATmega328P at 16 MHz (crystal), CH340N USB-to-serial on USB-C. CH340 RTS# drives the MCU reset
  through a capacitor, so opening the port resets the MCU like an Arduino (Uno/Nano style).
- One PSG: a DIP-40 socket (AY-3-8910 / YM2149F / AY8930) or a DIP-28 socket (AY-3-8912), never
  both. BC2 and A8 are tied high, A9# low.
- Pin map (`firmware/src/psg/driver/psg_wiring.h`, matches the schematic): DA0-DA3 = PC0-PC3,
  DA4-DA7 = PD4-PD7, BC1 = PB0, BDIR = PB1, RESET# = PD2, CLOCK = PD3 (OC2B; Timer2 makes the PSG
  clock from 16 MHz by an integer divider, so e.g. 1.7734 MHz is really 16/9 = 1.7778 MHz).
- LEDs: RXD and TXD are on the CH340 lines; the PSG LED is on PSG_RES# (lit while the chip is out
  of reset).
- Analog: the three channels are mixed to L/R by resistors and buffered by an NE5532.
- J1 EXP header: VBUS, VCC, GND, I2C (SDA/SCL on PC4/PC5), ADC6, ADC7. J2 is ICSP. Free MCU pins
  beyond these: PB2-PB5 (SPI, on ICSP).

## Build

There is no `arduino-cli`. Arduino IDE 1.8.19 is installed at `C:\Program Files (x86)\Arduino`
(avr-gcc 7.3.0, AVR core 1.8.3). Compile from the command line with its `arduino-builder`, run from
the IDE folder:

```bash
cd "/c/Program Files (x86)/Arduino" && ./arduino-builder.exe -compile -hardware hardware -tools tools-builder -tools hardware/tools/avr -built-in-libraries libraries -fqbn arduino:avr:uno -build-path "$(cygpath -w "$TEMP")\\aym-build" -warnings all 'D:\projects\github\aym-streamer\firmware\firmware.ino'
```

- The build path must exist (`mkdir -p`); `-build-cache` speeds up rebuilds of the core.
- It prints flash and RAM usage at the end. RAM is 2 KB: watch the global variables figure.
- Arduino compiles `firmware.ino` plus everything under `firmware/src/` recursively; includes are
  relative (`"src/psg/mode/psg_advanced.h"`, `"../driver/psg_driver.h"`).
- There are no tests and no linter. Checking on hardware is done by the user.

## Firmware architecture (`firmware/src/`)

Layered, each layer only uses the one below:
- `psg/driver/` (`PSG::Driver`): bus timing for latch address / write / read / reset (delays from
  the datasheets), Timer2 clock setup. Direct port access, no Arduino API.
- `psg/mode/psg_basic` (`PSG::Basic`): power on, reset, clock (clamped to 1-2 MHz), direct register
  writes, and chip detection: `compute_chipid` runs write/read tests (incl. AY8930 expanded mode)
  and hashes the read-back values into a `ChipId` enum value.
- `psg/mode/psg_advanced` (`PSG::Advanced`): a shadow copy of all 25 registers (bank A
  `0x00-0x0D`, AY8930 bank B `0x10-0x1A`). `SetRegister` takes addresses `0x00-0x0F` and follows
  the bank selected through R13 (`exp_mode` `0xA0`/`0xB0`); `Commit()` processes the changed state
  and writes only changed registers to the chip: clock conversion (virtual vs real clock), stereo
  channel remapping (ABC/ACB/...), the AY8930 pure-envelope fix, and bank switching for AY8930.
  Features can be compiled out with the `DISABLE_*` defines at the top of `psg_advanced.h`.
- `uart/`: `uart_driver` (USART0, interrupt RX into a ring buffer) and `uart_protocol` (parse the
  host stream). Both files are still empty: this is the next piece of work.

Keep the code free of Arduino runtime classes (`Serial`, `delay`, ...) where the old code avoided
them; use registers directly. Store constant tables in `PROGMEM`.

## Related project: psg-tools (host side)

psg-tools lives next to this repo at `D:\projects\github\psg-tools` (C++17, Windows, its own
`CLAUDE.md`). **Never commit in psg-tools**: its CLAUDE.md requires leaving changes uncommitted.

The host side of the serial protocol is
`psg-tools/psg-tools/psg_audio/sources/player/streamer/Streamer.cpp`. The firmware must match it,
and a protocol change must be made on both sides:
- On open, if the port's RTS control is enabled, the host reads a text banner at **9600** baud
  (`ReciveText`), then switches to **57600 8N1**, DTR and RTS disabled.
- Each frame (paced by the host at the song's frame rate, usually 50 Hz) is sent as one packet:
  for each chip a marker `0xFF` (chip 0) or `0xFE` (chip 1, TurboSound), followed by `addr, data`
  pairs of the changed registers only.
- Addresses are `0x00-0x0F` only. AY8930 bank B is reached through R13 bank switching, written by
  psg-tools' `ChipEncoder` (R13 first on a mode change, else last, after returning to bank A). Every
  R13 write restarts envelope A on the chip.
- On close the host writes an empty (silent) frame.
- The host assumes a 1.7734 MHz chip and stereo ABC unless the user chooses otherwise; it does all
  clock conversion and stereo remapping itself, so the firmware does not need to repeat it for
  psg-tools streams.
