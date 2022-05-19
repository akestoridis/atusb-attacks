# atusb-attacks

Modified ATUSB firmware that supports selective jamming and spoofing attacks


## Disclaimer

This repository contains implementations of proof-of-concept attacks against Zigbee networks, Thread networks, and other IEEE 802.15.4-based networks, which are made available for benign research purposes only.
The users of these implementations are responsible for making sure that they are compliant with their local laws and that they have proper permission from the affected network owners.


## Implemented Attacks

| Attack ID | Attack Description[<sup>†</sup>](#dagger) | Required Macros |
| --------- | ----------------------------------------- | --------------- |
| 00 | Ignore RX\_START interrupts; equivalent to the original ATUSB firmware |  |
| 01 | Jam only Network Update commands |  |
| 02 | Spoof a MAC acknowledgment for each 12-byte Data Request of a specified network | PANID |
| 03 | Jam only packets of a specified network that request a MAC acknowledgment | PANID |
| 04 | Jam only packets of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment | PANID |
| 05 | Jam only Rejoin Responses of a specified network | PANID |
| 06 | Jam only Rejoin Responses and Network Update commands |  |
| 07 | Jam only 28-byte beacons, whose EPID matches with the 32 least-significant bits of the specified EPID | EPID |
| 08 | Jam only 28-byte beacons, whose EPID matches with the 32 least-significant bits of the specified EPID, and Network Update commands | EPID |
| 09 | Jam only Rejoin Responses and Network Update commands with a MAC acknowledgment being spoofed for each jammed Rejoin Response |  |
| 10 | Jam only Network Update commands and spoof a MAC acknowledgment for each 12-byte Data Request of a specified network | PANID |
| 11 | Jam only 12-byte MAC commands of a specified network that request a MAC acknowledgment | PANID |
| 12 | Jam only 12-byte MAC commands of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment followed by a 127-byte NWK Data packet | PANID, SHORTDSTADDR, SHORTSRCADDR, FRAMECOUNTER, EXTENDEDSRCADDR, KEYSEQNUM |
| 13 | Jam only certain 12-byte MAC commands of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment followed by a 127-byte NWK Data packet, according to specified active and idle time intervals, with the active period restarting whenever a period of inactivity is observed and the idle period restarting whenever certain packet types are observed | PANID, SHORTDSTADDR, SHORTSRCADDR, FRAMECOUNTER, EXTENDEDSRCADDR, KEYSEQNUM, ACTIVESEC, IDLESEC |
| 14 | Jam only 22-byte MAC commands of a specified network that request a MAC acknowledgment | PANID |
| 15 | Jam only certain 22-byte MAC commands of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment followed by a 127-byte MAC Data packet, according to specified active and idle time intervals, with the active period restarting whenever a period of inactivity is observed | PANID, SHORTDSTADDR, SHORTSRCADDR, FRAMECOUNTER, KEYINDEX, ACTIVESEC, IDLESEC |
| 16 | Jam only certain 22-byte MAC commands of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment followed by a 127-byte MLE command, according to specified active and idle time intervals, with the active period restarting whenever a period of inactivity is observed | PANID, EXTENDEDDSTADDR, EXTENDEDSRCADDR, UDPCHECKSUM, FRAMECOUNTER, KEYSOURCE, KEYINDEX, ACTIVESEC, IDLESEC |
| 17 | Jam only certain 22-byte MAC commands of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment followed by a 124-byte first fragment, according to specified active and idle time intervals, with the active period restarting whenever a period of inactivity is observed | PANID, EXTENDEDDSTADDR, EXTENDEDSRCADDR, DATAGRAMTAG, ACTIVESEC, IDLESEC |
| 18 | Jam only certain 22-byte MAC commands of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment followed by a 124-byte subsequent fragment, according to specified active and idle time intervals, with the active period restarting whenever a period of inactivity is observed | PANID, EXTENDEDDSTADDR, EXTENDEDSRCADDR, DATAGRAMTAG, ACTIVESEC, IDLESEC |
| 19 | Jam only beacons of a specified network, each of which is at least 45 bytes in length | PANID |
| 20 | Jam only Discovery Responses of a specified network | PANID |
| 21 | Jam only beacons of a specified network, each of which is at least 45 bytes in length, and Discovery Responses of the same network | PANID |
| 22 | Jam only beacons of a specified network, each of which is at least 45 bytes in length, unless the MAC source address corresponds to the specified extended address | PANID, EXTENDEDSRCADDR |
| 23 | Jam only Discovery Responses of a specified network, unless the MAC source address corresponds to the specified extended address | PANID, EXTENDEDSRCADDR |
| 24 | Jam only beacons of a specified network, each of which is at least 45 bytes in length, and Discovery Responses of the same network, unless the MAC source address corresponds to the specified extended address | PANID, EXTENDEDSRCADDR |
| 25 | Jam only 124-byte unsecured 6LoWPAN first fragments of a specified network that use the specified UDP source and destination ports, unless the MAC addresses correspond to the specified extended addresses in either direction | PANID, EXTENDEDDSTADDR, EXTENDEDSRCADDR, UDPSRCPORT, UDPDSTPORT |
| 26 | Jam only 124-byte unsecured 6LoWPAN first fragments of a specified network that use the specified UDP source and destination ports, unless the MAC addresses correspond to the specified extended addresses in either direction, and then spoof a MAC acknowledgment | PANID, EXTENDEDDSTADDR, EXTENDEDSRCADDR, UDPSRCPORT, UDPDSTPORT |

<a name="dagger"><sup>†</sup></a>For more information, please refer to the source code files in the `fw/attacks` folder.


## Instructions

The following set of instructions was tested on [Debian 10.3](https://cdimage.debian.org/mirror/cdimage/archive/10.3.0/amd64/iso-cd/) with an [ATUSB](http://shop.sysmocom.de/products/atusb) and was compiled using information from the following sources:
* <http://projects.qi-hardware.com/index.php/p/ben-wpan/source/tree/805db6ebf5d80692158acadf88e239da9d3e67af/atusb/fw/README>
* <http://lists.en.qi-hardware.com/pipermail/discussion/2011-May/007993.html>
* <http://lists.en.qi-hardware.com/pipermail/discussion/2011-May/007994.html>
* <http://lists.en.qi-hardware.com/pipermail/discussion/2011-May/007995.html>
* <http://lists.en.qi-hardware.com/pipermail/discussion/2011-May/007996.html>
* <http://downloads.qi-hardware.com/people/werner/wpan/prod/flash.html>
* <http://en.qi-hardware.com/wiki/Ben_WPAN>
* <http://dfu-util.sourceforge.net/>
* <http://wpan.cakelab.org/>
* <https://github.com/linux-wpan/wpan-tools/wiki/Driver-Features>
* <https://www.bastibl.net/reactive-zigbee-jamming/>

Executing `$ lsusb`, after plugging the ATUSB into a Linux host machine, should display a message similar to the following:
```console
Bus 001 Device 006: ID 20b7:1540 Qi Hardware ben-wpan, AT86RF230-based
```

Install wpan-tools to configure the ATUSB from the host machine:
```console
$ sudo apt update
$ sudo apt install wpan-tools
```

Install Git to clone repositories:
```console
$ sudo apt update
$ sudo apt install git
```

Install an AVR toolchain to compile firmware images for the ATUSB:
```console
$ sudo apt update
$ sudo apt install avr-libc gcc-avr binutils-avr
```

Install version 0.7 of dfu-util to flash firmware images on the ATUSB:
```console
$ sudo apt update
$ sudo apt-get build-dep dfu-util
$ sudo apt install libusb-1.0-0-dev
$ cd
$ git clone https://git.code.sf.net/p/dfu-util/dfu-util.git
$ cd dfu-util/
$ git checkout v0.7
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```

The source code of all the implemented attacks is included in the master branch of this repository, which can be retrieved as follows:
```console
$ cd
$ git clone https://github.com/akestoridis/atusb-attacks.git
$ cd atusb-attacks/
$ cd fw/
```

Execute the following commands to compile and flash the firmware image that launches the attack with ID 01, which will print a disclaimer and the user will have to accept responsibility for their actions if they want to proceed:
```console
$ make clean
$ sudo make dfu ATTACKID=01
```

If the flashing process failed, unplug your ATUSB from your host machine and retry while the LED of your ATUSB is turned on immediately after plugging it into your host machine again.

After successfully compiling and flashing the firmware image, executing the following command should display the configuration of the ATUSB as an IEEE 802.15.4 interface, including its phyname (e.g., `phy1`) and its devname (e.g., `wpan0`):
```console
$ iwpan dev
```

To launch the attack of the flashed firmware image, create a new interface (e.g., `attack0`) in monitor mode and enable it on the appropriate channel (e.g., channel 20 on page 0) with the following commands:
```console
$ sudo iwpan dev wpan0 del
$ sudo iwpan phy phy1 interface add attack0 type monitor
$ sudo iwpan phy phy1 set channel 0 20
$ sudo ip link set attack0 up
```

To stop the attack, disable the new interface as follows:
```console
$ sudo ip link set attack0 down
```

Alternatively, to compile and flash the firmware image that launches the attack with ID 05 against the network with PAN ID 0x99aa, execute the following commands:
```console
$ make clean
$ sudo make dfu ATTACKID=05 PANID=0x99aa
```

Similarly, to compile and flash the firmware image that launches the attack with ID 07 against the network with EPID 0xfacefeedbeefcafe, execute the following commands:
```console
$ make clean
$ sudo make dfu ATTACKID=07 EPID=0xfacefeedbeefcafe
```

To only compile the firmware image that launches the attack with ID 01, execute the following commands:
```console
$ make clean
$ make ATTACKID=01
```

The attack with ID 00 is equivalent to the original ATUSB firmware, which can be used to sniff IEEE 802.15.4 packets with the same sequence of commands as the other attacks and [`tcpdump`](https://www.tcpdump.org/) to store them in a pcap file.

Whenever the user executes a compilation or flashing command, a disclaimer will be printed and they will have to accept responsibility for their actions in order to proceed.


## Related Publications

* D.-G. Akestoridis, V. Sekar, and P. Tague, “On the security of Thread networks: Experimentation with OpenThread-enabled devices,” in *Proc. ACM WiSec’22*, 2022, pp. 233–244, doi: [10.1145/3507657.3528544](https://doi.org/10.1145/3507657.3528544).
* D.-G. Akestoridis and P. Tague, “HiveGuard: A network security monitoring architecture for Zigbee networks,” in *Proc. IEEE CNS’21*, 2021, pp. 209–217, doi: [10.1109/CNS53000.2021.9705043](https://doi.org/10.1109/CNS53000.2021.9705043).
* D.-G. Akestoridis, M. Harishankar, M. Weber, and P. Tague, “Zigator: Analyzing the security of Zigbee-enabled smart homes,” in *Proc. ACM WiSec’20*, 2020, pp. 77–88, doi: [10.1145/3395351.3399363](https://doi.org/10.1145/3395351.3399363).


## Acknowledgments

This project was supported in part by the Carnegie Mellon CyLab Security and Privacy Institute and in part by Carnegie Mellon University.


## License

Copyright 2007 S. Salewski\
Copyright 2008-2011, 2013-2015 Werner Almesberger\
Copyright 2015-2016 Stefan Schmidt\
Copyright 2017 Josef Filzmaier\
Copyright 2020-2022 Dimitrios-Georgios Akestoridis

This repository includes modified source code from the [ben-wpan repository](http://projects.qi-hardware.com/index.php/p/ben-wpan/).
More specifically, the initial commit of this repository includes a copy of the [atusb/fw folder](http://projects.qi-hardware.com/index.php/p/ben-wpan/source/tree/805db6ebf5d80692158acadf88e239da9d3e67af/atusb/fw) from commit 805db6ebf5d80692158acadf88e239da9d3e67af of the ben-wpan repository.
The files of this repository are licensed under the [same terms](http://projects.qi-hardware.com/index.php/p/ben-wpan/source/tree/805db6ebf5d80692158acadf88e239da9d3e67af/COPYING) as the files of the ben-wpan repository.
