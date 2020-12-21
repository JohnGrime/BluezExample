# DBus Bluez monitoring example

This is a simple example of monitoring Bluetooth-related data over [`DBus`](https://www.freedesktop.org/wiki/Software/dbus/) using [`Bluez`](http://www.bluez.org/) on Linux.

Access to the DBus interface comes as standard in [`gio`](https://developer.gnome.org/gio/stable/), which is easily available via `apt`, e.g.:

```
sudo apt install libgio3.0-cil-dev
```

The code can be built using `g++` and `pkg-config`, e.g.:

```
g++ -std=c++14 -Wall -Wextra -pedantic -O2 $(pkg-config --cflags gio-2.0) example.cpp DBusBluez.cpp $(pkg-config --libs gio-2.0) -o bluez
```

The example program will listen for `ObjectManager` messages for an optional period until quitting (e.g., `./a.out 10` will run for 10 seconds and then quit). If no period is specified, the program runs continually until interrupted.