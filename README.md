# rpi-nfc-game-load
Loading games through NFC cards using the RPI and a PN532


## Setup

### Raspberry

- [Install RetroPie](https://retropie.org.uk/docs/First-Installation/)
- Quit emulation station
- sudo raspi-config
- [Activate SPI interface](https://learn.sparkfun.com/tutorials/raspberry-pi-spi-and-i2c-tutorial/all): for NFC reader
- [Activate SSH interface](https://retropie.org.uk/docs/SSH/): for easy rom loading and compiling

### PN532

#### Jumper Positions

| I1  | I0  |
| :-: | :-: |
|  H  |  L  |


| SCK | MISO | MOSI | NSS | SCL | SDA | RX  | TX  |
| :-: | :--: | :--: | :-: | :-: | :-: | :-: | :-: |
| ON  |  ON  |  ON  | ON  | OFF | OFF | OFF | OFF |

- RSTDPDN | D20

## Start Game

To start a game using emulation station, use the `runcommand.sh`.

```
/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ snes /path/to/ROM
```

This can also be down in C via 

[`system`:](https://man7.org/linux/man-pages/man3/system.3.html)

```c
const char* system_command = "/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ console /path/to/ROM";
system(system_command)
```

or [`execcl`:](https://linux.die.net/man/3/execl)
```c
execl("/opt/retropie/supplementary/runcommand/runcommand.sh",
      "runcommand.sh", 
      "0", 
      "_SYS_", 
      "console",
      "/path/to/rom", 
      (char *)NULL);
```

## Dependencies

### SDL

```
sudo apt install libsdl2-dev libsdl2-ttf-dev
```

### WiringPi

```
cd ~
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi
./build debian
mv debian-template/wiringpi-3.x.deb .
sudo apt install ./wiringpi-3.x.deb
```

### Linking

- libsdl2-dev
- libsdl2-ttf-dev
- pthread
- wiringPi

```
-lwiringPi -lSDL2 -lSDL2_ttf -pthread
```

These have already been added in the `src/Makefile`
