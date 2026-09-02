
#ifndef PICOLIB_H
#define PICOLIB_H

#include <raylib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "picolib_conf-core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Простая структура для возврата координат (аналог tuple в Lua)
typedef struct {
    int16_t x;
    int16_t y;
} picolib_vec2;

// Простая структура для возврата значений мышки
#if PICOLIB_USE_MOUSE == 1

typedef struct {
    int16_t x;              // координаты указателя мыши.
    int16_t y;              // координаты указателя мыши.
    bool left;              // нажата ли левая кнопка мыши (true/false).
    bool middle;            // нажата ли средняя кнопка мыши (true/false).
    bool right;             // нажата ли правая кнопка мыши (true/false).
    int8_t scrollx;         // scrollx — изменение прокрутки по горизонтали за последний кадр (значение в диапазоне от –31 до 32).
    int8_t scrolly;         // изменение прокрутки по вертикали за последний кадр (значение в диапазоне от –31 до 32).
} picolib_mouse;

picolib_mouse mouse(void);
picolib_mouse mousep(void);
#endif

// --- Структура для col_rect() --- ///
typedef struct {
    int16_t x, y;   // левый верхний угол
    int16_t w, h;   // ширина и высота
} Rect;

extern RenderTexture2D target;
extern bool show_fps;
extern int16_t cam_x;
extern int16_t cam_y;

extern Texture2D sprite_sheet;
extern bool spritesheet_loaded;

#if PICOLIB_USE_AUDIO == 1
extern Sound sounds[PICOLIB_MAX_SOUNDS];
extern bool sounds_loaded[PICOLIB_MAX_SOUNDS];
#endif

// --- Инициализация ---
void picolib_load_spritesheet(const char* filepath); // Загрузка спрайт-листа
void picolib_load_font(const char* filepath);
// Загружает все звуки из папки "sounds/" по именам файлов "0.wav", "1.wav" и т.д.
void picolib_load_sounds();

// Загружает спрайт-лист из данных в памяти
void picolib_load_spritesheet_from_memory(const unsigned char* data, int size);
// Загружает шрифт из данных в памяти
void picolib_load_font_from_memory(const unsigned char* data, int size);
// Загружает карту из данных в памяти
void picolib_load_map_from_memory(const char* data);

// --- Основные функции ---
void update(void);
void draw(void);

// --- API для рисования ---
void cls(uint8_t color);
void print(const char* format, int16_t x, int16_t y, uint8_t color, ...);
void circ(int16_t x, int16_t y, int16_t r, uint8_t color);
void circfill(int16_t x, int16_t y, int16_t r, uint8_t color);
void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
void rectfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);

// --- Спрайты ---
// Простая версия: рисует один спрайт 8x8
void spr(int16_t n, int16_t x, int16_t y);

// Полная версия: w и h теперь указывают КОЛИЧЕСТВО блоков 8x8 (а не пиксели!)
void spr_pro(int16_t n, int16_t x, int16_t y, uint8_t w, uint8_t h, bool flip_x, bool flip_y);

void spr_scale(int16_t n, int16_t x, int16_t y, uint8_t zoom);


// --- API для ввода ---
bool btn(uint8_t id);
bool btnp(uint8_t id);


// --- API для камеры ---
// Устанавливает смещение камеры и возвращает предыдущее значение.
// Чтобы сбросить камеру, вызовите camera(0, 0).
picolib_vec2 camera(int16_t x, int16_t y);


// --- API для звука ---
// Проигрывает звук по индексу
void sfx(int index);


// --- API для карты .csv
#if PICOLIB_USE_MAP == 1
extern uint8_t map[MAP_ROWS][MAP_COLS];    // карта (только ID тайлов)

void map_draw(int celx, int cely, int sx, int sy, int celw, int celh);
uint8_t mget(int x, int y);
void mset(int x, int y, uint8_t id);
#endif

// ============================================================
//                    СОХРАНЕНИЕ / ЗАГРУЗКА ДАННЫХ (API)
// ============================================================
#if PICOLIB_USE_SAVE == 1
// --- Хранилище ---
extern char* save_file_path;

void save(uint8_t pos, uint64_t value);
uint64_t load(uint8_t pos);
bool is_save(void);
bool storage_save_all(void); 

// --- Текстовые файлы ---

// Сохраняет текстовую строку в файл. Возвращает true при успехе, иначе false.
bool save_text(const char *fileName, const char *text);

// Загружает текстовый файл в память. Возвращает строку (выделенную память).
// Возвращает NULL, если файл не найден или ошибка.
char* load_text(const char* fileName);

// --- Бинарные файлы ---

// Сохраняет бинарные данные в файл. Возвращает true при успехе, иначе false.
bool save_data(const char *fileName, void *data, int bytesToWrite);

// Загружает бинарный файл в память. Размер файла записывается в *bytesRead.
// Возвращает указатель на массив байтов (выделенную память) или NULL при ошибке.
unsigned char* load_data(const char* fileName, int* bytesRead);

// --- Освобождение памяти ---

// Освобождает память, выделенную load_text().
void unload_text(char* text);

// Освобождает память, выделенную load_data().
void unload_data(unsigned char* data);
#endif


// --- API для столкновение ---
bool col_rect(Rect* a, Rect* b);

#ifdef __cplusplus
}
#endif

#endif
