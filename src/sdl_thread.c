#include "pn532.h"
#include "pn532_rpi.h"
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
      newCardFound = 1;
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
    if (newCardFound && prev_value != shared_value) {
      printf("%x\n", shared_value);
      prev_value = shared_value;
      cardFound = 0;
    }
    pthread_mutex_unlock(&lock); // unlock after accessing
    usleep(10 * 1000);
  }

  return NULL;
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
  if (pthread_create(&poll_card_reader_thread, NULL, poll_card_reader, NULL) != 0) {
    printf("Thread creation failed\n");
    return 1;
  }

  if (pthread_create(&print_result_thread, NULL, print_result, NULL) != 0) {
    printf("Thread creation failed\n");
    return 1;
  }

  char c = 0;

  printf("Press 'Q' to quit\n");

  while (c != 'q') {
    c = getchar();             // waits for Enter    
  }

  pthread_mutex_lock(&lock); // lock before accessing
  searchingForCard = 0;
  pthread_mutex_unlock(&lock); // unlock after accessing

  // Wait for the thread to finish
  pthread_join(poll_card_reader_thread, NULL);
  pthread_join(print_result_thread, NULL);

  pthread_mutex_destroy(&lock);

  printf("Final Card ID: %x\n", shared_value);
  return 0;
}
