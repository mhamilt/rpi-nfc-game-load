#include <stdint.h>

typedef struct {
  uint8_t card_id[4];
  const char *console;
  const char *title;
  const char *filename;
  const char *cover;
} Game;

Game gamelist[] = {
    {.card_id = {0X4B, 0xEB, 0x08, 0x25}, .console = "nes",       .title = "Teenage Mutant Ninja Turtles", .filename = "turtles.nes",          .cover = "turtles.bmp"},
    {.card_id = {0x80, 0xc4, 0x93, 0x97}, .console = "nes",       .title = "The Great Gatsby",             .filename = "gatsby.nes",           .cover = "gatsby.bmp"},
    {.card_id = NULL,                     .console = "nes",       .title = "Mario Bros.",                  .filename = "mariobros.nes",        .cover = "mariobros.bmp"},
    {.card_id = NULL,                     .console = "megadrive", .title = "Sonic 2.bin" ,                 .filename = "sonic-2.bin",          .cover = "sonic-2.bmp" },
    {.card_id = NULL,                     .console = "megadrive", .title = "Sonic 2 & Knuckles.bin" ,      .filename = "sonic-2-knuckles.bin", .cover = "sonic-2-knuckles.bmp" },
    {.card_id = NULL,                     .console = "megadrive", .title = "Sonic 3 & Knuckles.bin" ,      .filename = "sonic-3-knuckles.bin", .cover = "sonic-3-knuckles.bmp" },
    {.card_id = NULL,                     .console = "megadrive", .title = "Sonic.bin" ,                   .filename = "sonic.bin",            .cover = "sonic.bmp" }
};

