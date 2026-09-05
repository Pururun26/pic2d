// ============================================================
//  ПРАВИЛО №1 (CAMERA)
// ============================================================
//  Любая функция, которая рисует что-либо на экране (спрайты,
//  примитивы, текст), ОБЯЗАНА вычитать cam_x и cam_y из
//  переданных координат, чтобы преобразовать мировые координаты
//  в экранные.
// ============================================================
//  ПРАВИЛО №2 (СОХРАНЕНИЕ)
// ============================================================
//  1. Все игровые данные хранятся в массиве storage_data[64] (оперативная память).
//     Каждый слот — это uint64_t (число до 18 квинтиллионов).
//
//  2. save(pos, value) записывает значение в storage_data[pos],
//     НО НЕ СОХРАНЯЕТ ЕГО НА ДИСК (файл storage.bin).
//     Это сделано специально, чтобы избежать частых операций записи.
//
//  3. Реальная запись в файл storage.bin происходит ТОЛЬКО при вызове storage_save_all().
//     Эта функция перезаписывает весь файл (512 байт) текущим состоянием storage_data.
//
//  4. storage_save_all() вызывается автоматически в двух случаях:
//     - при штатном закрытии окна (в конце main.c) – всегда,
//     - каждые 10 минут (автосохранение в главном цикле) – если игра долго работает.
//
//  5. load(pos) возвращает значение из storage_data[pos] (если данные уже загружены)
//     или из файла (при первом обращении, если storage.bin существует).
//     При отсутствии файла load() возвращает 0.
//
//  6. Если вам нужно мгновенное сохранение на диск (например, в экстренных случаях),
//     вы можете раскомментировать вызов storage_save_all() внутри save(),
//     но помните, что это замедлит игру при частых сохранениях.
//
//  ВАЖНО: save() и load() работают с памятью, а запись на диск – отдельная операция.
//         Не забывайте, что прогресс сохраняется только при закрытии окна или автосохранении.
// ============================================================

#include "picolib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <raylib.h>

RenderTexture2D target;

// Переменные для спрайт-листа (теперь они статические и скрыты внутри файла)
Texture2D sprite_sheet;
bool spritesheet_loaded = false;

// Массив звуков
#if PICOLIB_USE_AUDIO == 1
Sound sounds[PICOLIB_MAX_SOUNDS];
bool sounds_loaded[PICOLIB_MAX_SOUNDS];
#endif

// Камера
int16_t cam_x = 0;
int16_t cam_y = 0;

bool show_fps = false;

Color palette[PICOLIB_COLOR_COUNT] = {
    {0x00, 0x00, 0x00, 0xFF}, // 0: black
    {0x1D, 0x2B, 0x53, 0xFF}, // 1: dark-blue
    {0x7E, 0x25, 0x53, 0xFF}, // 2: dark-purple
    {0x00, 0x87, 0x51, 0xFF}, // 3: dark-green
    {0xAB, 0x52, 0x36, 0xFF}, // 4: brown
    {0x5F, 0x57, 0x4F, 0xFF}, // 5: dark-grey
    {0xC2, 0xC3, 0xC7, 0xFF}, // 6: light-grey
    {0xFF, 0xF1, 0xE8, 0xFF}, // 7: white
    {0xFF, 0x00, 0x4D, 0xFF}, // 8: red
    {0xFF, 0xA3, 0x00, 0xFF}, // 9: orange
    {0xFF, 0xEC, 0x27, 0xFF}, // 10: yellow
    {0x00, 0xE4, 0x36, 0xFF}, // 11: green
    {0x29, 0xAD, 0xFF, 0xFF}, // 12: blue
    {0x83, 0x76, 0x9C, 0xFF}, // 13: lavender
    {0xFF, 0x77, 0xA8, 0xFF}, // 14: pink
    {0xFF, 0xCC, 0xAA, 0xFF}  // 15: light-peach
};


// --- 1. РЕАЛИЗАЦИЯ ЗАГРУЗКИ СПРАЙТ-ЛИСТА ---
void picolib_load_spritesheet(const char* filepath)
{
    sprite_sheet = LoadTexture(filepath);
    if (sprite_sheet.id != 0)
    {
        SetTextureFilter(sprite_sheet, TEXTURE_FILTER_POINT); // КРИТИЧНО для пиксель-арта!
        spritesheet_loaded = true;
    }
    else
    {
        // Если файл не найден, создаем безопасную заглушку (пурпурный квадрат 128x128), 
        // чтобы игра не упала с ошибкой сегментации.
        Image img = GenImageColor(128, 128, MAGENTA);
        sprite_sheet = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(sprite_sheet, TEXTURE_FILTER_POINT);
        TraceLog(LOG_WARNING, "PICOLIB: Spritesheet '%s' not found. Using placeholder.", filepath);
        spritesheet_loaded = true;
    }
}

// --- ПЕРЕМЕННЫЕ ДЛЯ ШРИФТА ---
Font pico_font;
bool font_loaded = false;
float font_size = 5.0f;    // Размер шрифта (5px идеально для сетки 128x128)
float font_spacing = 1.0f; // Расстояние между буквами

// --- 1. РЕАЛИЗАЦИЯ ЗАГРУЗКИ ШРИФТА ---
void picolib_load_font(const char* filepath) {
    Image img = LoadImage(filepath);
    
    if (img.data != NULL) {
        // Загружаем шрифт из изображения. 
        // MAGENTA используется как цвет прозрачности (если фон не альфа-канал).
        // 32 - это ASCII код пробела (первый символ в сетке должен быть пробелом).
        pico_font = LoadFontFromImage(img, MAGENTA, 32);
        UnloadImage(img);
        font_loaded = true;
        TraceLog(LOG_INFO, "PICOLIB: Font '%s' loaded successfully.", filepath);
    } else {
        // Если файл не найден, используем стандартный шрифт Raylib в качестве запасного варианта
        pico_font = GetFontDefault();
        font_size = 10.0f; // Стандартный шрифт лучше смотрится в размере 10
        font_loaded = false;
        TraceLog(LOG_WARNING, "PICOLIB: Font file '%s' not found. Using default font.", filepath);
    }
}


// --- РЕРЕАЛИЗАЦИЯ ЗАГРУЗКИ ЗВУКОВ ---
#if PICOLIB_USE_AUDIO == 1
void picolib_load_sounds() {
    char path[64];
    for (int i = 0; i < PICOLIB_MAX_SOUNDS; i++) {
        snprintf(path, sizeof(path), PICOLIB_SOUNDS_PATH, i);
        if (FileExists(path)) {  // проверяем, есть ли файл
            sounds[i] = LoadSound(path);
            sounds_loaded[i] = (sounds[i].frameCount > 0);
        } else {
            sounds_loaded[i] = false;
        }
    }
}
#endif


// --- 2. КАМЕРА ---
picolib_vec2 camera(int16_t x, int16_t y)
{
    picolib_vec2 prev = {cam_x, cam_y};
    cam_x = x;
    cam_y = y;
    return prev;
}


// --- 3. РИСОВАНИЕ ---
void cls(uint8_t color)
{
    if (color < PICOLIB_COLOR_COUNT )
    {
        ClearBackground(palette[color]);
    }
    else
    {
        ClearBackground(palette[0]);
    }
}

void print(const char* format, int16_t x, int16_t y, uint8_t color, ...) {
    va_list args;
    va_start(args, color); // последний именованный параметр перед ...
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Color c = (color < PICOLIB_COLOR_COUNT) ? palette[color] : palette[0];
    // Позиция с учетом камеры
    Vector2 pos = { (float)(x - cam_x), (float)(y - cam_y) };
    // Используем DrawTextEx для отрисовки кастомного шрифта
    DrawTextEx(pico_font, buffer, pos, font_size, font_spacing, c);
}


// --- ПРИМИТИВЫ РИСОВАНИЯ ---
void circ(int16_t x, int16_t y, int16_t r, uint8_t color) {
    DrawCircleLines(x - cam_x, y - cam_y, (float)r, palette[color]);
}

void circfill(int16_t x, int16_t y, int16_t r, uint8_t color) {
    DrawCircle(x - cam_x, y - cam_y, (float)r, palette[color]);
}

void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    // МАГИЯ: x1 и y1 инклюзивны, поэтому добавляем 1 к ширине и высоте
    int16_t w = (x1 - x0) + 1;
    int16_t h = (y1 - y0) + 1;
    
    // Если ширина или высота получились <= 0, ничего не рисуем (защита от ошибок)
    if (w <= 0 || h <= 0) return;

    DrawRectangleLines(x0 - cam_x, y0 - cam_y, w, h, palette[color]);
}

void rectfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    int16_t w = (x1 - x0) + 1;
    int16_t h = (y1 - y0) + 1;
    
    if (w <= 0 || h <= 0) return;

    DrawRectangle(x0 - cam_x, y0 - cam_y, w, h, palette[color]);
}

// Рисует линию от (x0, y0) до (x1, y1) цветом color (0-15)
void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    Color c = (color < PICOLIB_COLOR_COUNT) ? palette[color] : palette[0];
    DrawLine(x0 - cam_x, y0 - cam_y, x1 - cam_x, y1 - cam_y, c);
}

// Рисует пустой овал (эллипс) внутри прямоугольника (x0,y0)-(x1,y1)
void oval(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    Color c = (color < PICOLIB_COLOR_COUNT) ? palette[color] : palette[0];
    
    // Нормализуем координаты (чтобы x0 <= x1, y0 <= y1)
    int16_t left = (x0 < x1) ? x0 : x1;
    int16_t right = (x0 < x1) ? x1 : x0;
    int16_t top = (y0 < y1) ? y0 : y1;
    int16_t bottom = (y0 < y1) ? y1 : y0;
    
    int16_t cx = (left + right) / 2;
    int16_t cy = (top + bottom) / 2;
    float rx = (right - left) / 2.0f;
    float ry = (bottom - top) / 2.0f;
    
    DrawEllipseLines(cx - cam_x, cy - cam_y, rx, ry, c);
}

// Рисует залитый овал (эллипс) внутри прямоугольника (x0,y0)-(x1,y1)
void ovalfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    Color c = (color < PICOLIB_COLOR_COUNT) ? palette[color] : palette[0];
    
    // Нормализуем координаты (чтобы x0 <= x1, y0 <= y1)
    int16_t left = (x0 < x1) ? x0 : x1;
    int16_t right = (x0 < x1) ? x1 : x0;
    int16_t top = (y0 < y1) ? y0 : y1;
    int16_t bottom = (y0 < y1) ? y1 : y0;
    
    int16_t cx = (left + right) / 2;
    int16_t cy = (top + bottom) / 2;
    float rx = (right - left) / 2.0f;
    float ry = (bottom - top) / 2.0f;
    
    DrawEllipse(cx - cam_x, cy - cam_y, rx, ry, c);
}

// Рисует пиксель в (x, y) цветом color (0-15) с учётом камеры
void pset(int16_t x, int16_t y, uint8_t color) {
    Color c = (color < PICOLIB_COLOR_COUNT) ? palette[color] : palette[0];
    DrawPixel(x - cam_x, y - cam_y, c);
}


// --- 4. СПРАЙТЫ ---
void spr_pro(int16_t n, int16_t x, int16_t y, uint8_t w, uint8_t h, bool flip_x, bool flip_y) {
    if (!spritesheet_loaded) return;
    if (w == 0 || h == 0) return;

    int16_t col = n % PICOLIB_SPRITE_SHEET_TILES; 
    int16_t row = n / PICOLIB_SPRITE_SHEET_TILES;
    int16_t pixel_w = w * PICOLIB_SPRITE_SIZE;
    int16_t pixel_h = h * PICOLIB_SPRITE_SIZE;

    Rectangle src = {
        (float)(col * PICOLIB_SPRITE_SIZE),
        (float)(row * PICOLIB_SPRITE_SIZE),
        (float)pixel_w,
        (float)pixel_h
    };

    // ОТРАЖЕНИЕ ЧЕРЕЗ ОТРИЦАТЕЛЬНЫЕ РАЗМЕРЫ src
    if (flip_x) {
        src.width = -pixel_w;   // отражаем по X, x остаётся без изменений
    }
    if (flip_y) {
        src.height = -pixel_h;  // отражаем по Y, y остаётся без изменений
    }

    Rectangle dest = { 
        (float)(x - cam_x), 
        (float)(y - cam_y), 
        (float)pixel_w, 
        (float)pixel_h 
    };

    DrawTexturePro(sprite_sheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
}

// Простая версия (удобная обёртка)
void spr(int16_t n, int16_t x, int16_t y) {
    // Теперь передаем целые числа: 1 блок шириной, 1 блок высотой
    spr_pro(n, x, y, 1, 1, false, false);
}

// Рисует спрайт в маштабировании
void spr_scale(int16_t n, int16_t x, int16_t y, uint8_t zoom) {
    if (!spritesheet_loaded) return;
    if (zoom == 0) return;

    int16_t col = n % PICOLIB_SPRITE_SHEET_TILES;
    int16_t row = n / PICOLIB_SPRITE_SHEET_TILES;

    // Исходный тайл (размер определяется макросом)
    Rectangle src = {
        (float)(col * PICOLIB_SPRITE_SIZE),
        (float)(row * PICOLIB_SPRITE_SIZE),
        (float)PICOLIB_SPRITE_SIZE,
        (float)PICOLIB_SPRITE_SIZE
    };

    // Целевой прямоугольник с учётом камеры и масштаба
    Rectangle dest = {
        (float)(x - cam_x),
        (float)(y - cam_y),
        (float)PICOLIB_SPRITE_SIZE * zoom,
        (float)PICOLIB_SPRITE_SIZE * zoom
    };

    DrawTexturePro(sprite_sheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
}


// --- 5. API для ввода ---
bool btn(uint8_t id)
{
    switch (id)
    {
        case 0: return IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
        case 1: return IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        case 2: return IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
        case 3: return IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
        case 4: return IsKeyDown(KEY_J) || IsKeyDown(KEY_Z);
        case 5: return IsKeyDown(KEY_K) || IsKeyDown(KEY_X);
        default: return false;
    }
}

bool btnp(uint8_t id)
{
    switch (id)
    {
        case 0: return IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
        case 1: return IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
        case 2: return IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
        case 3: return IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
        case 4: return IsKeyPressed(KEY_J) || IsKeyPressed(KEY_Z);
        case 5: return IsKeyPressed(KEY_K) || IsKeyPressed(KEY_X);
        default: return false;
    }
}


// --- API для звука ---
void sfx(int index) {
#if PICOLIB_USE_AUDIO == 1
    if (index >= 0 && index < PICOLIB_MAX_SOUNDS && sounds_loaded[index]) {
        PlaySound(sounds[index]);
    }
#endif
}

#if PICOLIB_USE_TONE == 1
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Генераторы волн (можно вынести в отдельный файл, но пока внутри)
// Генератор синусоиды
static void generate_sine(short* samples, int count, float freq, int sample_rate, int volume) {
    for (int i = 0; i < count; i++) {
        float t = (float)i / sample_rate;
        float value = sinf(2.0f * M_PI * freq * t);
        samples[i] = (short)(value * volume * 327.67f);
    }
}

static void generate_square(short* samples, int count, float freq, int sample_rate, int volume, float duty) {
    for (int i = 0; i < count; i++) {
        float t = (float)i / sample_rate;
        float phase = fmodf(t * freq, 1.0f);
        float value = (phase < duty) ? 1.0f : -1.0f;
        samples[i] = (short)(value * volume * 327.67f);
    }
}

// Генератор треугольной волны
static void generate_triangle(short* samples, int count, float freq, int sample_rate, int volume) {
    for (int i = 0; i < count; i++) {
        float t = (float)i / sample_rate;
        float phase = fmodf(t * freq, 1.0f);
        float value = 2.0f * fabsf(2.0f * phase - 1.0f) - 1.0f;
        samples[i] = (short)(value * volume * 327.67f);
    }
}

// Генератор пилообразной волны
static void generate_sawtooth(short* samples, int count, float freq, int sample_rate, int volume) {
    for (int i = 0; i < count; i++) {
        float t = (float)i / sample_rate;
        float phase = fmodf(t * freq, 1.0f);
        float value = 2.0f * phase - 1.0f;
        samples[i] = (short)(value * volume * 327.67f);
    }
}

// Генератор шума (принимает те же параметры для единообразия)
static void generate_noise(short* samples, int count, float freq, int sample_rate, int volume) {
    (void)freq;         // параметр не используется
    (void)sample_rate;  // параметр не используется
    for (int i = 0; i < count; i++) {
        float value = (float)rand() / RAND_MAX * 2.0f - 1.0f;
        samples[i] = (short)(value * volume * 327.67f);
    }
}

// Массивы для хранения синтезированных звуков
static Sound tone_sounds[PICOLIB_MAX_TONE_SOUNDS];
static bool tone_loaded[PICOLIB_MAX_TONE_SOUNDS] = {false};
static int next_tone_slot = 0; // индекс следующего слота для перезаписи

void tone(uint32_t frequency, uint32_t duration, uint32_t volume, uint32_t flags) {
    // Если громкость нулевая или длительность нулевая — ничего не делаем
    if (volume == 0 || duration == 0) return;

    // Извлекаем канал (0-3)
    int channel = flags & 3;
    if (channel > 3) return; // только 4 канала

    // Параметры звука
    int sample_rate = 44100;
    float freq = (float)frequency;
    float duration_sec = duration / 60.0f;   // кадры -> секунды
    int sample_count = (int)(sample_rate * duration_sec);
    if (sample_count <= 0) return;

    // Выделяем память под сэмплы
    short* samples = malloc(sample_count * sizeof(short));
    if (!samples) return;

    // Генерируем волну в зависимости от канала
    switch (channel) {
        case TONE_PULSE1:
            generate_square(samples, sample_count, freq, sample_rate, volume, 0.125f); // 12.5%
            break;
        case TONE_PULSE2:
            generate_square(samples, sample_count, freq, sample_rate, volume, 0.25f);  // 25%
            break;
        case TONE_TRIANGLE:
            generate_triangle(samples, sample_count, freq, sample_rate, volume);
            break;
        case TONE_NOISE:
            generate_noise(samples, sample_count, freq, sample_rate, volume);
            break;
        default:
            free(samples);
            return;
    }

    // Создаём Wave
    Wave wave = {
        .frameCount = sample_count,
        .sampleRate = sample_rate,
        .sampleSize = 16,
        .channels = 1,
        .data = samples
    };

    // Загружаем Sound
    Sound sound = LoadSoundFromWave(wave);
    if (sound.frameCount == 0) {
        free(samples);
        return;
    }

    // Ищем свободный слот
    int slot = -1;
    for (int i = 0; i < PICOLIB_MAX_TONE_SOUNDS; i++) {
        if (!tone_loaded[i]) {
            slot = i;
            break;
        }
    }
    // Если все заняты — перезаписываем следующий по очереди
    if (slot == -1) {
        slot = next_tone_slot;
        if (tone_loaded[slot]) {
            UnloadSound(tone_sounds[slot]);
            tone_loaded[slot] = false;
        }
        next_tone_slot = (next_tone_slot + 1) % PICOLIB_MAX_TONE_SOUNDS;
    }

    // Сохраняем звук и проигрываем
    tone_sounds[slot] = sound;
    tone_loaded[slot] = true;
    PlaySound(sound);

    // Освобождаем сэмплы (они уже скопированы в Sound)
    free(samples);
}
#endif


// --- API для мышки ---
#if PICOLIB_USE_MOUSE == 1

picolib_mouse mouse(void) {
    picolib_mouse result = {0};
    
    // Получаем экранные координаты мыши
    Vector2 pos = GetMousePosition();
    
    // Вычисляем масштаб и смещение (как в главном цикле)
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float scaleX = (float)screenW / PICOLIB_WIDTH;
    float scaleY = (float)screenH / PICOLIB_HEIGHT;
    float current_scale = (scaleX < scaleY) ? scaleX : scaleY;
    int offsetX = (int)((screenW - PICOLIB_WIDTH * current_scale) / 2);
    int offsetY = (int)((screenH - PICOLIB_HEIGHT * current_scale) / 2);
    
    // Переводим экранные координаты в логические (внутри рендер-текстуры)
    int logical_x = (int)((pos.x - offsetX) / current_scale);
    int logical_y = (int)((pos.y - offsetY) / current_scale);
    
    // Преобразуем логические в мировые с учётом камеры
    result.x = logical_x + cam_x;
    result.y = logical_y + cam_y;
    
    // Состояния кнопок
    result.left   = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    result.middle = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    result.right  = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    
    // Прокрутка (колёсико)
    float wheel_x = GetMouseWheelMoveV().x;  // горизонтальная (если есть)
    float wheel_y = GetMouseWheelMove();      // вертикальная
    // Приводим к int8_t с ограничением, чтобы не вылезти за пределы
    result.scrollx = (int8_t)(wheel_x > 32 ? 32 : (wheel_x < -32 ? -32 : (int8_t)wheel_x));
    result.scrolly = (int8_t)(wheel_y > 32 ? 32 : (wheel_y < -32 ? -32 : (int8_t)wheel_y));
    
    return result;
}

picolib_mouse mousep(void) {
    picolib_mouse result = {0};
    
    // Получаем экранные координаты мыши
    Vector2 pos = GetMousePosition();
    
    // Вычисляем масштаб и смещение (как в главном цикле)
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float scaleX = (float)screenW / PICOLIB_WIDTH;
    float scaleY = (float)screenH / PICOLIB_HEIGHT;
    float current_scale = (scaleX < scaleY) ? scaleX : scaleY;
    int offsetX = (int)((screenW - PICOLIB_WIDTH * current_scale) / 2);
    int offsetY = (int)((screenH - PICOLIB_HEIGHT * current_scale) / 2);
    
    // Переводим экранные координаты в логические (внутри рендер-текстуры)
    int logical_x = (int)((pos.x - offsetX) / current_scale);
    int logical_y = (int)((pos.y - offsetY) / current_scale);
    
    // Преобразуем логические в мировые с учётом камеры
    result.x = logical_x + cam_x;
    result.y = logical_y + cam_y;
    
    // Состояния кнопок
    result.left   = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    result.middle = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
    result.right  = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    
    // Прокрутка (колёсико)
    float wheel_x = GetMouseWheelMoveV().x;  // горизонтальная (если есть)
    float wheel_y = GetMouseWheelMove();      // вертикальная
    // Приводим к int8_t с ограничением, чтобы не вылезти за пределы
    result.scrollx = (int8_t)(wheel_x > 32 ? 32 : (wheel_x < -32 ? -32 : (int8_t)wheel_x));
    result.scrolly = (int8_t)(wheel_y > 32 ? 32 : (wheel_y < -32 ? -32 : (int8_t)wheel_y));
    
    return result;
}

#endif


// --- API для сохранение и загрузки ---
#if PICOLIB_USE_SAVE == 1
char* save_file_path = "storage.bin";

bool save_text(const char *fileName, const char *text) { return SaveFileText(fileName, text); }
char* load_text(const char* fileName) { return LoadFileText(fileName); }

bool save_data(const char *fileName, void *data, int bytesToWrite) { return SaveFileData(fileName, data, bytesToWrite); }
unsigned char* load_data(const char* fileName, int* bytesRead) { return LoadFileData(fileName, bytesRead); }

void unload_text(char* text) { UnloadFileText(text); }
void unload_data(unsigned char* data) { UnloadFileData(data); }

// Массив слотов
static uint64_t storage_data[PICOLIB_SAVE_SLOTS];
static bool storage_loaded = false;

// Внутренняя функция для сохранения всего массива
bool storage_save_all(void) {
    TraceLog(LOG_INFO, "DEBUG: storage_save_all() called"); // добавить
    return save_data(save_file_path, storage_data, sizeof(storage_data));
}

// Внутренняя функция для загрузки всего массива
static bool storage_load_all(void) {
    int size;
    unsigned char* data = load_data(save_file_path, &size);
    if (data && size == sizeof(storage_data)) {
        memcpy(storage_data, data, sizeof(storage_data));
        unload_data(data);
        storage_loaded = true;
        return true;
    }
    // Если файла нет или он повреждён — инициализируем нулями
    memset(storage_data, 0, sizeof(storage_data));
    storage_loaded = true;
    // Сразу сохраняем, чтобы создать файл
    storage_save_all();
    return false;
}



// Публичный API: получить значение по позиции
uint64_t load(uint8_t pos) {
    if (pos >= PICOLIB_SAVE_SLOTS) return 0;
    if (!storage_loaded) {
        storage_load_all();
    }
    return storage_data[pos];
}

// Публичный API: установить значение по позиции и сразу сохранить
// Возвращает true, если сохранение прошло успешно, иначе false.
void save(uint8_t pos, uint64_t value) {
    if (pos >= PICOLIB_SAVE_SLOTS) return;
    if (!storage_loaded) storage_load_all();
    storage_data[pos] = value;
    // return storage_save_all(); // возвращаем результат сохранения
}

bool is_save(void) {
    return FileExists(save_file_path);
}
#endif


// -- API для карты ---
#if PICOLIB_USE_MAP == 1
uint8_t map[MAP_ROWS][MAP_COLS] = {0};
int map_loaded_flag = 0;

// Парсит одну строку CSV, извлекая целые числа (0-255) и сохраняя их в массив out.
// Возвращает количество распарсенных чисел.
int parse_csv_line(const char* line, uint8_t* out, int max_count) {
    int count = 0;
    const char* p = line;
    
    while (*p && count < max_count) {
        // Пропускаем пробелы и табуляции
        while (*p == ' ' || *p == '\t') p++;
        
        // Если конец строки или спецсимволы перевода – выходим
        if (*p == '\0' || *p == '\n' || *p == '\r') break;
        
        // Парсим число (только десятичные цифры)
        int val = 0;
        while (*p >= '0' && *p <= '9') {
            val = val * 10 + (*p - '0');
            p++;
        }
        out[count++] = (uint8_t)val;
        
        // Пропускаем запятую и пробелы после неё
        while (*p == ',' || *p == ' ' || *p == '\t') p++;
    }
    
    return count;
}

// Вспомогательная функция загрузки карты из CSV
static void load_map_from_csv(const char* filename) {
    char* file_content = LoadFileText(filename);
    if (!file_content) {
        TraceLog(LOG_WARNING, "PICOLIB: Map file '%s' not found. Using empty map.", filename);
        return;
    }

    int row = 0;
    // Разбиваем файл на строки. Используем внешний strtok (один раз, без вложенности)
    char* line = strtok(file_content, "\r\n");
    while (line != NULL && row < MAP_ROWS) {
        // Парсим строку и заполняем map[row]
        parse_csv_line(line, map[row], MAP_COLS);
        row++;
        line = strtok(NULL, "\r\n");
    }

    UnloadFileText(file_content);
    TraceLog(LOG_INFO, "PICOLIB: Map loaded from '%s'", filename);
}

// Загружает карту из данных в памяти (из архива)
void picolib_load_map_from_memory(const char* data) {
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

// Основная функция рисования карты (без слоёв, рисует все тайлы)
void map_draw(int celx, int cely, int sx, int sy, int celw, int celh) {
    if (!map_loaded_flag) {
        load_map_from_csv(PICOLIB_MAP_FILE);
        map_loaded_flag = 1;
    }

    if (celw <= 0 || celh <= 0) return;

    for (int row = 0; row < celh; row++) {
        for (int col = 0; col < celw; col++) {
            int map_x = celx + col;
            int map_y = cely + row;
            if (map_x < 0 || map_x >= MAP_COLS || map_y < 0 || map_y >= MAP_ROWS) continue;

            uint8_t tile_id = map[map_y][map_x];
            if (tile_id == 0) continue;

            // Рисуем тайл без фильтрации по флагам
            int pixel_x = sx + col * 8;
            int pixel_y = sy + row * 8;
            spr_pro(tile_id, pixel_x, pixel_y, 1, 1, 0, 0);
        }
    }
}

uint8_t mget(int x, int y) {
    if (x < 0 || x >= MAP_COLS || y < 0 || y >= MAP_ROWS) return 0;
    return map[y][x];
}

void mset(int x, int y, uint8_t id) {
    if (x < 0 || x >= MAP_COLS || y < 0 || y >= MAP_ROWS) return;
    map[y][x] = id;
}
#endif


// --- API для столкновение ---
bool col_rect(Rect* a, Rect* b) {
    int16_t a_left = a->x;
    int16_t a_top = a->y;
    int16_t a_right = a->x+a->w-1;
    int16_t a_bottom = a->y+a->h-1;

    int16_t b_left = b->x;
    int16_t b_top = b->y;
    int16_t b_right = b->x+b->w-1;
    int16_t b_bottom = b->y+b->h-1;

    if (a_top > b_bottom) return false;
    if (b_top > a_bottom) return false;
    if (a_left > b_right) return false;
    if (b_left > a_right) return false;

    return true;
}



void picolib_init(void) {
    uint16_t initial_scale_x = PICOLIB_WINDOW_WIDTH / PICOLIB_WIDTH;
    uint16_t initial_scale_y = PICOLIB_WINDOW_HEIGHT / PICOLIB_HEIGHT;
    if (initial_scale_x < 1) initial_scale_x = 1;
    if (initial_scale_y < 1) initial_scale_y = 1;
    
    InitWindow(PICOLIB_WIDTH * initial_scale_x, PICOLIB_HEIGHT * initial_scale_y, PICOLIB_TITLE);

    // Скрываем системный курсор
    HideCursor();
    
    SetTargetFPS(PICOLIB_FPS);

    // Аудио и tone
    #if PICOLIB_USE_AUDIO == 1 || PICOLIB_USE_TONE == 1
        InitAudioDevice();
        #if PICOLIB_USE_AUDIO == 1
            picolib_load_sounds();
        #endif
    #endif

    // ВАЖНО: Загружаем спрайт-лист, шрифт здесь! 
    picolib_load_spritesheet(PICOLIB_SS);
    picolib_load_font(PICOLIB_FONT);

    target = LoadRenderTexture(PICOLIB_WIDTH, PICOLIB_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    srand(time(NULL));
}


void picolib_run(void (*init)(void), void (*update)(void), void (*draw)(void)) {
    // Вызов пользовательской инициализации
    if (init) init();

    #if PICOLIB_USE_SAVE == 1
        int autosave_counter = 0;
        const int AUTOSAVE_INTERVAL = 60 * 10 * 60; // 10 минут при 60 FPS = 36000 кадров
    #endif

    while (!WindowShouldClose()) {
        // Обработка клавиш (F11, Ctrl+P)
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen(); 

        // Переключение FPS по Ctrl+P
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_P)) {
            show_fps = !show_fps;
        }

        #if PICOLIB_USE_SAVE == 1
            // Автосохранение
            autosave_counter++;
            if (autosave_counter >= AUTOSAVE_INTERVAL) {
                storage_save_all();
                autosave_counter = 0;
                // можно вывести сообщение "Autosaved"
            }
        #endif

        // Пользовательское обновление
        if (update) update();

        // Рендеринг
        BeginTextureMode(target);
        if (draw) draw();
        if (show_fps) {
            int fps = GetFPS();
            print("FPS: %d", cam_x+PICOLIB_WIDTH-30, cam_y+2, 7, fps); // ваш шрифт + камера
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // Вычисляем масштаб с сохранением пропорций
        float scaleX = (float)screenW / PICOLIB_WIDTH;
        float scaleY = (float)screenH / PICOLIB_HEIGHT;
        float current_scale = (scaleX < scaleY) ? scaleX : scaleY; // Переименовали в current_scale

        // Вычисляем смещение для центрирования
        int offsetX = (int)((screenW - PICOLIB_WIDTH * current_scale) / 2);
        int offsetY = (int)((screenH - PICOLIB_HEIGHT * current_scale) / 2);

        // Отрисовываем текстуру с центрированием
        DrawTexturePro(
            target.texture,
            (Rectangle){ 0, 0, (float)PICOLIB_WIDTH, (float)-PICOLIB_HEIGHT },
            (Rectangle){ (float)offsetX, (float)offsetY, PICOLIB_WIDTH * current_scale, PICOLIB_HEIGHT * current_scale },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );
        EndDrawing();
    }
}

void picolib_cleanup(void) {
// Очистка памяти при выходе
    UnloadRenderTexture(target);
    if (spritesheet_loaded) UnloadTexture(sprite_sheet);

    // --- Выгружаем ВСЕ звуки ДО закрытия аудио ---
    #if PICOLIB_USE_AUDIO == 1
        for (int i = 0; i < PICOLIB_MAX_SOUNDS; i++) {
            if (sounds_loaded[i]) UnloadSound(sounds[i]);
        }
    #endif

    #if PICOLIB_USE_TONE == 1
        for (int i = 0; i < PICOLIB_MAX_TONE_SOUNDS; i++) {
            if (tone_loaded[i]) {
                UnloadSound(tone_sounds[i]);
                tone_loaded[i] = false;
            }
        }
    #endif

    // --- ТЕПЕРЬ закрываем аудио ---
    #if PICOLIB_USE_AUDIO == 1 || PICOLIB_USE_TONE == 1
        CloseAudioDevice();
    #endif

    #if PICOLIB_USE_SAVE == 1
        storage_save_all();
    #endif

    ShowCursor();
    CloseWindow();
}