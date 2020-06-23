# atusb-attacks

Modified ATUSB firmware that supports selective jamming and spoofing attacks


## Disclaimer

This repository contains implementations of proof-of-concept attacks against Zigbee networks and other IEEE 802.15.4-based networks, which are made available for benign research purposes only.
The users of these implementations are responsible for making sure that they are compliant with their local laws and that they have proper permission from the affected network owners.


## Implemented Attacks

| Attack ID | Attack Description                                                                                                                 | Required Macro |
| --------- | -----------------------------------------------------------------------------------------------------------------------------------| -------------- |
| 00        | Do not jam or spoof any packets; equivalent to the original firmware                                                               |                |
| 01        | Jam only Network Update commands                                                                                                   |                |
| 02        | Spoof a MAC acknowledgment for each 12-byte Data Request of a specified network                                                    | PANID          |
| 03        | Jam only packets of a specified network that request a MAC acknowledgment                                                          | PANID          |
| 04        | Jam only packets of a specified network that request a MAC acknowledgment and then spoof a MAC acknowledgment                      | PANID          |
| 05        | Jam only Rejoin Responses of a specified network                                                                                   | PANID          |
| 06        | Jam only Rejoin Responses and Network Update commands                                                                              |                |
| 07        | Jam only 28-byte beacons, whose EPID matches with the 32 least-significant bits of the specified EPID                              | EPID           |
| 08        | Jam only 28-byte beacons, whose EPID matches with the 32 least-significant bits of the specified EPID, and Network Update commands | EPID           |
| 09        | Jam only Rejoin Responses and Network Update commands with a MAC acknowledgment being spoofed for each jammed Rejoin Response      |                |


## Instructions

* The following set of instructions was tested on [Debian 10.3](https://cdimage.debian.org/mirror/cdimage/archive/10.3.0/amd64/iso-cd/) with an [ATUSB](http://shop.sysmocom.de/products/atusb) and was compiled using information from the following sources:
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

* Executing `$ lsusb`, after plugging the ATUSB into a Linux host machine, should display a message similar to the following:
```
Bus 001 Device 006: ID 20b7:1540 Qi Hardware ben-wpan, AT86RF230-based
```

* Install wpan-tools to configure the ATUSB from the host machine:
```
$ sudo apt update
$ sudo apt install wpan-tools
```

* Install Git to clone repositories:
```
$ sudo apt update
$ sudo apt install git
```

* Install an AVR toolchain to compile firmware images for the ATUSB:
```
$ sudo apt update
$ sudo apt install avr-libc gcc-avr binutils-avr
```

* Install version 0.7 of dfu-util to flash firmware images on the ATUSB:
```
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

* The source code of all the implemented attacks is included in the master branch of this repository, which can be retrieved as follows:
```
$ cd
$ git clone https://github.com/akestoridis/atusb-attacks.git
$ cd atusb-attacks/
$ cd fw/
```

* Execute the following commands to compile and flash the firmware image that launches the attack with ID 01, which will print a disclaimer and the user will have to accept responsibility for their actions if they want to proceed:
```
$ make clean
$ sudo make dfu ATTACKID=01
```

* After compiling and flashing the firmware image, executing the following command should display the configuration of the ATUSB as an IEEE 802.15.4 interface, including its phyname (e.g., `phy1`) and its devname (e.g. `wpan0`).
```
$ iwpan dev
```

* To launch the attack of the flashed firmware image, create a new interface (e.g., `attack0`) in monitor mode and enable it on the appropriate channel (e.g., channel 20 on page 0) with the following commands:
```
$ sudo iwpan dev wpan0 del
$ sudo iwpan phy phy1 interface add attack0 type monitor
$ sudo iwpan phy phy1 set channel 0 20
$ sudo ip link set attack0 up
```

* To stop the attack, disable the new interface as follows:
```
$ sudo ip link set attack0 down
```

* Alternatively, to compile and flash the firmware image that launches the attack with ID 05 against the network with PANID 0x99aa, execute the following commands:
```
$ make clean
$ sudo make dfu ATTACKID=05 PANID=0x99aa
```

* Similarly, to compile and flash the firmware image that launches the attack with ID 07 against the network with EPID 0xfacefeedbeefcafe, execute the following commands:
```
$ make clean
$ sudo make dfu ATTACKID=07 EPID=0xfacefeedbeefcafe
```

* To only compile the firmware image that launches the attack with ID 01, execute the following commands:
```
$ make clean
$ make ATTACKID=01
```

* The attack with ID 00 is equivalent to the original ATUSB firmware, which can be used to sniff IEEE 802.15.4 packets with the same sequence of commands as the other attacks and `tcpdump` to store them in a PCAP file.

* Whenever the user executes a compilation or flashing command, a disclaimer will be printed and they will have to accept responsibility for their actions in order to proceed.


## License

Copyright 2007 S. Salewski\
Copyright 2008-2011, 2013-2015 Werner Almesberger\
Copyright 2015-2016 Stefan Schmidt\
Copyright 2017 Josef Filzmaier\
Copyright 2020 Dimitrios-Georgios Akestoridis

This repository includes modified source code from the [ben-wpan repository](http://projects.qi-hardware.com/index.php/p/ben-wpan/).
More specifically, the initial commit of this repository includes a copy of the [atusb/fw folder](http://projects.qi-hardware.com/index.php/p/ben-wpan/source/tree/805db6ebf5d80692158acadf88e239da9d3e67af/atusb/fw) from commit 805db6ebf5d80692158acadf88e239da9d3e67af of the ben-wpan repository.
The files of this repository are licensed under the [same terms](http://projects.qi-hardware.com/index.php/p/ben-wpan/source/tree/805db6ebf5d80692158acadf88e239da9d3e67af/COPYING) as the files of the ben-wpan repository.
