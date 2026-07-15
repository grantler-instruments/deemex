# deemex

MIDI to DMX512 interface for Teensy 4.x (USB MIDI) and ESP32 (ESP-NOW MIDI).
It can also emulate an Enttec DMX Pro over serial.

**Teensy:** USB type Serial+MIDI, TeensyDMX on Serial5.

**ESP32:** Install the [ESP-NOW-MIDI](https://github.com/grantler-instruments/ESP-NOW-MIDI) library, wire Grove DMX512 (TX=GPIO 21, DE=GPIO 4).

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
