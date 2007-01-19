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
extern int luaopen_zip (lua_State *L);
extern int luaopen_lfs (lua_State *L);
extern int luaopen_lpeg (lua_State *L);
extern int luaopen_md5 (lua_State *L);

extern int callbackdefined (char *name);

extern int  runcallback          (int i, char *values, ...);
extern int  runsavedcallback     (int i, char *name, char *values, ...);
extern int  runandsavecallback   (int i, char *values, ...);
extern void destroysavedcallback (int i);

extern void getsavedluaboolean   (int i, char *name, int *target);
extern void getsavedluanumber    (int i, char *name, int *target);
extern void getsavedluastring    (int i, char *name, char **target);

extern void getluaboolean        (char *table, char *name, int *target);
extern void getluanumber         (char *table, char *name, int *target);
extern void getluastring         (char *table, char *name, char **target);

extern void initfilecallbackids    (int max);
extern void setinputfilecallbackid (int n, int i) ;
extern void setreadfilecallbackid  (int n, int i) ;
extern int  getinputfilecallbackid (int n);
extern int  getreadfilecallbackid  (int n);

extern void lua_initialize (int ac, char **av);

extern int luaopen_kpse (lua_State *L);

extern int luaopen_callback (lua_State *L);

extern int luaopen_lua (lua_State *L, int n, char *fname);

extern int luaopen_stats (lua_State *L);

extern int luaopen_font (lua_State *L);
extern int font_to_lua   (lua_State *L, int f) ;
extern int font_from_lua (lua_State *L, int f) ; /* return is boolean */


extern void dumpluacregisters (void);

extern void undumpluacregisters (void);


extern void unhide_lua_table(lua_State *lua, char *name, int r);
extern int  hide_lua_table  (lua_State *lua, char *name);

extern void unhide_lua_value(lua_State *lua, char *name, char *item, int r);
extern int  hide_lua_value  (lua_State *lua, char *name, char *item);

