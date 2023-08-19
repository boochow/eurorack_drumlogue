# drumlogue ports of eurorack

Drumlogue ports of [Mutable Instruments eurorack modules](https://github.com/pichenettes/eurorack).

* [Lillian](http://mutable-instruments.net/modules/braids): a port of braids

[![top-page](https://img.youtube.com/vi/81GxDqZyT9k/0.jpg)](https://youtu.be/81GxDqZyT9k)

To build:

clone this repository under `logue-sdk/platform/drumlogue/` then
```
cd eurorack_drumlogue
git submodule update --init --recursive
cd lillian
make
```
