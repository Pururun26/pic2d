#include "picolib-lua.h"
#include "picolib.h"   // основная библиотека
#include <lauxlib.h>
#include <lualib.h>
#include <stdint.h>

// --- Биндинги ---

// cls(color)
static int lua_cls(lua_State *L) {
    int color = (int)luaL_checkinteger(L, 1);
    cls(color);
    return 0;
}

// spr(n, x, y, (uint8_t)w, (uint8_t)h, (bool)flip_x, (bool)flip_y)
static int lua_spr(lua_State *L) {
    int n = luaL_checkinteger(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    
    int w = 1, h = 1, flip_x = 0, flip_y = 0;
    if (lua_gettop(L) >= 4) w = luaL_checkinteger(L, 4);
    if (lua_gettop(L) >= 5) h = luaL_checkinteger(L, 5);
    if (lua_gettop(L) >= 6) flip_x = lua_toboolean(L, 6);
    if (lua_gettop(L) >= 7) flip_y = lua_toboolean(L, 7);
    
    spr_pro(n, x, y, (uint8_t)w, (uint8_t)h, (bool)flip_x, (bool)flip_y);
    return 0;
}

static int lua_spr_scale(lua_State *L) {
    int n = luaL_checkinteger(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int zoom = luaL_checkinteger(L, 4);
    spr_scale(n, x, y, (uint8_t)zoom);
    return 0;
}

// print(text, x, y, color)
static int lua_print(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int color = (int)luaL_checkinteger(L, 4);
    print("%s", x, y, color, text);
    return 0;
}

static int lua_circ(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    int color = luaL_checkinteger(L, 4);
    circ(x, y, r, color);
    return 0;
}

static int lua_circfill(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    int color = luaL_checkinteger(L, 4);
    circfill(x, y, r, color);
    return 0;
}

static int lua_rect(lua_State *L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    int color = luaL_checkinteger(L, 5);
    rect(x0, y0, x1, y1, color);
    return 0;
}

static int lua_rectfill(lua_State *L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    int color = luaL_checkinteger(L, 5);
    rectfill(x0, y0, x1, y1, color);
    return 0;
}

// --- Кнопки ---
static int lua_btn(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    lua_pushboolean(L, btn((uint8_t)key));
    return 1;
}

static int lua_btnp(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    lua_pushboolean(L, btnp((uint8_t)key));
    return 1;
}

// --- Мышка ---
static int lua_mouse(lua_State *L) {
    picolib_mouse m = mouse();
    lua_newtable(L);
    lua_pushinteger(L, m.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, m.y);
    lua_setfield(L, -2, "y");
    lua_pushboolean(L, m.left);
    lua_setfield(L, -2, "left");
    lua_pushboolean(L, m.middle);
    lua_setfield(L, -2, "middle");
    lua_pushboolean(L, m.right);
    lua_setfield(L, -2, "right");
    lua_pushinteger(L, m.scrollx);
    lua_setfield(L, -2, "scrollx");
    lua_pushinteger(L, m.scrolly);
    lua_setfield(L, -2, "scrolly");
    return 1;
}

static int lua_mousep(lua_State *L) {
    picolib_mouse m = mousep();
    lua_newtable(L);
    lua_pushinteger(L, m.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, m.y);
    lua_setfield(L, -2, "y");
    lua_pushboolean(L, m.left);
    lua_setfield(L, -2, "left");
    lua_pushboolean(L, m.middle);
    lua_setfield(L, -2, "middle");
    lua_pushboolean(L, m.right);
    lua_setfield(L, -2, "right");
    lua_pushinteger(L, m.scrollx);
    lua_setfield(L, -2, "scrollx");
    lua_pushinteger(L, m.scrolly);
    lua_setfield(L, -2, "scrolly");
    return 1;
}

// --- Камера ---
static int lua_camera(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    picolib_vec2 prev = camera((int16_t)x, (int16_t)y);
    lua_pushinteger(L, prev.x);
    lua_pushinteger(L, prev.y);
    return 2; // два возвращаемых значения
}

// --- Звук ---
#if PICOLIB_USE_AUDIO == 1
static int lua_sfx(lua_State *L) {
    int index = luaL_checkinteger(L, 1);
    sfx(index);
    return 0;
}
#endif

// --- Сохранение ---
// save(pos, value)
static int lua_save_storage(lua_State *L) {
    int pos = luaL_checkinteger(L, 1);
    uint64_t value = (uint64_t)luaL_checkinteger(L, 2);
    save((uint8_t)pos, value);
    return 0;
}

// load(pos) -> value
static int lua_load_storage(lua_State *L) {
    int pos = luaL_checkinteger(L, 1);
    uint64_t value = load((uint8_t)pos);
    lua_pushinteger(L, (lua_Integer)value);
    return 1;
}

// is_save() -> bool (проверяет, существует ли файл сохранения)
static int lua_is_save(lua_State *L) {
    lua_pushboolean(L, is_save());
    return 1;
}

// --- Карта ---
// map() -> рисует всю карту (0,0,0,0,MAP_COLS,MAP_ROWS)
// map(celx, cely) -> рисует всю карту, начиная с (celx, cely) (смещение по карте)
// map(celx, cely, sx, sy) -> рисует всю карту со смещением на экране (sx, sy)
// map(celx, cely, sx, sy, celw, celh) -> рисует только указанную область карты
static int lua_map_draw(lua_State *L) {
    int argc = lua_gettop(L);
    
    // Значения по умолчанию
    int celx = 0;
    int cely = 0;
    int sx = 0;
    int sy = 0;
    int celw = MAP_COLS;
    int celh = MAP_ROWS;

    if (argc == 6) {
        celx = (int)luaL_checkinteger(L, 1);
        cely = (int)luaL_checkinteger(L, 2);
        sx = (int)luaL_checkinteger(L, 3);
        sy = (int)luaL_checkinteger(L, 4);
        celw = (int)luaL_checkinteger(L, 5);
        celh = (int)luaL_checkinteger(L, 6);
    }
    else if (argc == 4) {
        celx = (int)luaL_checkinteger(L, 1);
        cely = (int)luaL_checkinteger(L, 2);
        sx = (int)luaL_checkinteger(L, 3);
        sy = (int)luaL_checkinteger(L, 4);
        // celw и celh остаются MAP_COLS и MAP_ROWS
    }
    else if (argc == 2) {
        celx = (int)luaL_checkinteger(L, 1);
        cely = (int)luaL_checkinteger(L, 2);
        // sx, sy, celw, celh — по умолчанию
    }
    // если 0, 1, 3, 5 аргументов — используем всю карту (без предупреждения)
    
    map_draw(celx, cely, sx, sy, celw, celh);
    return 0;
}

// mget(x, y) -> id
static int lua_mget(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    uint8_t id = mget(x, y);
    lua_pushinteger(L, id);
    return 1;
}

// mset(x, y, id)
static int lua_mset(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int id = (int)luaL_checkinteger(L, 3);
    mset(x, y, (uint8_t)id);
    return 0;
}


// Регистрация всех функций
void picolib_lua_register(lua_State *L) {
    lua_register(L, "cls", lua_cls);
    lua_register(L, "spr", lua_spr);
    lua_register(L, "spr_scale", lua_spr_scale);
    lua_register(L, "print", lua_print);
    lua_register(L, "circ", lua_circ);
    lua_register(L, "circfill", lua_circfill);
    lua_register(L, "rect", lua_rect);
    lua_register(L, "rectfill", lua_rectfill);
    lua_register(L, "btn", lua_btn);
    lua_register(L, "btnp", lua_btnp);
    lua_register(L, "camera", lua_camera);
    lua_register(L, "mouse", lua_mouse);
    lua_register(L, "mousep", lua_mousep);
    #if PICOLIB_USE_AUDIO == 1
    lua_register(L, "sfx", lua_sfx);
    #endif
    lua_register(L, "save", lua_save_storage);
    lua_register(L, "load", lua_load_storage);
    lua_register(L, "is_save", lua_is_save);
    lua_register(L, "map", lua_map_draw);
    lua_register(L, "mget", lua_mget);
    lua_register(L, "mset", lua_mset);
}