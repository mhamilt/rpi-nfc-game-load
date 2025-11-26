//
// Basic Launch a game using NFC
//
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "pn532.h"
#include "pn532_rpi.h"

typedef struct
{
    uint8_t     card_id[4];
    const char* console;
    const char* filename;
} Game;

const uint8_t numGames = 2;
const char* path = "/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ nes /home/pi/RetroPie/roms/nes/";
char system_command[sizeof(path) + 100];
const char* system_command_format = "/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ %s /home/pi/RetroPie/roms/%s/%s";

Game gamelist[numGames] = {
    {.card_id =  {0x80, 0xc4, 0x93, 0x97}, .console = "nes", .filename = "Gatsby.nes"},
    {.card_id =  {0X4B, 0xEB, 0x08, 0x25}, .console = "nes", .filename = "turtles.nes"}
};


int main(int argc, char** argv) {
    uint8_t buff[255];
    uint8_t uid[MIFARE_UID_MAX_LENGTH];
    
    int32_t uid_len = 0;
    PN532 pn532;
    
    PN532_SPI_Init(&pn532);
    
    if (PN532_GetFirmwareVersion(&pn532, buff) == PN532_STATUS_OK) {
        printf("Found PN532 with firmware version: %d.%d\r\n", buff[1], buff[2]);
    } else {
        return -1;
    }
    PN532_SamConfiguration(&pn532);
    printf("Waiting for RFID/NFC card...\r\n");
    while (1)
    {
        // Check if a card is available to read
        uid_len = PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000);
        if (uid_len == PN532_STATUS_ERROR) { // Waiting
            fflush(stdout);
        } else {
            
            bool gameFound = false;
            int gameNum;
            for (int i = 0; i < numGames; i++)            
            {
                if(*((uint32_t*)gamelist[i].card_id) == *((uint32_t*)uid))
                {
                    gameNum = i
                    break;
                }
            }
            
            if(gameFound)
            {
                sprintf(system_command, system_command_format, gamelist[i].console, gamelist[i].console, gamelist[i].filename);
                printf("%s\n", system_command);
            }
            else
            {
                printf("Card Not Recognised\r\n\r\n");
            }
            break;
        }
    }
}
