#ifndef PIC2D_CORE_H
#define PIC2D_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Функции загрузки из памяти (для PIC2D) ---
void pic2d_load_spritesheet_from_memory(const unsigned char* data, int size);
void pic2d_load_font_from_memory(const unsigned char* data, int size);
void pic2d_load_map_from_memory(const char* data);

#ifdef __cplusplus
}
#endif

#endif