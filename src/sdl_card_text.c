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

volatile uint32_t shared_value = 0;
volatile uint32_t prev_value = 1;
volatile uint8_t searchingForCard = 1;
volatile uint8_t cardFound = 0;
pthread_mutex_t lock; // mutex to protect access

const char *resultTextFormat = "%x";
char resultText[40];

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

  while (searchingForCard) {
    pthread_mutex_lock(&lock); // lock before modifying
    uid_len =
        PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000);

    if (uid_len != PN532_STATUS_ERROR) {

      cardFound = 1;
      shared_value = *(uint32_t *)uid;
    }

    pthread_mutex_unlock(&lock); // unlock after modifying
    usleep(50 * 1000);           // 50 ms
  }

  return NULL;
}

void *print_result(void *arg) {

  while (searchingForCard) {
    pthread_mutex_lock(&lock); // lock before accessing
    if (cardFound && prev_value != shared_value) {
      sprintf(resultText, resultTextFormat, shared_value);
      printf("%s\n\r", resultText);
      prev_value = shared_value;
      cardFound = 0;
    }
    pthread_mutex_unlock(&lock); // unlock after accessing
    usleep(10 * 1000);
  }

  return NULL;
}

SDL_Texture *renderText(SDL_Renderer *renderer, TTF_Font *font,
                        const char *message, SDL_Color color,
                        SDL_Rect *rectOut) {
  SDL_Surface *surf = TTF_RenderText_Solid(font, message, color);
  if (!surf)
    return NULL;

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_FreeSurface(surf);

  rectOut->x = 0; // or wherever you want
  rectOut->y = 0;
  rectOut->w = surf->w;
  rectOut->h = surf->h;

  return tex;
}

int main() {
  pthread_t poll_card_reader_thread;
  pthread_t print_result_thread;

  // Initialize mutex
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("Mutex init failed\n");
    return 1;
  }

  // Create threads
  if (pthread_create(&poll_card_reader_thread, NULL, poll_card_reader, NULL) !=
      0) {
    printf("Thread creation failed\n");
    return 1;
  }

  if (pthread_create(&print_result_thread, NULL, print_result, NULL) != 0) {
    printf("Thread creation failed\n");
    return 1;
  }

  //---------------------------------------------------------------------------
  // SDL Setup
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();

  SDL_Window *window =
      SDL_CreateWindow("SDL Text Example", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 800, 600, 0);

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Load font (adjust path and size)
  TTF_Font *font =
      TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", 48);
  if (!font) {
    SDL_Log("Failed to load font: %s", TTF_GetError());
    return 1;
  }

  // Set text color
  SDL_Color color = {255, 255, 255, 255}; // white

  // Render text surface
  char msg[128] = "Hello World!";

  SDL_Event e;
  SDL_Rect dest;
  dest.x = 100;
  dest.y = 100;

  SDL_Surface *surface = TTF_RenderText_Solid(font, msg, color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);
  SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);

  int running = 1;

  while (running) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_QUIT:
      case SDL_KEYDOWN:
        running = 0;
        break;
      }
    }

    if (cardFound && prev_value != shared_value) {
      prev_value = shared_value;

      if (textTexture)
        SDL_DestroyTexture(textTexture);
      // char msg[64];
      // sprintf(resultText, resultTextFormat, shared_value);
      snprintf(resultText, sizeof(resultText), "Value: %x", shared_value);

      textTexture = renderText(renderer, font, resultText, color, &dest);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &dest);
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
  SDL_DestroyTexture(texture);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  //---------------------------------------------------------------------------
  return 0;
}
