# ALDL-IO for Raspberry Pi 5

A maintained Raspberry Pi 5 friendly fork of **ALDL-IO**, a small C-based datalogger and dashboard tool for GM OBD-I / ALDL 8192 baud ECMs.

This fork is based on **ALDL-IO 1.6.2** from the ECMHack Raspberry Pi dashboard/datalogger release, with build fixes and small runtime hardening changes for modern Raspberry Pi OS / Raspberry Pi 5 systems.

Original project:

- Project page: <https://ecmhack.com/misc/raspberry-pi-dashboarddatalogger/>
- Original GitHub repository: <https://github.com/BulldogDrummond/aldl>

The original upstream repository appears inactive. This fork preserves the original license and attribution.

## What this does

ALDL-IO talks to GM 8192 baud OBD-I ECMs through an FTDI-based USB ALDL cable and can:

- request ALDL mode 1 datastream packets,
- decode ECM data using text configuration files,
- log selected values to CSV,
- show a simple ncurses dashboard,
- run a dummy ECM backend for testing without a vehicle.

Main binaries:

| Binary | Purpose |
| --- | --- |
| `aldl-ftdi` | Real FTDI / ALDL interface |
| `aldl-dummy` | Fake ECM backend for testing configs and logging |
| `aldl-tty` | Unfinished generic TTY backend from upstream |
| `aldl` | Installed symlink to `aldl-ftdi` |

## Raspberry Pi 5 changes

This fork includes Raspberry Pi 5 build fixes tested on modern Raspberry Pi OS.

### Build dependencies

Install these packages:

```bash
apt install build-essential libftdi1-dev libncurses-dev libtinfo-dev
```

### Makefile updates

The Makefile has been updated for current libraries:

- uses `libftdi1` instead of the older FTDI library naming,
- links with `-lftdi1`,
- links ncurses wide/tinfo libraries:
  - `-lncurses`
  - `-lncursesw`
  - `-ltinfo`
- builds:
  - `aldl-ftdi`
  - `aldl-tty`
  - `aldl-dummy`
- installs binaries to `/usr/local/bin`, configs to `/etc/aldl`, logs to `/var/log/aldl`.

Build:

```bash
make clean
make
```

Install as root:

```bash
make install
```

## Configuration files

Default install paths:

| Path | Purpose |
| --- | --- |
| `/etc/aldl/aldl.conf` | Main configuration file |
| `/etc/aldl/datalogger.conf` | CSV logger configuration |
| `/etc/aldl/consoleif.conf` | ncurses dashboard layout |
| `/etc/aldl/lt1.conf` | Original LT1 example definition |
| `/etc/aldl/6E.conf` | Minimal 1227165 / 6E example definition |
| `/var/log/aldl` | Default CSV log output directory |

The main config selects the ECM definition file:

```ini
DEFINITION=/etc/aldl/lt1.conf
```

For the LB9 / ECM 1227165 / mask 6E work in this fork, see:

```text
config-examples/lb9.conf
```

That file is generated from TunerPro 1227165 / 6E material and should be treated as an initial working definition, not final calibration gospel.

## Important datalogger note

`LOG_FILENAME` is a **file prefix**, not a directory.

Good:

```ini
LOG_FILENAME=/var/log/aldl/aldl-autolog-%Y-%m-%d-
```

This creates files like:

```text
/var/log/aldl/aldl-autolog-2026-05-10-00001.csv
```

Bad:

```ini
LOG_FILENAME=/home/user/aldllogs
```

That would create files beside the directory path, not inside it, because ALDL-IO appends `00001.csv`, `00002.csv`, etc. directly to the prefix.

## Config parser fix in this fork

Original ALDL-IO parsed every `PARAM=value` pair anywhere in a config file, including examples inside comment lines.

That meant this was unsafe:

```ini
-- LOG_FILENAME=/var/log/aldl/old-prefix
LOG_FILENAME=/var/log/aldl/new-prefix-%Y-%m-%d-
```

The parser could pick the commented `LOG_FILENAME` first and silently ignore the intended active value. Very old-school. Not in a charming way.

This fork fixes that in `loadconfig.c`:

- added `dfile_strip_comments()`;
- whole-line comments are stripped before `PARAM=value` parsing;
- supported comment line prefixes after whitespace:
  - `-`
  - `.`
  - `#`
  - `;`
  - `*`
  - `/*`

The fix was verified with a parser test where multiple commented `LOG_FILENAME=` entries are ignored and the active value is selected.

## Running without a vehicle

Use the dummy backend:

```bash
/usr/local/bin/aldl-dummy
```

For a config-only check:

```bash
/usr/local/bin/aldl-dummy configtest
```

The dummy backend is useful for validating build, config parsing, datalogging and service behavior before touching the real ECM.

## Running with FTDI / real ALDL cable

Use:

```bash
/usr/local/bin/aldl-ftdi
```

The FTDI backend uses raw libftdi access. The Linux kernel `ftdi_sio` driver may need to be unloaded or blacklisted for the ALDL cable, depending on the system and adapter.

The original project includes:

```text
debian-config.sh
```

Review it before use. Do not run random driver-changing scripts blindly on a car computer. Computers are serious tools.

## Optional systemd service

For unattended datalogging, run `aldl-dummy` or `aldl-ftdi` as a service and keep screen/ncurses output out of log files.

Recommended service behavior:

- service starts at boot,
- `ExecStart=/usr/local/bin/aldl-ftdi` for real vehicle use,
- `ExecStart=/usr/local/bin/aldl-dummy` for bench testing,
- `CONSOLEIF_ENABLE=0`,
- `DATALOGGER_ENABLE=1`,
- `StandardOutput=null`,
- `StandardError=journal`,
- CSV logs go to `/var/log/aldl`.

Daily CSV rotation can be handled by `logrotate` using `/var/log/aldl/*.csv`.

## LB9 / ECM 1227165 / mask 6E notes

This fork includes early work toward a proper LB9 / 1227165 / 6E config:

```text
config-examples/lb9.conf
```

Current status:

- generated from `1227165_6E.adx`,
- packet base follows existing `6E.conf`,
- includes common live values like RPM, TPS, coolant temp, battery voltage, speed, MAF, LV8, O2, BLM, INT, BPW, spark and knock retard,
- skips AutoProm-only channels outside the ECM packet,
- keeps some unsupported ADX conversions as raw values where ALDL-IO cannot express lookup tables or reciprocal equations.

Known limitations:

- ALDL-IO only supports simple linear conversions: `X * multiplier + adder`;
- lookup tables from ADX are not expressible in stock ALDL-IO config syntax;
- reciprocal formulas such as target AFR are currently represented as raw values;
- real ECM / FTDI testing is still required.

## License

This fork keeps the original ALDL-IO license intact. See [`LICENSE`](LICENSE).

Original copyright:

```text
Copyright(c) 2014 Steve Haslin
```

Redistribution and modification are permitted under the conditions listed in the license file.

## Repository status

This is a pragmatic Raspberry Pi 5 compatibility fork of legacy ALDL-IO code. It is useful now, but it remains old C with a simple custom config parser and old assumptions about Linux systems.

For new integrations, this fork can act as:

- a working reference implementation,
- a datalogging fallback,
- a source of verified ECM definitions,
- a bridge while a modern ALDL Python service is developed.
