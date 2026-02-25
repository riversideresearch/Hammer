#  TFTP Packet Parser

Authored by Matthew Kinnison, you can reach me on Teams or email @ mkinniosn@riversideresearch.org

Here's a link to a tutorial write-up about this file: https://confluence.riversideresearch.org/display/TRS/TFTP+Tutorial

This program will parse TFTP packets to check their validity.

Testing was done with hand typed hex packets, and is not yet comprehensive. 
For the syntax of a packet, please refer to the tutorial linked above.


## Build and Run

- Prerequisites:
```shell
sudo apt install build-essential
```

- One-liner using `make`:
```shell
make clean && make && make run
```
