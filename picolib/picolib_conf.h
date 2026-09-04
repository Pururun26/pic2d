#ifndef PICOLIB_CONF_H
#define PICOLIB_CONF_H

// Настройки по умолчанию (пользователь может отредактировать)
// Размер окна (если не определены, используется автоматический расчёт)
#define PICOLIB_WINDOW_WIDTH  512
#define PICOLIB_WINDOW_HEIGHT 512
#define PICOLIB_WIDTH   128
#define PICOLIB_HEIGHT  128
#define PICOLIB_TITLE   "PIC2D"
#define PICOLIB_FPS     30

// Палитра
#define PICOLIB_COLOR_COUNT 16

// Путь и имя файла спрайт-листа (должен быть 128x128 пикселей для 256 спрайтов 8x8)
#define PICOLIB_SS      "picolib_spritesheet.png"
#define PICOLIB_SPRITE_SHEET_TILES 16   // количество тайлов по каждой стороне (16×16 = 256)
#define PICOLIB_SPRITE_SIZE  8    // размер тайла (8x8)


// Шрифт
#define PICOLIB_FONT    "font_pico8.png"

// Звуки
#define PICOLIB_MAX_SOUNDS      64                 // Максимум звуков, которые можно загрузить
#define PICOLIB_USE_AUDIO       0                  // 1 включить звук для игры, 0 отключить звук
#define PICOLIB_SOUNDS_PATH     "sounds/%d.wav"    // Путь к звуковым файлам

// Генерация звуков на лету (синтез)
#define PICOLIB_USE_TONE        1
#define PICOLIB_MAX_TONE_SOUNDS 4

// Мышка
#define PICOLIB_USE_MOUSE       1


// Сохранение и загрузка
#define PICOLIB_USE_SAVE        1
#define PICOLIB_SAVE_SLOTS      64


// Карта
#define PICOLIB_USE_MAP         1

// Размеры карты
#define MAP_ROWS     32
#define MAP_COLS     128

// Путь к файлу карты (если используется)
#define PICOLIB_MAP_FILE "map.csv"

#endif
