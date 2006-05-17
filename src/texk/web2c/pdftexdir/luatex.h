/* $Id */

#include <stdlib.h>
#include <stdio.h>
#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>
#include <../lua51/lualib.h>

#define BUFSIZE (4096*256)

extern char *buf;
extern int bufloc;

typedef struct LoadS {
    const char *s;
    size_t size;
} LoadS;

extern int poolprint(lua_State *L);

extern int setdimen (lua_State *L);
extern int getdimen (lua_State *L);
extern int setcount (lua_State *L);
extern int getcount (lua_State *L);
extern int settoks  (lua_State *L);
extern int gettoks  (lua_State *L);

extern int getboxwd (lua_State *L);
extern int getboxht (lua_State *L);
extern int getboxdp (lua_State *L);

extern int setboxwd (lua_State *L);
extern int setboxht (lua_State *L);
extern int setboxdp (lua_State *L);

extern int findcurv  (lua_State *L);
extern int findcurh  (lua_State *L);

extern int makecurv  (lua_State *L);
extern int makecurh  (lua_State *L);

extern void luatex_init (int s, LoadS *ls);

extern void luatex_return (void);

extern void luatex_error (char *err);
