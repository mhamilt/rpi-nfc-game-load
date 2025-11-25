# rpi-nfc-game-load
Loading games through NFC cards using the RPI and a PN532


## Setup

## Jumper Positions

| I1  | I0  |
| :-: | :-: |
|  H  |  L  |



| SCK | MISO | MOSI | NSS | SCL | SDA | RX  | TX  |
| :-: | :--: | :--: | :-: | :-: | :-: | :-: | :-: |
| ON  |  ON  |  ON  | ON  | OFF | OFF | OFF | OFF |

### Script

```
cd ~
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi
./build debian
mv debian-template/wiringpi-3.x.deb .
sudo apt install ./wiringpi-3.x.deb

cd ~
git clone https://github.com/soonuse/pn532-lib
sudo nano pn532-lib/example/raspberrypi/rpi_get_uid.c
```

Change lines to

```
PN532_SPI_Init(&pn532);
//PN532_I2C_Init(&pn532);
//PN532_UART_Init(&pn532);
```

```
cd pn532-lib/example/raspberrypi/ && make
```

## Start Game

```
/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ snes /path/to/ROM
```