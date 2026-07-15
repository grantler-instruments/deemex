# deemex

MIDI to DMX512 interface for Teensy 4.x (USB MIDI) and ESP32 (ESP-NOW MIDI).
It can also emulate an Enttec DMX Pro over serial.

**Teensy:** USB type Serial+MIDI, TeensyDMX on Serial5.

**ESP32:** Install the [ESP-NOW-MIDI](https://github.com/grantler-instruments/ESP-NOW-MIDI) library, wire Grove DMX512 (TX=GPIO 21, DE=GPIO 4).

## MIDI mapping

DMX can be driven by **note on/off** or by **14-bit control-change pairs**. Both paths are always active and write the same DMX universe; if they target the same DMX channel, the last write wins.

MIDI channel numbers below are 1-based (1–16), as in MIDI.

### Notes

**Accepted MIDI channels:** `start` … `start + 4`, where `start` is the persisted `noteOnStartChannel` setting (default **1**, allowed 1–16). Channels outside that window are ignored. MIDI only has channels 1–16, so if `start + 4 > 16`, only channels `start`…`16` ever arrive.

**Accepted notes:** 1–127 (note 0 is ignored).

```
DMX channel = (MIDI channel − start) × 127 + note
DMX value   = velocity × 2          // note on; note off writes 0
```

Only DMX channels in **1–512** are written; larger results are dropped.

| property | detail |
|---|---|
| Value range | 0, 2, 4, …, 254 (velocity 0–127) — odd levels and 255 are unreachable |
| Note off | same address formula; value forced to 0 (note-off velocity ignored) |

With the default `start = 1`, MIDI channels **1–5** cover DMX **1–512** (on channel 5, only notes 1–4 map to 509–512; notes 5–127 are dropped).

Full coverage always needs five MIDI channels, so the highest workable `start` is **12**. Higher values leave the top DMX channels unreachable via notes:

| `start` | MIDI channels | DMX via notes |
|---|---|---|
| 1 (default) | 1–5 | 1–512 |
| 12 | 12–16 | 1–512 |
| 13 | 13–16 | 1–508 |
| 16 | 16 | 1–127 |

**Examples (`start = 1`):**

| MIDI | → DMX |
|---|---|
| ch 1, note 1 | 1 |
| ch 1, note 127 | 127 |
| ch 2, note 1 | 128 |
| ch 5, note 1 | 509 |
| ch 5, note 4 | 512 |

### Control change (14-bit)

No start-channel setting. Addressing is fixed from MIDI channel **1** through **16** (32 DMX channels per MIDI channel → full universe).

**Intended controllers:** for each DMX slot, send a pair on the same MIDI channel:

- **MSB:** CC `n` (0 ≤ `n` ≤ 31)
- **LSB:** CC `n + 32` (32 ≤ CC ≤ 63)

Written only after both halves for that slot have been received:

```
index       = (MIDI channel − 1) × 32 + n          // 0…511
DMX channel = index + 1                            // 1…512
fullValue   = (MSB ≪ 7) | LSB                     // 14-bit, 0…16383
DMX value   = fullValue ≫ 6                       // 8-bit, 0…255
```

Equivalent form: `DMX value = (MSB ≪ 1) | (LSB ≫ 6)`. The low 6 bits of the LSB are discarded (64 consecutive 14-bit encodings → each level `V`).

**Pairing rules:**

- MSB and LSB may arrive in either order.
- If more than **20 ms** elapses since the last half for that slot, the pending half is cleared and pairing restarts.
- After a successful write, both halves are cleared; the next update needs a fresh pair.
- A single unpaired CC does not change DMX.

**Set exact DMX level `V` (0–255):**

```
MSB = V ≫ 1
LSB = (V & 1) ≪ 6
```

(or any LSB whose top bit matches `(V & 1)`). Example: `V = 200` → MSB `100`, LSB `0`.

**Address examples:**

| MIDI | Controllers | → DMX |
|---|---|---|
| ch 1 | CC 0 + CC 32 | 1 |
| ch 1 | CC 31 + CC 63 | 32 |
| ch 2 | CC 0 + CC 32 | 33 |
| ch 16 | CC 31 + CC 63 | 512 |

## Contributing

Bug reports, test feedback, and pull requests are welcome via the [GitHub issue tracker](https://github.com/grantler-instruments/deemex/issues).

1. **Bugs** — Open an issue with your board (Teensy 4.x or ESP32), build details, and steps to reproduce.
2. **Testing** — If you have hardware, try changes on your setup and note what you tested in the issue or PR.
3. **Pull requests** — Fork the repo, create a branch, and open a PR against `main` with a clear description of the change.

By contributing, you agree that your contributions will be licensed under the same terms as this project ([AGPL-3.0](LICENSE)).

## Support

If you find deemex useful, you can support development:

[![Buy Me A Coffee](https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png)](https://buymeacoffee.com/thomasgeissl)

## License

This project is licensed under the [GNU Affero General Public License v3.0](LICENSE) (AGPL-3.0).
