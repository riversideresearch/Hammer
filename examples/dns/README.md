# DNS Packet Parser

## Usage
Build
```shell
make
```

Testing all packets
```shell
cd tests && python3 runtests.py
```
Testing individual packets
```shell
./dns_parser tests/<directory>/<packet_binary>
```
ex.
```shell
./dns_parser tests/good_packets/full_response_pass.bin
```