#include <raylib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <time.h>
#include <stdlib.h>   // для getenv
#include <string.h>   // для strcpy/strcat
#include <sys/stat.h> // для mkdir (Linux/macOS)
#include <direct.h>   // для _mkdir (Windows)
#include "picolib.h"
#include "pic2d-core.h"
#include "picolib-lua.h"
#include "miniz.h"


#define PIC2D_DEBUG_LOG 1


void get_save_directory(char* out_path, size_t size) {
    #ifdef _WIN32
        // Windows
        char* appdata = getenv("APPDATA");
        if (appdata) {
            snprintf(out_path, size, "%s\\PIC2D", appdata);
        } else {
            // fallback
            strcpy(out_path, "./PIC2D");
        }
    #elif defined(__APPLE__)
        // macOS
        char* home = getenv("HOME");
        if (home) {
            snprintf(out_path, size, "%s/Library/Application Support/PIC2D", home);
        } else {
            strcpy(out_path, "./PIC2D");
        }
    #else
        // Linux и другие Unix-подобные
        char* home = getenv("HOME");
        if (home) {
            // По стандарту XDG
            char* xdg_data = getenv("XDG_DATA_HOME");
            if (xdg_data) {
                snprintf(out_path, size, "%s/PIC2D", xdg_data);
            } else {
                snprintf(out_path, size, "%s/.local/share/PIC2D", home);
            }
        } else {
            strcpy(out_path, "./PIC2D");
        }
    #endif
}

#ifdef _WIN32
    #include <direct.h>
    #define mkdir _mkdir
#endif

void create_directory(const char* path) {
    #ifdef _WIN32
        _mkdir(path);
    #else
        mkdir(path, 0755);
    #endif
}


// Кастомный обработчик логов Raylib (пишет в файл и дублирует в stderr)
void CustomTraceLog(int logType, const char *text, va_list args) {
    // Преобразуем тип лога в строку
    const char *typeStr = "UNKNOWN";
    switch (logType) {
        case LOG_TRACE:   typeStr = "TRACE";   break;
        case LOG_DEBUG:   typeStr = "DEBUG";   break;
        case LOG_INFO:    typeStr = "INFO";    break;
        case LOG_WARNING: typeStr = "WARNING"; break;
        case LOG_ERROR:   typeStr = "ERROR";   break;
        case LOG_FATAL:   typeStr = "FATAL";   break;
    }

    // Форматируем сообщение
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    // Пишем в файл (если открыт) и в stderr
    FILE *logFile = fopen("log.txt", "a");
    if (logFile) {
        fprintf(logFile, "[%s] %s\n", typeStr, buffer);
        fclose(logFile);
    }
    // Дублируем в stderr (видно в консоли, если она открыта)
    fprintf(stderr, "[%s] %s\n", typeStr, buffer);
}


// Функция извлечения файла из ZIP в память (возвращает строку, которую нужно освободить)
char* extract_file_from_zip(const char *zip_path, const char *file_name) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zip_path, 0)) return NULL;

    int index = mz_zip_reader_locate_file(&zip, file_name, NULL, 0);
    if (index < 0) {
        mz_zip_reader_end(&zip);
        return NULL;
    }

    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&zip, index, &stat)) {
        mz_zip_reader_end(&zip);
        return NULL;
    }

    char *data = malloc(stat.m_uncomp_size + 1);
    if (!data) {
        mz_zip_reader_end(&zip);
        return NULL;
    }
    if (!mz_zip_reader_extract_to_mem(&zip, index, data, stat.m_uncomp_size, 0)) {
        free(data);
        mz_zip_reader_end(&zip);
        return NULL;
    }
    data[stat.m_uncomp_size] = '\0';
    mz_zip_reader_end(&zip);
    return data;
}

// Для бинарных данных
unsigned char* extract_file_data_from_zip(const char* zip_path, const char* file_name, int* out_size) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zip_path, 0)) return NULL;

    int index = mz_zip_reader_locate_file(&zip, file_name, NULL, 0);
    if (index < 0) {
        mz_zip_reader_end(&zip);
        return NULL;
    }

    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&zip, index, &stat)) {
        mz_zip_reader_end(&zip);
        return NULL;
    }

    unsigned char* data = malloc(stat.m_uncomp_size);
    if (!data) {
        mz_zip_reader_end(&zip);
        return NULL;
    }
    if (!mz_zip_reader_extract_to_mem(&zip, index, data, stat.m_uncomp_size, 0)) {
        free(data);
        mz_zip_reader_end(&zip);
        return NULL;
    }
    *out_size = stat.m_uncomp_size;
    mz_zip_reader_end(&zip);
    return data;
}

// Глобальный указатель на Lua, чтобы использовать в update/draw
static lua_State *L = NULL;

// Переопределяем update() для вызова Lua-функции
void _update(void) {
    if (L) {
        lua_getglobal(L, "_update");
        lua_pcall(L, 0, 0, 0);
    }
}

// Переопределяем draw() для вызова Lua-функции
void _draw(void) {
    if (L) {
        lua_getglobal(L, "_draw");
        lua_pcall(L, 0, 0, 0);
    }
}


int main(int argc, char *argv[]) {
    // --- Инициализация окна ---
    int16_t initial_scale = 512 / PICOLIB_WIDTH;
    if (initial_scale < 1) initial_scale = 1;
    InitWindow(PICOLIB_WIDTH * initial_scale, PICOLIB_HEIGHT * initial_scale, "PIC2D");

    // --- Перенаправление stdout и stderr в файл (для printf и прочего) ---
    #if PIC2D_DEBUG_LOG == 1
        // Открываем файл для записи (затираем предыдущий)
        freopen("log.txt", "w", stdout);
        freopen("log.txt", "w", stderr);
        // Отключаем буферизацию, чтобы записи появлялись сразу
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
        // Устанавливаем кастомный обработчик для TraceLog
        SetTraceLogCallback(CustomTraceLog);
    #endif
    
    SetTargetFPS(PICOLIB_FPS);

    HideCursor();

    #if PICOLIB_USE_AUDIO == 1
    InitAudioDevice();
    #endif

    target = LoadRenderTexture(PICOLIB_WIDTH, PICOLIB_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    // --- Определяем картридж ---
    const char *cartridge = NULL;
    if (argc > 1) {
        cartridge = argv[1];
    } else if (FileExists("game.pic")) {
        cartridge = "game.pic";
    }

    if (!cartridge) {
        // Показываем синий экран с сообщением
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(BLUE);
            DrawText("No cartridge found.", 10, 10, 20, WHITE);
            DrawText("Place game.pic in the folder or drag a .pic file.", 10, 30, 20, WHITE);
            EndDrawing();
        }
        CloseWindow();
        return 0;
    }

    // --- Определяем путь к сохранениям ---
    char save_dir[512];
    get_save_directory(save_dir, sizeof(save_dir));
    create_directory(save_dir); // создаём папку, если её нет

    // Извлекаем имя файла без расширения .pic
    const char* filename = strrchr(cartridge, '\\');
    if (!filename) filename = strrchr(cartridge, '/');
    if (!filename) filename = cartridge;
    else filename++; // пропускаем разделитель

    char basename[256];
    strcpy(basename, filename);
    char* dot = strrchr(basename, '.');
    if (dot && strcmp(dot, ".pic") == 0) {
        *dot = '\0'; // убираем расширение
    }

    // Формируем полный путь к .bin файлу
    static char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s/%s.bin", save_dir, basename);
    save_file_path = save_path;


    // --- Проверяем, ZIP ли это ---
    int use_zip = 0;
    size_t len = strlen(cartridge);
    if (len > 4 && strcmp(cartridge + len - 4, ".pic") == 0) {
        use_zip = 1;
    }

    // --- Загружаем спрайты и шрифт ---
    if (use_zip) {
    // --- Загрузка спрайтов ---
    int sprite_size;
    unsigned char* sprite_data = extract_file_data_from_zip(cartridge, "assets/picolib_spritesheet.png", &sprite_size);
    if (sprite_data) {
        pic2d_load_spritesheet_from_memory(sprite_data, sprite_size);
        free(sprite_data);
    } else {
        picolib_load_spritesheet(PICOLIB_SS);
    }

    // --- Загрузка шрифта ---
    int font_size;
    unsigned char* font_data = extract_file_data_from_zip(cartridge, "assets/font_pico8.png", &font_size);
    if (font_data) {
        pic2d_load_font_from_memory(font_data, font_size);
        free(font_data);
    } else {
        picolib_load_font(PICOLIB_FONT);
    }

    // --- Загрузка звуков (ТОЛЬКО ДЛЯ АРХИВА) ---
    #if PICOLIB_USE_AUDIO == 1
        for (int i = 0; i < PICOLIB_MAX_SOUNDS; i++) {
            char path[64];
            snprintf(path, sizeof(path), PICOLIB_SOUNDS_PATH, i);
            int size;
            unsigned char* data = extract_file_data_from_zip(cartridge, path, &size);
            if (data) {
                Wave wave = LoadWaveFromMemory(".wav", data, size);
                if (wave.frameCount > 0) {
                    sounds[i] = LoadSoundFromWave(wave);
                    sounds_loaded[i] = true;
                    UnloadWave(wave);
                } else {
                    sounds_loaded[i] = false;
                }
                free(data);
            } else {
                sounds_loaded[i] = false;
            }
        }
    #endif

    // --- Загрузка карты из архива ---
    #if PICOLIB_USE_MAP == 1
        char* map_data = extract_file_from_zip(cartridge, "assets/map.csv");
        if (map_data) {
            pic2d_load_map_from_memory(map_data);
            free(map_data);
        } else {
            // Карты нет в архиве — можно попробовать загрузить из файловой системы
            // Это произойдёт автоматически при первом вызове map_draw, если map_loaded_flag == 0
        }
    #endif
    } else {
        // --- Обычная загрузка из файловой системы ---
        picolib_load_spritesheet(PICOLIB_SS);
        picolib_load_font(PICOLIB_FONT);

        #if PICOLIB_USE_AUDIO == 1
            picolib_load_sounds();   // загружает звуки из папки sounds/
        #endif
    }


    // --- Инициализация Lua ---
    L = luaL_newstate();
    luaL_openlibs(L);
    picolib_lua_register(L);

    // --- Загружаем Lua-код ---
    char *lua_code = NULL;
    if (use_zip) {
        lua_code = extract_file_from_zip(cartridge, "main.lua");
        if (!lua_code) {
            printf("No main.lua found in cartridge.\n");
            return 1;
        }
        if (luaL_dostring(L, lua_code) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            printf("Lua error: %s\n", err);
            lua_pop(L, 1);
            free(lua_code);
            return 1;
        }
        free(lua_code);
    } else {
        if (luaL_dofile(L, cartridge) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            printf("Lua error: %s\n", err);
            lua_pop(L, 1);
            return 1;
        }
    }

    // --- Вызываем _init() из Lua ---
    lua_getglobal(L, "_init");
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        printf("_init error: %s\n", err);
        lua_pop(L, 1);
        return 1;
    }


    // --- Сохранение ---
    #if PICOLIB_USE_SAVE == 1
        int autosave_counter = 0;
        const int AUTOSAVE_INTERVAL = 60 * 10 * 60; // 10 минут при 60 FPS
    #endif

    
    // --- Главный цикл ---
    while (!WindowShouldClose()) {
        // --- Обработка клавиш ---
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_P)) {
            show_fps = !show_fps;
        }

        // --- Автосохранение (каждые 10 минут) ---
        #if PICOLIB_USE_SAVE == 1
            autosave_counter++;
            if (autosave_counter >= AUTOSAVE_INTERVAL) {
                storage_save_all();
                autosave_counter = 0;
                // можно вывести сообщение "Autosaved" в лог или на экран
            }
        #endif

        // Вызов _update()
        lua_getglobal(L, "_update");
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            printf("_update error: %s\n", err);
            lua_pop(L, 1);
            // Можно продолжить или прервать – решайте
        }

        BeginTextureMode(target);
        // Вызов _draw()
        lua_getglobal(L, "_draw");
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            printf("_draw error: %s\n", err);
            lua_pop(L, 1);
        }

        if (show_fps) {
            int fps = GetFPS();
            print("FPS: %d", cam_x+PICOLIB_WIDTH-30, cam_y+2, 7, fps);
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        int screenW = GetScreenWidth(), screenH = GetScreenHeight();
        float scaleX = (float)screenW / PICOLIB_WIDTH;
        float scaleY = (float)screenH / PICOLIB_HEIGHT;
        float current_scale = (scaleX < scaleY) ? scaleX : scaleY;
        int offsetX = (int)((screenW - PICOLIB_WIDTH * current_scale) / 2);
        int offsetY = (int)((screenH - PICOLIB_HEIGHT * current_scale) / 2);
        DrawTexturePro(target.texture,
            (Rectangle){0, 0, PICOLIB_WIDTH, -PICOLIB_HEIGHT},
            (Rectangle){offsetX, offsetY, PICOLIB_WIDTH * current_scale, PICOLIB_HEIGHT * current_scale},
            (Vector2){0,0}, 0, WHITE);
        EndDrawing();
    }

    // --- Очистка ---
    UnloadRenderTexture(target);
    if (spritesheet_loaded) UnloadTexture(sprite_sheet);

    #if PICOLIB_USE_AUDIO == 1
        for (int i = 0; i < PICOLIB_MAX_SOUNDS; i++) {
            if (sounds_loaded[i]) UnloadSound(sounds[i]);
        }
        CloseAudioDevice();
    #endif

    #if PICOLIB_USE_SAVE == 1
        storage_save_all();
    #endif

    lua_close(L);
    CloseWindow();
    return 0;
}