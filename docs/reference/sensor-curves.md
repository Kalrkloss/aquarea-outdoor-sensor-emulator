# Aquarea outdoor sensor characteristics

Two different NTCs feed the same physical quantity on a Panasonic Aquarea. Values
are from the service manuals (section 12.4 appendix for the optional sensors,
section 18.5.42 for the built-in one). Do not merge them.

## Optional external sensor (indoor PCB input)

`PAW-A2W-TSOD`, also the characteristic of `PAW-A2W-TSRT` (room) and
`PAW-A2W-TSSO` (solar). 6.523 kΩ @ 25 °C, B25/50 ≈ 3485 K.

| T °C | R kΩ | T °C | R kΩ |
|---|---|---|---|
| 30 | 5.326 | −5 | 25.05 |
| 25 | 6.523 | −10 | 32.10 |
| 20 | 8.044 | −15 | 41.45 |
| 15 | 9.980 | −20 | 53.92 |
| 10 | 12.443 | −25 | 70.53 |
| 5 | 15.604 | −30 | 93.05 |
| 0 | 19.70 | −35 | 124.24 |
| | | −40 | 167.82 |

## Built-in sensor (outdoor unit, CN-TH1, fault code F36)

≈ 15.1 kΩ @ 25 °C, B ≈ 3961 K — a different sensor, roughly 2.5× the resistance
at the same temperature.

| T °C | R kΩ |
|---|---|
| 50 | 5 |
| 40 | 8 |
| 30 | 13 |
| 20 | 19 |
| 10 | 30 |
| 0 | 51 |
| −10 | 88 |

## How to tell them apart in five minutes

Unplug the sensor, put it in ice water (0 °C) and measure:

- **51 kΩ** → built-in sensor at the outdoor unit
- **19.7 kΩ** → optional external sensor

## Controlling the value

`Installer → System setup → step 10 "Alternative outdoor sensor"` (default `No`).
Set `Yes` when an external sensor is installed: *"Controlled by optional outdoor
sensor without reading the outdoor sensor of heat pump unit"*. The alternative
input is **not** error-checked, so it will not hide a dead primary sensor — the
outdoor unit still judges its own sensor and still raises F36.
