/*
 * This file is auto-generated by "./lua2c socket.lua socket_lua"
 */

#include "lua.h"
#include "lauxlib.h"

int luatex_socket_lua_open (lua_State *L) { 
    static unsigned char B[] = {
    45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 45, 45,
 32, 76,117, 97, 83,111, 99,107,101,116, 32,104,101,108,112,101,114, 32,109,111,
100,117,108,101, 10, 45, 45, 32, 65,117,116,104,111,114, 58, 32, 68,105,101,103,
111, 32, 78,101,104, 97, 98, 10, 45, 45, 32, 82, 67, 83, 32, 73, 68, 58, 32, 36,
 73,100, 58, 32,115,111, 99,107,101,116, 46,108,117, 97, 44,118, 32, 49, 46, 50,
 50, 32, 50, 48, 48, 53, 47, 49, 49, 47, 50, 50, 32, 48, 56, 58, 51, 51, 58, 50,
 57, 32,100,105,101,103,111, 32, 69,120,112, 32, 36, 10, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 10, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 45, 45, 32, 68,101, 99,108, 97,114,
101, 32,109,111,100,117,108,101, 32, 97,110,100, 32,105,109,112,111,114,116, 32,
100,101,112,101,110,100,101,110, 99,105,101,115, 10, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10,108,111, 99, 97,108, 32, 98, 97,115,
101, 32, 61, 32, 95, 71, 10,108,111, 99, 97,108, 32,115,116,114,105,110,103, 32,
 61, 32,114,101,113,117,105,114,101, 40, 34,115,116,114,105,110,103, 34, 41, 10,
108,111, 99, 97,108, 32,109, 97,116,104, 32, 61, 32,114,101,113,117,105,114,101,
 40, 34,109, 97,116,104, 34, 41, 10,108,111, 99, 97,108, 32,115,111, 99,107,101,
116, 32, 61, 32,114,101,113,117,105,114,101, 40, 34,115,111, 99,107,101,116, 46,
 99,111,114,101, 34, 41, 10,109,111,100,117,108,101, 40, 34,115,111, 99,107,101,
116, 34, 41, 10, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 10, 45, 45, 32, 69,120,112,111,114,116,101,100, 32, 97,117,120,105,108,
105, 97,114, 32,102,117,110, 99,116,105,111,110,115, 10, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10,102,117,110, 99,116,105,111,110,
 32, 99,111,110,110,101, 99,116, 40, 97,100,100,114,101,115,115, 44, 32,112,111,
114,116, 44, 32,108, 97,100,100,114,101,115,115, 44, 32,108,112,111,114,116, 41,
 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,115,111, 99,107, 44, 32,101,114,114,
 32, 61, 32,115,111, 99,107,101,116, 46,116, 99,112, 40, 41, 10, 32, 32, 32, 32,
105,102, 32,110,111,116, 32,115,111, 99,107, 32,116,104,101,110, 32,114,101,116,
117,114,110, 32,110,105,108, 44, 32,101,114,114, 32,101,110,100, 10, 32, 32, 32,
 32,105,102, 32,108, 97,100,100,114,101,115,115, 32,116,104,101,110, 10, 32, 32,
 32, 32, 32, 32, 32, 32,108,111, 99, 97,108, 32,114,101,115, 44, 32,101,114,114,
 32, 61, 32,115,111, 99,107, 58, 98,105,110,100, 40,108, 97,100,100,114,101,115,
115, 44, 32,108,112,111,114,116, 44, 32, 45, 49, 41, 10, 32, 32, 32, 32, 32, 32,
 32, 32,105,102, 32,110,111,116, 32,114,101,115, 32,116,104,101,110, 32,114,101,
116,117,114,110, 32,110,105,108, 44, 32,101,114,114, 32,101,110,100, 10, 32, 32,
 32, 32,101,110,100, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,114,101,115, 44,
 32,101,114,114, 32, 61, 32,115,111, 99,107, 58, 99,111,110,110,101, 99,116, 40,
 97,100,100,114,101,115,115, 44, 32,112,111,114,116, 41, 10, 32, 32, 32, 32,105,
102, 32,110,111,116, 32,114,101,115, 32,116,104,101,110, 32,114,101,116,117,114,
110, 32,110,105,108, 44, 32,101,114,114, 32,101,110,100, 10, 32, 32, 32, 32,114,
101,116,117,114,110, 32,115,111, 99,107, 10,101,110,100, 10, 10,102,117,110, 99,
116,105,111,110, 32, 98,105,110,100, 40,104,111,115,116, 44, 32,112,111,114,116,
 44, 32, 98, 97, 99,107,108,111,103, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108,
 32,115,111, 99,107, 44, 32,101,114,114, 32, 61, 32,115,111, 99,107,101,116, 46,
116, 99,112, 40, 41, 10, 32, 32, 32, 32,105,102, 32,110,111,116, 32,115,111, 99,
107, 32,116,104,101,110, 32,114,101,116,117,114,110, 32,110,105,108, 44, 32,101,
114,114, 32,101,110,100, 10, 32, 32, 32, 32,115,111, 99,107, 58,115,101,116,111,
112,116,105,111,110, 40, 34,114,101,117,115,101, 97,100,100,114, 34, 44, 32,116,
114,117,101, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,114,101,115, 44, 32,
101,114,114, 32, 61, 32,115,111, 99,107, 58, 98,105,110,100, 40,104,111,115,116,
 44, 32,112,111,114,116, 41, 10, 32, 32, 32, 32,105,102, 32,110,111,116, 32,114,
101,115, 32,116,104,101,110, 32,114,101,116,117,114,110, 32,110,105,108, 44, 32,
101,114,114, 32,101,110,100, 10, 32, 32, 32, 32,114,101,115, 44, 32,101,114,114,
 32, 61, 32,115,111, 99,107, 58,108,105,115,116,101,110, 40, 98, 97, 99,107,108,
111,103, 41, 10, 32, 32, 32, 32,105,102, 32,110,111,116, 32,114,101,115, 32,116,
104,101,110, 32,114,101,116,117,114,110, 32,110,105,108, 44, 32,101,114,114, 32,
101,110,100, 10, 32, 32, 32, 32,114,101,116,117,114,110, 32,115,111, 99,107, 10,
101,110,100, 10, 10,116,114,121, 32, 61, 32,110,101,119,116,114,121, 40, 41, 10,
 10,102,117,110, 99,116,105,111,110, 32, 99,104,111,111,115,101, 40,116, 97, 98,
108,101, 41, 10, 32, 32, 32, 32,114,101,116,117,114,110, 32,102,117,110, 99,116,
105,111,110, 40,110, 97,109,101, 44, 32,111,112,116, 49, 44, 32,111,112,116, 50,
 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,105,102, 32, 98, 97,115,101, 46,116,121,
112,101, 40,110, 97,109,101, 41, 32,126, 61, 32, 34,115,116,114,105,110,103, 34,
 32,116,104,101,110, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,110, 97,
109,101, 44, 32,111,112,116, 49, 44, 32,111,112,116, 50, 32, 61, 32, 34,100,101,
102, 97,117,108,116, 34, 44, 32,110, 97,109,101, 44, 32,111,112,116, 49, 10, 32,
 32, 32, 32, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32, 32, 32, 32, 32,108,
111, 99, 97,108, 32,102, 32, 61, 32,116, 97, 98,108,101, 91,110, 97,109,101, 32,
111,114, 32, 34,110,105,108, 34, 93, 10, 32, 32, 32, 32, 32, 32, 32, 32,105,102,
 32,110,111,116, 32,102, 32,116,104,101,110, 32, 98, 97,115,101, 46,101,114,114,
111,114, 40, 34,117,110,107,110,111,119,110, 32,107,101,121, 32, 40, 34, 46, 46,
 32, 98, 97,115,101, 46,116,111,115,116,114,105,110,103, 40,110, 97,109,101, 41,
 32, 46, 46, 34, 41, 34, 44, 32, 51, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,101,
108,115,101, 32,114,101,116,117,114,110, 32,102, 40,111,112,116, 49, 44, 32,111,
112,116, 50, 41, 32,101,110,100, 10, 32, 32, 32, 32,101,110,100, 10,101,110,100,
 10, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10,
 45, 45, 32, 83,111, 99,107,101,116, 32,115,111,117,114, 99,101,115, 32, 97,110,
100, 32,115,105,110,107,115, 44, 32, 99,111,110,102,111,114,109,105,110,103, 32,
116,111, 32, 76, 84, 78, 49, 50, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 10, 45, 45, 32, 99,114,101, 97,116,101, 32,110, 97,109,
101,115,112, 97, 99,101,115, 32,105,110,115,105,100,101, 32, 76,117, 97, 83,111,
 99,107,101,116, 32,110, 97,109,101,115,112, 97, 99,101, 10,115,111,117,114, 99,
101,116, 32, 61, 32,123,125, 10,115,105,110,107,116, 32, 61, 32,123,125, 10, 10,
 66, 76, 79, 67, 75, 83, 73, 90, 69, 32, 61, 32, 50, 48, 52, 56, 10, 10,115,105,
110,107,116, 91, 34, 99,108,111,115,101, 45,119,104,101,110, 45,100,111,110,101,
 34, 93, 32, 61, 32,102,117,110, 99,116,105,111,110, 40,115,111, 99,107, 41, 10,
 32, 32, 32, 32,114,101,116,117,114,110, 32, 98, 97,115,101, 46,115,101,116,109,
101,116, 97,116, 97, 98,108,101, 40,123, 10, 32, 32, 32, 32, 32, 32, 32, 32,103,
101,116,102,100, 32, 61, 32,102,117,110, 99,116,105,111,110, 40, 41, 32,114,101,
116,117,114,110, 32,115,111, 99,107, 58,103,101,116,102,100, 40, 41, 32,101,110,
100, 44, 10, 32, 32, 32, 32, 32, 32, 32, 32,100,105,114,116,121, 32, 61, 32,102,
117,110, 99,116,105,111,110, 40, 41, 32,114,101,116,117,114,110, 32,115,111, 99,
107, 58,100,105,114,116,121, 40, 41, 32,101,110,100, 10, 32, 32, 32, 32,125, 44,
 32,123, 10, 32, 32, 32, 32, 32, 32, 32, 32, 95, 95, 99, 97,108,108, 32, 61, 32,
102,117,110, 99,116,105,111,110, 40,115,101,108,102, 44, 32, 99,104,117,110,107,
 44, 32,101,114,114, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,105,
102, 32,110,111,116, 32, 99,104,117,110,107, 32,116,104,101,110, 10, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,115,111, 99,107, 58, 99,108,
111,115,101, 40, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32,114,101,116,117,114,110, 32, 49, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32,101,108,115,101, 32,114,101,116,117,114,110, 32,115,111, 99,107, 58,
115,101,110,100, 40, 99,104,117,110,107, 41, 32,101,110,100, 10, 32, 32, 32, 32,
 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32,125, 41, 10,101,110,100, 10, 10,
115,105,110,107,116, 91, 34,107,101,101,112, 45,111,112,101,110, 34, 93, 32, 61,
 32,102,117,110, 99,116,105,111,110, 40,115,111, 99,107, 41, 10, 32, 32, 32, 32,
114,101,116,117,114,110, 32, 98, 97,115,101, 46,115,101,116,109,101,116, 97,116,
 97, 98,108,101, 40,123, 10, 32, 32, 32, 32, 32, 32, 32, 32,103,101,116,102,100,
 32, 61, 32,102,117,110, 99,116,105,111,110, 40, 41, 32,114,101,116,117,114,110,
 32,115,111, 99,107, 58,103,101,116,102,100, 40, 41, 32,101,110,100, 44, 10, 32,
 32, 32, 32, 32, 32, 32, 32,100,105,114,116,121, 32, 61, 32,102,117,110, 99,116,
105,111,110, 40, 41, 32,114,101,116,117,114,110, 32,115,111, 99,107, 58,100,105,
114,116,121, 40, 41, 32,101,110,100, 10, 32, 32, 32, 32,125, 44, 32,123, 10, 32,
 32, 32, 32, 32, 32, 32, 32, 95, 95, 99, 97,108,108, 32, 61, 32,102,117,110, 99,
116,105,111,110, 40,115,101,108,102, 44, 32, 99,104,117,110,107, 44, 32,101,114,
114, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,105,102, 32, 99,104,
117,110,107, 32,116,104,101,110, 32,114,101,116,117,114,110, 32,115,111, 99,107,
 58,115,101,110,100, 40, 99,104,117,110,107, 41, 10, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32,101,108,115,101, 32,114,101,116,117,114,110, 32, 49, 32,101,
110,100, 10, 32, 32, 32, 32, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32,125,
 41, 10,101,110,100, 10, 10,115,105,110,107,116, 91, 34,100,101,102, 97,117,108,
116, 34, 93, 32, 61, 32,115,105,110,107,116, 91, 34,107,101,101,112, 45,111,112,
101,110, 34, 93, 10, 10,115,105,110,107, 32, 61, 32, 99,104,111,111,115,101, 40,
115,105,110,107,116, 41, 10, 10,115,111,117,114, 99,101,116, 91, 34, 98,121, 45,
108,101,110,103,116,104, 34, 93, 32, 61, 32,102,117,110, 99,116,105,111,110, 40,
115,111, 99,107, 44, 32,108,101,110,103,116,104, 41, 10, 32, 32, 32, 32,114,101,
116,117,114,110, 32, 98, 97,115,101, 46,115,101,116,109,101,116, 97,116, 97, 98,
108,101, 40,123, 10, 32, 32, 32, 32, 32, 32, 32, 32,103,101,116,102,100, 32, 61,
 32,102,117,110, 99,116,105,111,110, 40, 41, 32,114,101,116,117,114,110, 32,115,
111, 99,107, 58,103,101,116,102,100, 40, 41, 32,101,110,100, 44, 10, 32, 32, 32,
 32, 32, 32, 32, 32,100,105,114,116,121, 32, 61, 32,102,117,110, 99,116,105,111,
110, 40, 41, 32,114,101,116,117,114,110, 32,115,111, 99,107, 58,100,105,114,116,
121, 40, 41, 32,101,110,100, 10, 32, 32, 32, 32,125, 44, 32,123, 10, 32, 32, 32,
 32, 32, 32, 32, 32, 95, 95, 99, 97,108,108, 32, 61, 32,102,117,110, 99,116,105,
111,110, 40, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,105,102, 32,
108,101,110,103,116,104, 32, 60, 61, 32, 48, 32,116,104,101,110, 32,114,101,116,
117,114,110, 32,110,105,108, 32,101,110,100, 10, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32,108,111, 99, 97,108, 32,115,105,122,101, 32, 61, 32,109, 97,116,
104, 46,109,105,110, 40,115,111, 99,107,101,116, 46, 66, 76, 79, 67, 75, 83, 73,
 90, 69, 44, 32,108,101,110,103,116,104, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32,108,111, 99, 97,108, 32, 99,104,117,110,107, 44, 32,101,114,114,
 32, 61, 32,115,111, 99,107, 58,114,101, 99,101,105,118,101, 40,115,105,122,101,
 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,105,102, 32,101,114,114,
 32,116,104,101,110, 32,114,101,116,117,114,110, 32,110,105,108, 44, 32,101,114,
114, 32,101,110,100, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,108,101,
110,103,116,104, 32, 61, 32,108,101,110,103,116,104, 32, 45, 32,115,116,114,105,
110,103, 46,108,101,110, 40, 99,104,117,110,107, 41, 10, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32,114,101,116,117,114,110, 32, 99,104,117,110,107, 10, 32,
 32, 32, 32, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32,125, 41, 10,101,110,
100, 10, 10,115,111,117,114, 99,101,116, 91, 34,117,110,116,105,108, 45, 99,108,
111,115,101,100, 34, 93, 32, 61, 32,102,117,110, 99,116,105,111,110, 40,115,111,
 99,107, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,100,111,110,101, 10, 32,
 32, 32, 32,114,101,116,117,114,110, 32, 98, 97,115,101, 46,115,101,116,109,101,
116, 97,116, 97, 98,108,101, 40,123, 10, 32, 32, 32, 32, 32, 32, 32, 32,103,101,
116,102,100, 32, 61, 32,102,117,110, 99,116,105,111,110, 40, 41, 32,114,101,116,
117,114,110, 32,115,111, 99,107, 58,103,101,116,102,100, 40, 41, 32,101,110,100,
 44, 10, 32, 32, 32, 32, 32, 32, 32, 32,100,105,114,116,121, 32, 61, 32,102,117,
110, 99,116,105,111,110, 40, 41, 32,114,101,116,117,114,110, 32,115,111, 99,107,
 58,100,105,114,116,121, 40, 41, 32,101,110,100, 10, 32, 32, 32, 32,125, 44, 32,
123, 10, 32, 32, 32, 32, 32, 32, 32, 32, 95, 95, 99, 97,108,108, 32, 61, 32,102,
117,110, 99,116,105,111,110, 40, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32,105,102, 32,100,111,110,101, 32,116,104,101,110, 32,114,101,116,117,114,
110, 32,110,105,108, 32,101,110,100, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32,108,111, 99, 97,108, 32, 99,104,117,110,107, 44, 32,101,114,114, 44, 32,
112, 97,114,116,105, 97,108, 32, 61, 32,115,111, 99,107, 58,114,101, 99,101,105,
118,101, 40,115,111, 99,107,101,116, 46, 66, 76, 79, 67, 75, 83, 73, 90, 69, 41,
 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,105,102, 32,110,111,116, 32,
101,114,114, 32,116,104,101,110, 32,114,101,116,117,114,110, 32, 99,104,117,110,
107, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,101,108,115,101,105,102,
 32,101,114,114, 32, 61, 61, 32, 34, 99,108,111,115,101,100, 34, 32,116,104,101,
110, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,115,111,
 99,107, 58, 99,108,111,115,101, 40, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32,100,111,110,101, 32, 61, 32, 49, 10, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,114,101,116,117,114,110, 32,112,
 97,114,116,105, 97,108, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,101,
108,115,101, 32,114,101,116,117,114,110, 32,110,105,108, 44, 32,101,114,114, 32,
101,110,100, 10, 32, 32, 32, 32, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32,
125, 41, 10,101,110,100, 10, 10, 10,115,111,117,114, 99,101,116, 91, 34,100,101,
102, 97,117,108,116, 34, 93, 32, 61, 32,115,111,117,114, 99,101,116, 91, 34,117,
110,116,105,108, 45, 99,108,111,115,101,100, 34, 93, 10, 10,115,111,117,114, 99,
101, 32, 61, 32, 99,104,111,111,115,101, 40,115,111,117,114, 99,101,116, 41, 10,
 10,
 0 };
  return luaL_dostring(L, (const char*)B); 
} /* end of embedded lua code */

