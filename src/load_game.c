#include "pn532.h"
#include "pn532_rpi.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
  system("date +%A");
  system("gcc --version");

  pid_t pid = fork();
  if (pid == 0) {
    // Child process: replace with runcommand
    execl("/opt/retropie/supplementary/runcommand/runcommand.sh",
          "runcommand.sh", "0", "_SYS_", "nes",
          "/home/pi/RetroPie/roms/nes/turtles.nes", (char *)NULL);

    // If execl returns, something failed:
    perror("execl failed");
    exit(1);
  }

  // Parent process: wait until emulator finishes
  int status;
  waitpid(pid, &status, 0);

  //   system("/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ nes
  //   "
  //          "/home/pi/RetroPie/roms/nes/turtle.nes");
}