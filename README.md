# aircraft_arinc_enet
EOL/RAF code support for the Alta Arinc Ethernet device (ARINC to UDP).

ADT_API/ - Contains the level 0 and level 1 library source from Alta.  And an RPM spec file.  This is downloaded from https://www.altadt.com/customers/ - requires username and password.

AltaSetup/ - Program to setup the Alta ENET device how we want it.  Main program here, forked by nidas dsm process, or run standalone.

rdAltaUDP/ - Command line utility to read and print legible UDP output from the Alta ENET.

