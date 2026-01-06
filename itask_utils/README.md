# ITASK: Your code here

## Set up

```bash
sudo apt update
sudo apt install wireshark

chmod +x itask_utils/init_host.sh
./itask_utils/init_host.sh

make clean && make qemu # -j 8
```

...

## JOSI --- JOS OSI (Overview)

```
+-----------+-----+-----------------+
|HTTP |xxxxx|xxxxx|xxxxxxxxxxxxxxxxx|
+-----------+-----+-----------------+
| TCP | UDP |ICMP |xxxxxxxxxxxxxxxxx|
+-----------+-----+-----------------+
|        IP       |       ARP       |
+-----------------+-----------------+
|             Ethernet              |
+-----------------------------------+
|           e1000 driver            |
+-----------------------------------+
```

## TCP/IP

```
Application		layer:	[HTTP
Transport		layer:	[TCP / UDP
Internet		layer:	[IP / ICMP
Link			layer:	[ARP
						[Ethernet
						[e1000 driver
```

## OSI

```
Application		layer:	[HTTP
Presentation	layer:	[ASCII
Session			layer:	[xxxxxxxxx
Transport		layer:	[TCP / UDP
Network			layer:	[IP / ICMP
Data link		layer:	[ARP
						[Ethernet
Physical		layer:	[e1000 driver?
```

