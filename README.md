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


## License

Copyright 2007 S. Salewski\
Copyright 2008-2011, 2013-2015 Werner Almesberger\
Copyright 2015-2016 Stefan Schmidt\
Copyright 2017 Josef Filzmaier\
Copyright 2020 Dimitrios-Georgios Akestoridis

This repository includes modified source code from the [ben-wpan repository](http://projects.qi-hardware.com/index.php/p/ben-wpan/).
More specifically, the initial commit of this repository includes a copy of the [atusb/fw folder](http://projects.qi-hardware.com/index.php/p/ben-wpan/source/tree/805db6ebf5d80692158acadf88e239da9d3e67af/atusb/fw) from commit 805db6ebf5d80692158acadf88e239da9d3e67af of the ben-wpan repository.
The files of this repository are licensed under the [same terms](http://projects.qi-hardware.com/index.php/p/ben-wpan/source/tree/805db6ebf5d80692158acadf88e239da9d3e67af/COPYING) as the files of the ben-wpan repository.
