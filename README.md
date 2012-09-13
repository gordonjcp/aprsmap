aprsmap
=======

An APRS mapping application developed by Gordon JC Pearce (MM0YEQ) and
others, with key goals of being easier to use and looking prettier
than the standard Amateur Radio software. The first in a series of
applications based around the APRS protocol.

This application is currently in the earliest stages of development, and
is not representative of the quality of the final application, if we
ever get there.

It requires osm-gps-map libraries, and libfap libraries. If you're using
Arch Linux, you can install both of these from AUR. Debian and Ubuntu
users may need to install libsoup2.4-dev due to a packaging bug.

aprsmap uses waf for building. To build:

    ./waf configure
    ./waf

To run:
    ./build/aprsmap
