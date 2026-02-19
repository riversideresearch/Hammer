# NTP Packet Parser

Refer to [steps.md](packet/steps.md) in `packet/` to see how the sample NTP packet was captured.

One-liner Makefile command to build and run:

```shell
make clean && make && make testp
```

## User-defined Test Inputs

The program now reads raw bytes as input. To use an existing hex-string format, see below:

* String Input

```shell
echo "<string>" | xxd -r -p | ./ntp_parser
```

* Raw Byte Input

```shell
./ntp_parser <raw byte file>
```
