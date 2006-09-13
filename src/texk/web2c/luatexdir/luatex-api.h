/* $Id */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>
#include <../lua51/lualib.h>


typedef struct LoadS {
    const char *s;
    size_t size;
} LoadS;

extern lua_State *Luas[];

extern void make_table (lua_State *L, char *tab, char *getfunc, char*setfunc);

extern int luaopen_tex (lua_State *L);

extern int luaopen_pdf (lua_State *L);

#define LUA_TEXFILEHANDLE		"TEXFILE*"

extern int luaopen_texio (lua_State *L);

extern void luatex_load_init (int s, LoadS *ls);

extern lua_State *luatex_error (lua_State *L, int fatal);

extern int luaopen_unicode (lua_State *L);

extern int callbackdefined (char *name);

extern int runcallback (char *name, char *values, ...);

extern void luainitialize (int luaid, int format);

extern int luaopen_kpse (lua_State *L);

extern int luaopen_callback (lua_State *L);

extern int callback_initialize (void);
