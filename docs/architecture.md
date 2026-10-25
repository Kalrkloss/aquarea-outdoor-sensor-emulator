# Architecture

## Zigbee role

The ESP32-C6 joins the **existing** Zigbee2MQTT network as a **router** (or a
non-sleepy end device). Never a sleepy end device: a SED collects commands only
when it polls, so a setpoint write can arrive tens of seconds late while the heat
pump runs on the old value. The device is permanently powered, so there is
nothing to save.

A Zigbee device does not receive other devices' reports. The outdoor sensors talk
to their coordinator, not to the emulator. Binding a sensor directly to the
emulator's `msTemperatureMeasurement` cluster is technically possible, but it
costs bind-table entries (2–4 on most battery sensors), silently stops when the
sensor is re-paired, and leaves no diagnostics. **Do not use it as the primary
path** — aggregate in Zigbee2MQTT / Home Assistant and write one value.

## Data flow

```
sensor reports ─► Z2M ─► HA aggregation ─► genAnalogOutput.presentValue ─► ESP32-C6
                                                                             │
                                          DS18B20 ───────────────────────────┤
                                          NVS (last good) ───────────────────┤
                                                                             ▼
                                                              clamp → slew → MCP4726
```

### Source priority

| # | source | condition |
|---|---|---|
| 1 | HA override | set and younger than 30 min — test / tuning only |
| 2 | aggregate of Zigbee sensors | only sensors whose last report is fresh |
| 3 | DS18B20 local | aggregate stale for more than 15 min |
| 4 | last good value from NVS | nothing else available |

The override must have an **expiry**. A forgotten test value otherwise heats or
blocks the house for days.

## Aggregation

Minimum over all sensors is safe against a sun-exposed sensor but expensive
against a cold pocket: one sensor in a frost pocket drags the whole system to a
higher flow temperature and costs COP, and one failed sensor reading −30 °C runs
the pump at full output. With three or more sensors prefer the **second lowest**
or the **median**; with exactly two, minimum plus the filters below.

| filter | rule | reaction |
|---|---|---|
| freshness | report older than 15 min | sensor excluded from the aggregate |
| jump | more than 5 K within 1 min | sample discarded |
| range | outside −35…+50 °C | sensor discarded |
| cross-check | more than 8 K from DS18B20 | sensor flagged, excluded |
| output clamp | −25…+40 °C | hard limit |
| slew | 1 K/min | rate limit |

Configure the sensors' minimum report interval to ≤ 5 min in Z2M; otherwise the
"minimum" is computed from hour-old values.

The clamp and the slew limit are not cosmetic. A forced high value trips the
outdoor thermo-OFF control and stops heating in winter; a forced low value starts
anti-freeze pump runs; a step of 20 K makes the heat pump chase the compensation
curve.

## Value provenance

Report back to Home Assistant, next to the applied temperature:

- which source won (override / aggregate / DS18B20 / NVS),
- the age of every sensor in the aggregate,
- the measured node voltage from the ESP32-C6 ADC.

Without this you cannot explain later why the system behaved differently on a
given day.

## Failure matrix

| failure | behaviour |
|---|---|
| HA / Z2M down | C6 holds the last setpoint; DS18B20 takes over after 15 min |
| one sensor gone | dropped from the aggregate, the rest continue |
| Zigbee link lost | same as HA down — last value, then local fallback |
| C6 firmware hangs | MCP4726 auto-recall keeps the last code, the board sees a plausible value |
| emulator unpowered | relay returns the input to a real sensor; without a relay the board reads "sensor open" |
| DS18B20 defective | CRC error, ignored; Zigbee sensors remain |
| R_pullup mis-measured | constant temperature offset — the one error the failure matrix does not catch; it shows up in the two-point calibration |

## What the emulation does not cover

The outdoor unit's own sensor is still used for defrost and for F36. Feeding the
alternative input changes the heating curve only. If the goal is to correct
defrost behaviour, this design is the wrong lever.
