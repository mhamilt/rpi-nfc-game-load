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
//-----------------------------------------------------------------------------
typedef enum {
  FADE_IN,
  FADE_OUT,
  NO_FADE,
  FADE_OUT_END,
  FADE_IN_END,
} FADE_STATE;

FADE_STATE fade_state = NO_FADE;

volatile uint32_t shared_value = 0;
volatile uint32_t prev_value = 1;
volatile uint8_t searchingForCard = 1;
volatile uint8_t cardFound = 0;
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
//-----------------------------------------------------------------------------
void *print_result(void *arg) {

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

//-----------------------------------------------------------------------------

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

  SDL_Rect destRect;
  destRect.x = 100;
  destRect.y = 100;

  SDL_Texture *textTextures[2];
  uint8_t textureIndex = 0;

  SDL_Surface *textSurf = TTF_RenderText_Blended(font, "Hello", color);
  textTextures[0] = SDL_CreateTextureFromSurface(renderer, textSurf);
  SDL_FreeSurface(textSurf);
  
  textSurf = TTF_RenderText_Blended(font, "World", color);
  textTextures[1] = SDL_CreateTextureFromSurface(renderer, textSurf);
  SDL_FreeSurface(textSurf);

  SDL_Texture *currentTexture = textTextures[textureIndex];
  SDL_QueryTexture(currentTexture, NULL, NULL, &destRect.w, &destRect.h);

  SDL_Event e;
  int running = 1;

  char displayText[10];

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

    switch (fade_state) {

    case FADE_IN:
      alpha += alphaStep;
      if (alpha >= 255) {
        alpha = 255;
        fade_state = FADE_IN_END;
      }
      SDL_SetTextureAlphaMod(currentTexture, alpha);
      break;
    case FADE_OUT:
      alpha -= alphaStep;
      if (alpha <= 0) {
        alpha = 0;
        fade_state = FADE_OUT_END;
      }
      SDL_SetTextureAlphaMod(currentTexture, alpha);
      break;
    }

    if (value_updated) {

      switch (fade_state) {
      case FADE_IN_END:
        if (swapTexture) {
          //   uint8_t swapTextureIndex = (textureIndex == 1) ? 0 : 1;
          //   sprintf(displayText, "%X", print_value);

          //   SDL_Surface *textSurf =
          //       TTF_RenderText_Blended(font, displayText, color);

          //   if (textTextures[swapTextureIndex])
          //     SDL_DestroyTexture(textTextures[swapTextureIndex]);

          //   textTextures[swapTextureIndex] =
          //       SDL_CreateTextureFromSurface(renderer, textSurf);
          //   SDL_FreeSurface(textSurf);

          swapTexture = 0;
          fade_state = FADE_OUT;
        }
        break;
      case FADE_OUT_END:
        textureIndex = (textureIndex == 1) ? 0 : 1;
        currentTexture = textTextures[textureIndex];
        SDL_SetTextureAlphaMod(currentTexture, 0);
        SDL_QueryTexture(currentTexture, NULL, NULL, &destRect.w, &destRect.h);
        value_updated = 0;
        fade_state = FADE_IN;
        break;
      }
    }

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, currentTexture, NULL, &destRect);
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
  SDL_DestroyTexture(textTextures[0]);
  SDL_DestroyTexture(textTextures[1]);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  //---------------------------------------------------------------------------
  return 0;
}
//-----------------------------------------------------------------------------