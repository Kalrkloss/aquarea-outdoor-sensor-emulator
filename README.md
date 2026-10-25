# Aquarea outdoor sensor emulator

Replaces the optional external ambient sensor of a Panasonic Aquarea air-to-water
heat pump with a controlled voltage, so the outdoor temperature the heat pump
controls on can come from several shaded sensors instead of the sun-exposed
sensor behind the front panel.

**Status: design.** Nothing here has been built or measured on hardware yet.
The numeric tables are complete and self-tested; the firmware does not exist.

## How it works

```
Zigbee outdoor sensors ──► Zigbee2MQTT / Home Assistant
                                    │  aggregate (min / median, filtered)
                                    ▼
                            genAnalogOutput PresentValue (writable)
                                    │
                             ESP32-C6 (Zigbee router)
                                    │  I2C
                                    ▼
                            MCP4726 (12 bit DAC)
                                    │  V = Vcc · R(T) / (R(T) + R_pullup)
                                    ▼
              Aquarea indoor PCB – alternative outdoor sensor input
                                    ▲
                       DS18B20 fallback (local, on the ESP32-C6)
```

The ESP32-C6 holds the sensor node at the voltage the heat pump expects for a
given temperature. It does not present a resistance — the DAC drives the divider
node directly, which is why the exponential NTC curve never has to be reproduced
in hardware: the divider linearises it.

## Why

The mono-bloc outdoor unit carries its ambient sensor behind the front panel,
which takes direct sun. Panasonic provides a second input on the indoor PCB for
an alternative ambient sensor — `Installer → System setup → step 10
"Alternative outdoor sensor"`, default `No`. When it is enabled, the indoor unit
controls on that value and stops reading the built-in sensor
(`docs/reference/sensor-curves.md`).

Note what this does **not** fix: the outdoor unit keeps using its own sensor for
defrost and for fault code F36. An emulated value corrects the heating curve, not
the defrost decision.

## Safety

This project touches a mains-powered appliance control board.

- The sensor input's logic ground is **not** potential-free. Never bring a PC,
  USB port or earthed instrument ground to it. The ESP32-C6 has its radio on the
  same board, which is the main reason this design uses no wired control link.
- Power the emulator from the unit's own 5 V rail or through an isolated DC/DC
  (e.g. B0505S). Check the rail can supply the radio's TX peaks — the ESP32-C6
  draws on the order of 100 mA in bursts.
- Switch the appliance off at the breaker before touching the indoor PCB.
- Whatever you feed the input, the heat pump will act on it: a too-high value
  trips the outdoor thermo-OFF logic and stops heating in winter, a too-low one
  starts anti-freeze pump runs. Hence the clamp, the slew limit and the
  fail-safe relay described in `docs/architecture.md`.

## Repository layout

| path | content |
|---|---|
| `docs/hardware.md` | sensor curve, divider maths, MCP4726 configuration, wiring, fail-safe relay |
| `docs/architecture.md` | Zigbee role, aggregation and filtering rules, priorities, failure matrix |
| `docs/commissioning.md` | measure the pull-up, enable the menu setting, two-point calibration, rollback |
| `docs/reference/sensor-curves.md` | vendor characteristic tables for both Aquarea outdoor inputs |
| `firmware/aquarea_tsod_dac.h` | C99: vendor curve, 12-bit code LUT, conversion functions |
| `firmware/aquarea_tsod_dac.csv` | −40.0…+45.0 °C in 0.1 K steps: R, DAC code, V/Vcc, K per LSB |

## Design notes

- **Curve source.** Panasonic service-manual characteristic table
  (6.523 kΩ @ 25 °C, B25/50 ≈ 3485 K), interpolated log-linearly in 1/T, so the
  vendor nodes are reproduced exactly and the deviation in between stays below
  0.1 K. A single fitted beta would be 1–3 % off at the ends.
- **Ratiometric.** The DAC references its own Vdd and the board's ADC is
  ratiometric to the same rail, so the code is a pure ratio and Vcc cancels:
  `code = 4095 · R / (R + R_pullup)`. The pull-up does *not* cancel — 1 % error
  is 0.24 K — so it has to be measured, not assumed.
- **Resolution.** 12 bit gives 0.041 K per step over −25…+40 °C (0.075 K at
  −40 °C); 8 bit would be 0.66 K. 10 bit (0.08 K) is the floor for this job.
- **Why not a digital rheostat.** A rheostat has to reproduce the exponential
  curve with a linear element, which concentrates the resolution problem exactly
  where the curve is steepest, and it cannot reach the >100 kΩ the cold end
  needs. The divider voltage spans only 0.27…0.94 V/V across −40…+40 °C.

## Open items

- Firmware (ESP-Zigbee-SDK, router role, write callback → DAC, NVS, DS18B20).
- Zigbee2MQTT external converter and the Home Assistant aggregation automation.
- Placement study for the DS18B20: behind the unit it measures air the evaporator
  has already chilled, not the ambient temperature.
- Fail-safe relay that returns the input to a real PAW-A2W-TSOD when the emulator
  is unpowered.

## License

MIT — see `LICENSE`.
