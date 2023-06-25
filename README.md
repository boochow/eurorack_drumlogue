# drumlogue ports of eurorack

Drumlogue ports of [Mutable Instruments eurorack modules](https://github.com/pichenettes/eurorack).

* [Lillian](http://mutable-instruments.net/modules/braids): a port of braids

To build:

clone this repository under `logue-sdk/platform/drumlogue/` then
```
cd eurorack_drumlogue
git submodule update --init --recursive
cd lillian
make
```
