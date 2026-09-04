#include <raylib.h>
#include <string.h>
#include "pic2d-core.h"
#include "picolib.h"

void pic2d_load_spritesheet_from_memory(const unsigned char* data, int size) {
    Image img = LoadImageFromMemory(".png", data, size);
    if (img.data != NULL) {
        sprite_sheet = LoadTextureFromImage(img);
        SetTextureFilter(sprite_sheet, TEXTURE_FILTER_POINT);
        spritesheet_loaded = true;
        UnloadImage(img);
    } else {
        // заглушка
        Image fallback = GenImageColor(128, 128, MAGENTA);
        sprite_sheet = LoadTextureFromImage(fallback);
        UnloadImage(fallback);
        spritesheet_loaded = true;
    }
}

void pic2d_load_font_from_memory(const unsigned char* data, int size) {
    Image img = LoadImageFromMemory(".png", data, size);
    if (img.data != NULL) {
        pico_font = LoadFontFromImage(img, MAGENTA, 32);
        UnloadImage(img);
        font_loaded = true;
        font_size = 5.0f;        // как в файловой версии
        font_spacing = 1.0f;     // как в файловой версии
        TraceLog(LOG_INFO, "PICOLIB: Font loaded from memory.");
    } else {
        // Запасной вариант — стандартный шрифт Raylib
        pico_font = GetFontDefault();
        font_size = 10.0f;
        font_spacing = 1.0f;
        font_loaded = false;
        TraceLog(LOG_WARNING, "PICOLIB: Failed to load font from memory. Using default.");
    }
}

// Загружает карту из данных в памяти (из архива)
void pic2d_load_map_from_memory(const char* data) {
    int row = 0;
    const char* p = data;
    while (*p && row < MAP_ROWS) {
        // Ищем конец строки
        const char* end = p;
        while (*end && *end != '\n' && *end != '\r') end++;

        // Копируем строку в буфер для парсинга
        char line[2048];
        int len = end - p;
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        strncpy(line, p, len);
        line[len] = '\0';

        // Парсим строку CSV
        parse_csv_line(line, map[row], MAP_COLS);
        row++;

        // Переходим к следующей строке
        p = end;
        while (*p == '\n' || *p == '\r') p++;
    }
    map_loaded_flag = 1;   // устанавливаем флаг, что карта загружена
}