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
- `psg/driver/` (`PSG::Driver`): bus timing for latch address / write / read / reset (the
  strictest delays of all supported chip datasheets; the write pulse runs with interrupts off),
  Timer2 clock setup (the real clock of the divider is cached). Direct port access, no Arduino API.
- `psg/mode/psg_basic` (`PSG::Basic`): power on, reset (clears all registers, used to mute), clock
  (clamped to 1-2 MHz, `GetClock` gives the real clock), direct register writes and reads
  (`0x00-0x0F`), and chip detection: `compute_chipid` runs write/read tests (incl. AY8930 expanded
  mode) and hashes the read-back values into a `ChipId` enum value. Changing the tests or the bus
  timing can change the hashes: then the `ChipId` constants must be measured again on the chips.
- `psg/mode/psg_advanced` (`PSG::Advanced`): hides the methods of `Basic` (no virtual calls).
  `m_input` is the state the host wrote (all 25 registers: bank A `0x00-0x0D`, AY8930 bank B
  `0x10-0x1A`), read back by `GetRegister`. `SetRegister` takes addresses `0x00-0x0F` and follows
  the bank selected through R13 (expanded mode only on AY8930: `0xA0`/`0xB0`). `Commit()` copies
  the input to `m_output`, processes it (clock conversion as in psg-tools `ChipClockRateConvert`,
  stereo channel remapping with only ABC/ACB in expanded mode, the AY8930 pure-envelope fix) and
  writes only the registers that differ from the previous `m_output`, which mirrors the chip, plus
  the envelope shapes the host wrote. The write order is that of psg-tools `ChipEncoder`. Features
  can be compiled out with the `DISABLE_*` defines at the top of `psg_advanced.h` (debug only).
- `psg/mode/psg_turbosound` (`PSG::TurboSound<PSGMode>`): two chips of `Basic` or `Advanced` mode
  behind one API, no direct access to them. `PowerOn` detects the board (`SingleChip` or
  `TurboSound`) apart from the chip types; `Select(0/1)` sets the current chip; the chip is
  selected on the bus by a latch address cycle `0xFF`/`0xFE` (only when it changes). Clock and
  reset are common for both chips. On a single chip board the calls for chip 1 are ignored; a chip
  that is not found still gets the data. `Commit` and `SetStereo` exist only for `Advanced`.
- `uart/uart_driver` (`UART::Driver`): USART0 at 57600 8N1 (U2X, 57143 baud). The RX ISR puts
  bytes into a 128-byte ring buffer (one-byte indexes, no `cli`); lost bytes (frame error,
  overrun, full buffer) increment an error counter. `Write` sends a byte and waits for the
  transmitter (short replies only, no TX buffer).
- `uart/uart_protocol`: `UART::Parser` turns the received bytes into events in the main loop.
  `0xFF` is a frame (the previous frame is committed, so the chips follow the host frame timer,
  one frame late) and selects chip 0, `0xFE` selects chip 1, then `addr, data` pairs go to
  `SetRegister`. A bad address or a driver error skips to the next `0xFF`. A printable char where
  an address is expected starts a text command, `\r`/`\n` ends it (16 chars max, `0xFF` aborts
  it). The protocol knows only the config (see below); other texts go to the text handler that
  the streamer passes to `Start`. Timer1 runs free at F_CPU/1024 (no ISR): when no frame comes
  for 100 ms the chips are reset (muted). `UART::Protocol<PSGMode>` sends the events to
  `TurboSound<Basic>` or `TurboSound<Advanced>`; the mode differences are overloads (`Commit`,
  `Configure`). It also gives the streamer `Configure` (no reply), `GetConfig` (the config in
  use), `SendInfo`, `GetChips`, and text output helpers (`WriteText`, `WriteHex`, `WriteConfig`,
  `WriteLineEnd`, `ChipName`).
- `firmware.ino` is the streamer: it picks the type with `using PSGMode = PSG::Advanced;` (or
  `PSG::Basic`), two firmwares to study the hardware and the host processing. Its text handler
  has the terminal commands, not a part of the protocol: `info` (the config reply without a
  change), `chip` (a line `E6D5F1AE - AY8910`, the `ChipId` hash and the type, for each chip),
  `save` (the config in use to EEPROM, for Basic the real clock and ABC, with a magic byte and a
  checksum; replies the saved config, e.g. `ABC1750000`). At the start a valid saved config is
  applied, for hosts that do not send one; a host config overrides it until the next start.

Keep the code free of Arduino runtime classes (`Serial`, `delay`, ...) where the old code avoided
them; use registers directly. Store constant tables in `PROGMEM`.

## Related project: psg-tools (host side)

psg-tools lives next to this repo at `D:\projects\github\psg-tools` (C++17, Windows, its own
`CLAUDE.md`). **Never commit in psg-tools**: its CLAUDE.md requires leaving changes uncommitted.

The host side of the serial protocol is
`psg-tools/psg-tools/psg_audio/sources/player/streamer/Streamer.cpp`. The firmware must match it,
and a protocol change must be made on both sides:
- The host opens the port at **57600 8N1**, DTR and RTS disabled. With RTS enabled by the driver,
  opening the port resets the MCU: the firmware is ready after the bootloader and `PowerOn`.
- The config: before each track (`DeviceInit`) the host sends `ABC1773400\n`, the stereo and the
  clock of the destination chip. The streamer replies with 4 lines (`\r\n`): its type
  (`Advanced`/`Basic`), the stereo and the clock it uses (`ACB1773400`), the type of chip 0 and of
  chip 1 (`NotFound`, `Compatible`, `AY8910`, `AY8930`, `YM2149`, `Unknown`). Advanced takes both
  and processes the stream; Basic only sets the nearest clock divider and replies its real clock
  and `ABC`. After open the host sends the config again every 250 ms for up to 3 s (the MCU may
  still start); a streamer that does not reply is not asked again. Its last info, if it sent one
  (also unasked, e.g. once after the start), is kept; without any, the host uses 1.7734 MHz, ABC
  and one chip (AYPlayer, AVR-AY and other simple streamers).
- The host sets its destination chip from the reply: the clock (its own `ChipClockRateConvert`
  then converts to it), the models (the second chip only for two-chip tracks, not when it is
  `NotFound`). When the reply stereo is the one sent, the host does not reorder the channels
  (`Output::DeviceRemapsChannels`), else it does it itself.
- Each frame (paced by the host at the song's frame rate, usually 50 Hz) is sent as one packet:
  for each chip a marker `0xFF` (chip 0) or `0xFE` (chip 1, TurboSound), followed by `addr, data`
  pairs of the changed registers only.
- Addresses are `0x00-0x0F` only. AY8930 bank B is reached through R13 bank switching, written by
  psg-tools' `ChipEncoder` (R13 first on a mode change, else last, after returning to bank A). Every
  R13 write restarts envelope A on the chip.
- On close the host writes an empty (silent) frame.
- AYPlayer (`software/AYPlayer_v137`) sends only the frames (`0xFF` and pairs of registers 0-13),
  no text: the firmware must keep working without the config.
