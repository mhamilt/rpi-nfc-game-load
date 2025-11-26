#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "pn532.h"
#include "pn532_rpi.h"
 
int main(void) {
    system("date +%A");
    system("gcc --version");

    system("/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ nes /home/pi/RetroPie/roms/nes/turtle.nes");
    
}