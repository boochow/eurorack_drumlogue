# Lillian: braids for drumlogue

Lillian is a drumlogue port of braids.

- `synth.h` is the main code of this port; it works as the replacement of  `eurorack/braids/braids.cc`.
- to set the sample rate to 48 KHz, the following files of braids are modified:
  - `eurorack/braids/resources/lookup_tables.py`
  - `eurorack/braids/resources/waveforms.py`
- `linenvelope.h` is an extended version of `eurorack/braids/envelope.h`
