#  TFTP Packet Parser

This program will parse TFTP packets to check their validity.

Parser implementation based on [RFC 1350](https://www.rfc-editor.org/rfc/rfc1350).

Testing was done with hand typed hex packets, and is not yet comprehensive.


## Build and Run

- Prerequisites:
```shell
sudo apt install build-essential
```

- One-liner using `make`:
```shell
make clean && make && make run
```
