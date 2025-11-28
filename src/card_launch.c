//-----------------------------------------------------------------------------
#include "pn532.h"
#include "pn532_rpi.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "gamelist.h"
//-----------------------------------------------------------------------------
// typedef struct {
//   uint8_t card_id[4];
//   const char *console;
//   const char *filename;
//   const char *title;
// } Game;
// //-----------------------------------------------------------------------------
// Game gamelist[] = {
//     {.card_id = {0X4B, 0xEB, 0x08, 0x25},
//      .console = "nes",
//      .title = "Teenage Mutant Ninja Turtles",
//      .filename = "turtles.nes"},
//     {.card_id = {0x80, 0xc4, 0x93, 0x97},
//      .console = "nes",
//      .title = "The Great Gatsby",
//      .filename = "gatsby.nes"},
// };

const char *path = "/opt/retropie/supplementary/runcommand/runcommand.sh 0 "
                   "_SYS_ nes /home/pi/RetroPie/roms/nes/";
char system_command[sizeof(path) + 100];
const char *system_command_format =
    "/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ %s "
    "/home/pi/RetroPie/roms/%s/%s";

//-----------------------------------------------------------------------------
typedef enum {
  FADE_IN,
  FADE_OUT,
  NO_FADE,
  FADE_OUT_END,
  FADE_IN_END,
} FADE_STATE;

FADE_STATE fade_state = NO_FADE;
//-----------------------------------------------------------------------------
typedef enum {
  WELCOME_MESSAGE,
  SHOW_GAME,
} MENU_STATE;
MENU_STATE menu_state = WELCOME_MESSAGE;
//-----------------------------------------------------------------------------
uint32_t windowHeight = 600;
uint32_t windowWidth = 800;

volatile uint32_t shared_value = 0;
volatile uint32_t prev_value = 1;
volatile uint8_t searchingForCard = 1;
volatile uint8_t cardFound = 0;
volatile uint8_t cardReaderReady = 0;
pthread_mutex_t lock; // mutex to protect access

uint32_t print_value = 0;
uint8_t value_updated = 0;
uint8_t swapTexture = 0;

const char *resultTextFormat = "%x";
char resultText[40];
//-----------------------------------------------------------------------------
void *poll_card_reader(void *arg) {
  uint8_t buff[255];
  uint8_t uid[MIFARE_UID_MAX_LENGTH];
  int32_t uid_len = 0;

  PN532 pn532;
  PN532_SPI_Init(&pn532);

  if (PN532_GetFirmwareVersion(&pn532, buff) == PN532_STATUS_OK) {
    printf("Found PN532 with firmware version: %d.%d\r\n", buff[1], buff[2]);
  } else {
    return NULL;
  }

  PN532_SamConfiguration(&pn532);
  printf("Waiting for RFID/NFC card...\r\n");
  cardReaderReady = 1;

  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
    printf("Desktop resolution: %dx%d\n", dm.w, dm.h);
  }

  while (searchingForCard) {
    pthread_mutex_lock(&lock); // lock before modifying
    uid_len =
        PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000);

    if (uid_len != PN532_STATUS_ERROR) {
      if (menu_state == WELCOME_MESSAGE) {
        menu_state = SHOW_GAME;
      } else {
        cardFound = 1;
        shared_value = *(uint32_t *)uid;
      }
    }

    pthread_mutex_unlock(&lock); // unlock after modifying
    usleep(50 * 1000);           // 50 ms
  }

  return NULL;
}
//-----------------------------------------------------------------------------
void *print_result(void *arg) {

  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
    printf("Desktop resolution: %dx%d\n", dm.w, dm.h);
  }

  while (searchingForCard) {
    pthread_mutex_lock(&lock); // lock before accessing
    if (cardFound && prev_value != shared_value) {

      sprintf(resultText, resultTextFormat, shared_value);
      printf("%s\n\r", resultText);
      prev_value = shared_value;
      print_value = shared_value;
      value_updated = 1;
      swapTexture = 1;

      cardFound = 0;
    }
    pthread_mutex_unlock(&lock); // unlock after accessing
    usleep(10 * 1000);
  }

  return NULL;
}
//-----------------------------------------------------------------------------

int main() {
  //---------------------------------------------------------------------------
  // Threading
  pthread_t poll_card_reader_thread;
  pthread_t print_result_thread;

  // Initialize mutex
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("Mutex init failed\n");
    return 1;
  }

  // Create threads
  if (pthread_create(&poll_card_reader_thread, NULL, poll_card_reader, NULL)) {
    printf("Thread creation failed\n");
    return 1;
  }

  if (pthread_create(&print_result_thread, NULL, print_result, NULL)) {
    printf("Thread creation failed\n");
    return 1;
  }

  //---------------------------------------------------------------------------
  // SDL Setup
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
  TTF_Init();

  SDL_Window *window =
      SDL_CreateWindow("SDL Text Example", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0);

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
    printf("Desktop resolution: %dx%d\n", dm.w, dm.h);
  }

  SDL_ShowCursor(SDL_DISABLE);

  windowWidth = dm.w;
  windowHeight = dm.h;
  //---------------------------------------------------------------------------
  // Font
  // Load font (adjust path and size)
  TTF_Font *font =
      TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", 48);
  if (!font) {
    SDL_Log("Failed to load font: %s", TTF_GetError());
    return 1;
  }
  SDL_Color color = {255, 255, 255, 255}; // white
  //---------------------------------------------------------------------------
  // Welcome Message
  const int numLines = 4;
  const char *welcomeMessageLines[] = {
      "Merry Christmas Rowan and Sandy!",
      "Love from Mummy and Daddy x x x",
      "",
      "Insert a Card To Play",
  };

  SDL_Texture *welcomeMessageTextures[numLines];
  SDL_Rect welcomeMessageDest[numLines];
  welcomeMessageDest[0].x = 0; // X position on screen
  welcomeMessageDest[0].y = 0; // Y position on screen
  for (int i = 0; i < numLines; i++) {

    SDL_Surface *welcomeMessageSurface =
        TTF_RenderText_Blended(font, welcomeMessageLines[i], color);
    welcomeMessageTextures[i] =
        SDL_CreateTextureFromSurface(renderer, welcomeMessageSurface);
    SDL_FreeSurface(welcomeMessageSurface);

    int texw;
    int texh;
    SDL_QueryTexture(welcomeMessageTextures[i], NULL, NULL, &texw, &texh);
    welcomeMessageDest[i].x = (windowWidth - texw) / 2;
    welcomeMessageDest[i].y = (windowHeight / 2) + (texh * (i - 1));
    welcomeMessageDest[i].w = texw;
    welcomeMessageDest[i].h = texh;
    // welcomeMessageDest[i].y = ((windowHeight - welcomeMessageDest.h) / 2) + i
    // * welcomeMessageDest.h;
  }

  //---------------------------------------------------------------------------
  // Text

  SDL_Texture *textTextures[2];
  uint8_t textureIndex = 0;

  SDL_Surface *textSurf =
      TTF_RenderText_Blended(font, gamelist[0].title, color);
  textTextures[0] = SDL_CreateTextureFromSurface(renderer, textSurf);
  SDL_FreeSurface(textSurf);

  textSurf = TTF_RenderText_Blended(font, gamelist[1].title, color);
  textTextures[1] = SDL_CreateTextureFromSurface(renderer, textSurf);
  SDL_FreeSurface(textSurf);

  SDL_Texture *currentTexture = textTextures[textureIndex];

  SDL_Rect textDest;

  SDL_QueryTexture(currentTexture, NULL, NULL, &textDest.w, &textDest.h);
  textDest.x = (windowWidth - textDest.w) / 2; // X position on screen
  textDest.y = 4 * windowHeight / 5;           // Y position on screen
  //---------------------------------------------------------------------------
  // Cover Art
  // Load the BMP file
  SDL_Texture *currentCoverTexture;
  SDL_Texture *coverTextures[2];
  SDL_Surface *coverSurface = SDL_LoadBMP("turtles.bmp");
  if (!coverSurface) {
    SDL_Log("Failed to load BMP: %s", SDL_GetError());
    return 1;
  }

  // Convert surface to texture
  int imgW[2];
  int imgH[2];
  imgW[0] = coverSurface->w;
  imgH[0] = coverSurface->h;
  coverTextures[0] = SDL_CreateTextureFromSurface(renderer, coverSurface);
  SDL_SetTextureBlendMode(coverTextures[0], SDL_BLENDMODE_BLEND);
  SDL_FreeSurface(coverSurface);

  coverSurface = SDL_LoadBMP("gatsby.bmp");
  if (!coverSurface) {
    SDL_Log("Failed to load BMP: %s", SDL_GetError());
    return 1;
  }
  imgW[1] = coverSurface->w;
  imgH[1] = coverSurface->h;
  coverTextures[1] = SDL_CreateTextureFromSurface(renderer, coverSurface);
  SDL_SetTextureBlendMode(coverTextures[1], SDL_BLENDMODE_BLEND);
  SDL_FreeSurface(coverSurface);

  // Set the display size you want
  SDL_Rect coverDest;
  coverDest.x = (windowWidth - imgW[textureIndex]) / 2;  // X position on screen
  coverDest.y = (windowHeight - imgH[textureIndex]) / 2; // Y position on screen
  coverDest.w = imgW[textureIndex];                      // Width to draw
  coverDest.h = imgH[textureIndex];                      // Height to draw

  currentCoverTexture = coverTextures[textureIndex];
  //---------------------------------------------------------------------------

  SDL_Event e;
  int running = 1;

  char gameTitleText[40];

  uint16_t alpha = 0;
  uint8_t alphaStep = 5;
  FADE_STATE fade_state = FADE_IN;

  while (running) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_QUIT:
      case SDL_KEYDOWN:
        running = 0;
        break;
      }
    }

    SDL_RenderClear(renderer);

    switch (menu_state) {
    case WELCOME_MESSAGE:

      for (int i = 0; i < numLines - 1; i++) {
        SDL_RenderCopy(renderer, welcomeMessageTextures[i], NULL,
                       &welcomeMessageDest[i]);
      }
      if (cardReaderReady) {
        SDL_RenderCopy(renderer, welcomeMessageTextures[numLines - 1], NULL,
                       &welcomeMessageDest[numLines - 1]);
      }

      break;

    case SHOW_GAME:

      switch (fade_state) {
        // use SDL_GetTicks for a consistent duration
      case FADE_IN:
        alpha += alphaStep;
        if (alpha >= 255) {
          alpha = 255;
          fade_state = FADE_IN_END;
        }
        SDL_SetTextureAlphaMod(currentTexture, alpha);
        SDL_SetTextureAlphaMod(currentCoverTexture, alpha);
        break;
      case FADE_OUT:
        alpha -= alphaStep;
        if (alpha <= 0) {
          alpha = 0;
          fade_state = FADE_OUT_END;
        }
        SDL_SetTextureAlphaMod(currentTexture, alpha);
        SDL_SetTextureAlphaMod(currentCoverTexture, alpha);
        break;
      }

      if (value_updated) {

        switch (fade_state) {
        case FADE_IN_END:
          if (swapTexture) {

            uint8_t swapTextureIndex = (textureIndex == 1) ? 0 : 1;
            sprintf(gameTitleText, "%s", gamelist[swapTextureIndex].title);
            textSurf = TTF_RenderText_Blended(font, gameTitleText, color);

            if (textTextures[swapTextureIndex])
              SDL_DestroyTexture(textTextures[swapTextureIndex]);

            textTextures[swapTextureIndex] =
                SDL_CreateTextureFromSurface(renderer, textSurf);
            SDL_FreeSurface(textSurf);

            swapTexture = 0;
            fade_state = FADE_OUT;
          }
          break;
        case FADE_OUT_END:
          textureIndex = (textureIndex == 1) ? 0 : 1;
          currentTexture = textTextures[textureIndex];
          currentCoverTexture = coverTextures[textureIndex];
          SDL_SetTextureAlphaMod(currentTexture, 0);
          SDL_SetTextureAlphaMod(currentCoverTexture, 0);
          SDL_QueryTexture(currentTexture, NULL, NULL, &textDest.w,
                           &textDest.h);

          textDest.x = (windowWidth - textDest.w) / 2;
          coverDest.x = (windowWidth - imgW[textureIndex]) / 2;
          coverDest.y = (windowHeight - imgH[textureIndex]) / 2;
          coverDest.w = imgW[textureIndex];
          coverDest.h = imgH[textureIndex];

          value_updated = 0;
          fade_state = FADE_IN;
          break;
        }
      }

      SDL_RenderCopy(renderer, currentTexture, NULL, &textDest);
      SDL_RenderCopy(renderer, currentCoverTexture, NULL, &coverDest);
      break;
    }
    SDL_RenderPresent(renderer);
  }
  //---------------------------------------------------------------------------
  pthread_mutex_lock(&lock); // lock before accessing
  searchingForCard = 0;
  pthread_mutex_unlock(&lock); // unlock after accessing

  // Wait for the thread to finish
  pthread_join(poll_card_reader_thread, NULL);
  pthread_join(print_result_thread, NULL);

  pthread_mutex_destroy(&lock);

  printf("Final Card ID: %x\n", shared_value);
  //---------------------------------------------------------------------------
  // SDL Teardown
  // Hide cursor
  // system("setterm -cursor off");

  // // Make background black
  // system("setterm -blank force");

  // // Clear the console fully
  // system("clear > /dev/tty1");

  // // Disable kernel messages to console
  // system("dmesg -n 1");

  SDL_DestroyTexture(textTextures[0]);
  SDL_DestroyTexture(textTextures[1]);
  SDL_DestroyTexture(coverTextures[0]);
  SDL_DestroyTexture(coverTextures[1]);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  //---------------------------------------------------------------------------
  // system("stty sane");
  // //---------------------------------------------------------------------------
  // int gameNum = textureIndex;
  // sprintf(system_command, system_command_format, gamelist[gameNum].console,
  //         gamelist[gameNum].console, gamelist[gameNum].filename);
  // printf("%s\n", system_command);
  // system(system_command);
  //---------------------------------------------------------------------------
  return 0;
}
//-----------------------------------------------------------------------------

// // 1. Hide console + disable keyboard
// system("setterm -cursor off");
// system("stty -echo -icanon");

// // 2. Switch to a clean VT
// system("chvt 2");

// // 3. Shutdown SDL
// SDL_DestroyRenderer(renderer);
// SDL_DestroyWindow(window);
// SDL_Quit();

// // 4. Launch emulator
// pid_t pid = fork();
// if (pid == 0) {
//     execl("/opt/retropie/supplementary/runcommand/runcommand.sh",
//           "runcommand.sh", "0", "_SYS_", "nes",
//           "/home/pi/RetroPie/roms/nes/game.nes",
//           (char*)NULL);
//     exit(1);
// }
// waitpid(pid, NULL, 0);

// // 5. Restore VT + terminal
// system("chvt 1");
// system("stty echo icanon");
// system("setterm -cursor on");

// // 6. Reinitialize SDL
// SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);