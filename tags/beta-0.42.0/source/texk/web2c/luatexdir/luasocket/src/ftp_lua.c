/*
 * This file is auto-generated by "./lua2c ftp.lua ftp_lua"
 */

#include "lua.h"
#include "lauxlib.h"

int luatex_ftp_lua_open (lua_State *L) { 
    static unsigned char B[] = {
    45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 45, 45,
 32, 70, 84, 80, 32,115,117,112,112,111,114,116, 32,102,111,114, 32,116,104,101,
 32, 76,117, 97, 32,108, 97,110,103,117, 97,103,101, 10, 45, 45, 32, 76,117, 97,
 83,111, 99,107,101,116, 32,116,111,111,108,107,105,116, 46, 10, 45, 45, 32, 65,
117,116,104,111,114, 58, 32, 68,105,101,103,111, 32, 78,101,104, 97, 98, 10, 45,
 45, 32, 82, 67, 83, 32, 73, 68, 58, 32, 36, 73,100, 58, 32,102,116,112, 46,108,
117, 97, 44,118, 32, 49, 46, 52, 53, 32, 50, 48, 48, 55, 47, 48, 55, 47, 49, 49,
 32, 49, 57, 58, 50, 53, 58, 52, 55, 32,100,105,101,103,111, 32, 69,120,112, 32,
 36, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10,
 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 45,
 45, 32, 68,101, 99,108, 97,114,101, 32,109,111,100,117,108,101, 32, 97,110,100,
 32,105,109,112,111,114,116, 32,100,101,112,101,110,100,101,110, 99,105,101,115,
 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10,108,
111, 99, 97,108, 32, 98, 97,115,101, 32, 61, 32, 95, 71, 10,108,111, 99, 97,108,
 32,116, 97, 98,108,101, 32, 61, 32,114,101,113,117,105,114,101, 40, 34,116, 97,
 98,108,101, 34, 41, 10,108,111, 99, 97,108, 32,115,116,114,105,110,103, 32, 61,
 32,114,101,113,117,105,114,101, 40, 34,115,116,114,105,110,103, 34, 41, 10,108,
111, 99, 97,108, 32,109, 97,116,104, 32, 61, 32,114,101,113,117,105,114,101, 40,
 34,109, 97,116,104, 34, 41, 10,108,111, 99, 97,108, 32,115,111, 99,107,101,116,
 32, 61, 32,114,101,113,117,105,114,101, 40, 34,115,111, 99,107,101,116, 34, 41,
 10,108,111, 99, 97,108, 32,117,114,108, 32, 61, 32,114,101,113,117,105,114,101,
 40, 34,115,111, 99,107,101,116, 46,117,114,108, 34, 41, 10,108,111, 99, 97,108,
 32,116,112, 32, 61, 32,114,101,113,117,105,114,101, 40, 34,115,111, 99,107,101,
116, 46,116,112, 34, 41, 10,108,111, 99, 97,108, 32,108,116,110, 49, 50, 32, 61,
 32,114,101,113,117,105,114,101, 40, 34,108,116,110, 49, 50, 34, 41, 10,109,111,
100,117,108,101, 40, 34,115,111, 99,107,101,116, 46,102,116,112, 34, 41, 10, 10,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 45, 45,
 32, 80,114,111,103,114, 97,109, 32, 99,111,110,115,116, 97,110,116,115, 10, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 45, 45, 32,
116,105,109,101,111,117,116, 32,105,110, 32,115,101, 99,111,110,100,115, 32, 98,
101,102,111,114,101, 32,116,104,101, 32,112,114,111,103,114, 97,109, 32,103,105,
118,101,115, 32,117,112, 32,111,110, 32, 97, 32, 99,111,110,110,101, 99,116,105,
111,110, 10, 84, 73, 77, 69, 79, 85, 84, 32, 61, 32, 54, 48, 10, 45, 45, 32,100,
101,102, 97,117,108,116, 32,112,111,114,116, 32,102,111,114, 32,102,116,112, 32,
115,101,114,118,105, 99,101, 10, 80, 79, 82, 84, 32, 61, 32, 50, 49, 10, 45, 45,
 32,116,104,105,115, 32,105,115, 32,116,104,101, 32,100,101,102, 97,117,108,116,
 32, 97,110,111,110,121,109,111,117,115, 32,112, 97,115,115,119,111,114,100, 46,
 32,117,115,101,100, 32,119,104,101,110, 32,110,111, 32,112, 97,115,115,119,111,
114,100, 32,105,115, 10, 45, 45, 32,112,114,111,118,105,100,101,100, 32,105,110,
 32,117,114,108, 46, 32,115,104,111,117,108,100, 32, 98,101, 32, 99,104, 97,110,
103,101,100, 32,116,111, 32,121,111,117,114, 32,101, 45,109, 97,105,108, 46, 10,
 85, 83, 69, 82, 32, 61, 32, 34,102,116,112, 34, 10, 80, 65, 83, 83, 87, 79, 82,
 68, 32, 61, 32, 34, 97,110,111,110,121,109,111,117,115, 64, 97,110,111,110,121,
109,111,117,115, 46,111,114,103, 34, 10, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 10, 45, 45, 32, 76,111,119, 32,108,101,118,101,
108, 32, 70, 84, 80, 32, 65, 80, 73, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 10,108,111, 99, 97,108, 32,109,101,116, 97,116, 32,
 61, 32,123, 32, 95, 95,105,110,100,101,120, 32, 61, 32,123,125, 32,125, 10, 10,
102,117,110, 99,116,105,111,110, 32,111,112,101,110, 40,115,101,114,118,101,114,
 44, 32,112,111,114,116, 44, 32, 99,114,101, 97,116,101, 41, 10, 32, 32, 32, 32,
108,111, 99, 97,108, 32,116,112, 32, 61, 32,115,111, 99,107,101,116, 46,116,114,
121, 40,116,112, 46, 99,111,110,110,101, 99,116, 40,115,101,114,118,101,114, 44,
 32,112,111,114,116, 32,111,114, 32, 80, 79, 82, 84, 44, 32, 84, 73, 77, 69, 79,
 85, 84, 44, 32, 99,114,101, 97,116,101, 41, 41, 10, 32, 32, 32, 32,108,111, 99,
 97,108, 32,102, 32, 61, 32, 98, 97,115,101, 46,115,101,116,109,101,116, 97,116,
 97, 98,108,101, 40,123, 32,116,112, 32, 61, 32,116,112, 32,125, 44, 32,109,101,
116, 97,116, 41, 10, 32, 32, 32, 32, 45, 45, 32,109, 97,107,101, 32,115,117,114,
101, 32,101,118,101,114,121,116,104,105,110,103, 32,103,101,116,115, 32, 99,108,
111,115,101,100, 32,105,110, 32, 97,110, 32,101,120, 99,101,112,116,105,111,110,
 10, 32, 32, 32, 32,102, 46,116,114,121, 32, 61, 32,115,111, 99,107,101,116, 46,
110,101,119,116,114,121, 40,102,117,110, 99,116,105,111,110, 40, 41, 32,102, 58,
 99,108,111,115,101, 40, 41, 32,101,110,100, 41, 10, 32, 32, 32, 32,114,101,116,
117,114,110, 32,102, 10,101,110,100, 10, 10,102,117,110, 99,116,105,111,110, 32,
109,101,116, 97,116, 46, 95, 95,105,110,100,101,120, 58,112,111,114,116, 99,111,
110,110,101, 99,116, 40, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121,
 40,115,101,108,102, 46,115,101,114,118,101,114, 58,115,101,116,116,105,109,101,
111,117,116, 40, 84, 73, 77, 69, 79, 85, 84, 41, 41, 10, 32, 32, 32, 32,115,101,
108,102, 46,100, 97,116, 97, 32, 61, 32,115,101,108,102, 46,116,114,121, 40,115,
101,108,102, 46,115,101,114,118,101,114, 58, 97, 99, 99,101,112,116, 40, 41, 41,
 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,100,
 97,116, 97, 58,115,101,116,116,105,109,101,111,117,116, 40, 84, 73, 77, 69, 79,
 85, 84, 41, 41, 10,101,110,100, 10, 10,102,117,110, 99,116,105,111,110, 32,109,
101,116, 97,116, 46, 95, 95,105,110,100,101,120, 58,112, 97,115,118, 99,111,110,
110,101, 99,116, 40, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,100, 97,116, 97,
 32, 61, 32,115,101,108,102, 46,116,114,121, 40,115,111, 99,107,101,116, 46,116,
 99,112, 40, 41, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,
101,108,102, 46,100, 97,116, 97, 58,115,101,116,116,105,109,101,111,117,116, 40,
 84, 73, 77, 69, 79, 85, 84, 41, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,
114,121, 40,115,101,108,102, 46,100, 97,116, 97, 58, 99,111,110,110,101, 99,116,
 40,115,101,108,102, 46,112, 97,115,118,116, 46,105,112, 44, 32,115,101,108,102,
 46,112, 97,115,118,116, 46,112,111,114,116, 41, 41, 10,101,110,100, 10, 10,102,
117,110, 99,116,105,111,110, 32,109,101,116, 97,116, 46, 95, 95,105,110,100,101,
120, 58,108,111,103,105,110, 40,117,115,101,114, 44, 32,112, 97,115,115,119,111,
114,100, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,
102, 46,116,112, 58, 99,111,109,109, 97,110,100, 40, 34,117,115,101,114, 34, 44,
 32,117,115,101,114, 32,111,114, 32, 85, 83, 69, 82, 41, 41, 10, 32, 32, 32, 32,
108,111, 99, 97,108, 32, 99,111,100,101, 44, 32,114,101,112,108,121, 32, 61, 32,
115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112, 58, 99,104,101,
 99,107,123, 34, 50, 46, 46, 34, 44, 32, 51, 51, 49,125, 41, 10, 32, 32, 32, 32,
105,102, 32, 99,111,100,101, 32, 61, 61, 32, 51, 51, 49, 32,116,104,101,110, 10,
 32, 32, 32, 32, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,
102, 46,116,112, 58, 99,111,109,109, 97,110,100, 40, 34,112, 97,115,115, 34, 44,
 32,112, 97,115,115,119,111,114,100, 32,111,114, 32, 80, 65, 83, 83, 87, 79, 82,
 68, 41, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,115,101,108,102, 46,116,114,121,
 40,115,101,108,102, 46,116,112, 58, 99,104,101, 99,107, 40, 34, 50, 46, 46, 34,
 41, 41, 10, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32,114,101,116,117,114,
110, 32, 49, 10,101,110,100, 10, 10,102,117,110, 99,116,105,111,110, 32,109,101,
116, 97,116, 46, 95, 95,105,110,100,101,120, 58,112, 97,115,118, 40, 41, 10, 32,
 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112, 58,
 99,111,109,109, 97,110,100, 40, 34,112, 97,115,118, 34, 41, 41, 10, 32, 32, 32,
 32,108,111, 99, 97,108, 32, 99,111,100,101, 44, 32,114,101,112,108,121, 32, 61,
 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112, 58, 99,104,
101, 99,107, 40, 34, 50, 46, 46, 34, 41, 41, 10, 32, 32, 32, 32,108,111, 99, 97,
108, 32,112, 97,116,116,101,114,110, 32, 61, 32, 34, 40, 37,100, 43, 41, 37, 68,
 40, 37,100, 43, 41, 37, 68, 40, 37,100, 43, 41, 37, 68, 40, 37,100, 43, 41, 37,
 68, 40, 37,100, 43, 41, 37, 68, 40, 37,100, 43, 41, 34, 10, 32, 32, 32, 32,108,
111, 99, 97,108, 32, 97, 44, 32, 98, 44, 32, 99, 44, 32,100, 44, 32,112, 49, 44,
 32,112, 50, 32, 61, 32,115,111, 99,107,101,116, 46,115,107,105,112, 40, 50, 44,
 32,115,116,114,105,110,103, 46,102,105,110,100, 40,114,101,112,108,121, 44, 32,
112, 97,116,116,101,114,110, 41, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,
114,121, 40, 97, 32, 97,110,100, 32, 98, 32, 97,110,100, 32, 99, 32, 97,110,100,
 32,100, 32, 97,110,100, 32,112, 49, 32, 97,110,100, 32,112, 50, 44, 32,114,101,
112,108,121, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,112, 97,115,118,116, 32,
 61, 32,123, 10, 32, 32, 32, 32, 32, 32, 32, 32,105,112, 32, 61, 32,115,116,114,
105,110,103, 46,102,111,114,109, 97,116, 40, 34, 37,100, 46, 37,100, 46, 37,100,
 46, 37,100, 34, 44, 32, 97, 44, 32, 98, 44, 32, 99, 44, 32,100, 41, 44, 10, 32,
 32, 32, 32, 32, 32, 32, 32,112,111,114,116, 32, 61, 32,112, 49, 42, 50, 53, 54,
 32, 43, 32,112, 50, 10, 32, 32, 32, 32,125, 10, 32, 32, 32, 32,105,102, 32,115,
101,108,102, 46,115,101,114,118,101,114, 32,116,104,101,110, 10, 32, 32, 32, 32,
 32, 32, 32, 32,115,101,108,102, 46,115,101,114,118,101,114, 58, 99,108,111,115,
101, 40, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,115,101,108,102, 46,115,101,114,
118,101,114, 32, 61, 32,110,105,108, 10, 32, 32, 32, 32,101,110,100, 10, 32, 32,
 32, 32,114,101,116,117,114,110, 32,115,101,108,102, 46,112, 97,115,118,116, 46,
105,112, 44, 32,115,101,108,102, 46,112, 97,115,118,116, 46,112,111,114,116, 10,
101,110,100, 10, 10,102,117,110, 99,116,105,111,110, 32,109,101,116, 97,116, 46,
 95, 95,105,110,100,101,120, 58,112,111,114,116, 40,105,112, 44, 32,112,111,114,
116, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,112, 97,115,118,116, 32, 61, 32,
110,105,108, 10, 32, 32, 32, 32,105,102, 32,110,111,116, 32,105,112, 32,116,104,
101,110, 10, 32, 32, 32, 32, 32, 32, 32, 32,105,112, 44, 32,112,111,114,116, 32,
 61, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112, 58,103,
101,116, 99,111,110,116,114,111,108, 40, 41, 58,103,101,116,115,111, 99,107,110,
 97,109,101, 40, 41, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,115,101,108,102, 46,
115,101,114,118,101,114, 32, 61, 32,115,101,108,102, 46,116,114,121, 40,115,111,
 99,107,101,116, 46, 98,105,110,100, 40,105,112, 44, 32, 48, 41, 41, 10, 32, 32,
 32, 32, 32, 32, 32, 32,105,112, 44, 32,112,111,114,116, 32, 61, 32,115,101,108,
102, 46,116,114,121, 40,115,101,108,102, 46,115,101,114,118,101,114, 58,103,101,
116,115,111, 99,107,110, 97,109,101, 40, 41, 41, 10, 32, 32, 32, 32, 32, 32, 32,
 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,115,101,114,118,101,
114, 58,115,101,116,116,105,109,101,111,117,116, 40, 84, 73, 77, 69, 79, 85, 84,
 41, 41, 10, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32,108,111, 99, 97,108,
 32,112,108, 32, 61, 32,109, 97,116,104, 46,109,111,100, 40,112,111,114,116, 44,
 32, 50, 53, 54, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,112,104, 32, 61,
 32, 40,112,111,114,116, 32, 45, 32,112,108, 41, 47, 50, 53, 54, 10, 32, 32, 32,
 32,108,111, 99, 97,108, 32, 97,114,103, 32, 61, 32,115,116,114,105,110,103, 46,
103,115,117, 98, 40,115,116,114,105,110,103, 46,102,111,114,109, 97,116, 40, 34,
 37,115, 44, 37,100, 44, 37,100, 34, 44, 32,105,112, 44, 32,112,104, 44, 32,112,
108, 41, 44, 32, 34, 37, 46, 34, 44, 32, 34, 44, 34, 41, 10, 32, 32, 32, 32,115,
101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112, 58, 99,111,109,109,
 97,110,100, 40, 34,112,111,114,116, 34, 44, 32, 97,114,103, 41, 41, 10, 32, 32,
 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112, 58, 99,
104,101, 99,107, 40, 34, 50, 46, 46, 34, 41, 41, 10, 32, 32, 32, 32,114,101,116,
117,114,110, 32, 49, 10,101,110,100, 10, 10,102,117,110, 99,116,105,111,110, 32,
109,101,116, 97,116, 46, 95, 95,105,110,100,101,120, 58,115,101,110,100, 40,115,
101,110,100,116, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,
101,108,102, 46,112, 97,115,118,116, 32,111,114, 32,115,101,108,102, 46,115,101,
114,118,101,114, 44, 32, 34,110,101,101,100, 32,112,111,114,116, 32,111,114, 32,
112, 97,115,118, 32,102,105,114,115,116, 34, 41, 10, 32, 32, 32, 32, 45, 45, 32,
105,102, 32,116,104,101,114,101, 32,105,115, 32, 97, 32,112, 97,115,118,116, 32,
116, 97, 98,108,101, 44, 32,119,101, 32, 97,108,114,101, 97,100,121, 32,115,101,
110,116, 32, 97, 32, 80, 65, 83, 86, 32, 99,111,109,109, 97,110,100, 10, 32, 32,
 32, 32, 45, 45, 32,119,101, 32,106,117,115,116, 32,103,101,116, 32,116,104,101,
 32,100, 97,116, 97, 32, 99,111,110,110,101, 99,116,105,111,110, 32,105,110,116,
111, 32,115,101,108,102, 46,100, 97,116, 97, 10, 32, 32, 32, 32,105,102, 32,115,
101,108,102, 46,112, 97,115,118,116, 32,116,104,101,110, 32,115,101,108,102, 58,
112, 97,115,118, 99,111,110,110,101, 99,116, 40, 41, 32,101,110,100, 10, 32, 32,
 32, 32, 45, 45, 32,103,101,116, 32,116,104,101, 32,116,114, 97,110,115,102,101,
114, 32, 97,114,103,117,109,101,110,116, 32, 97,110,100, 32, 99,111,109,109, 97,
110,100, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32, 97,114,103,117,109,101,110,
116, 32, 61, 32,115,101,110,100,116, 46, 97,114,103,117,109,101,110,116, 32,111,
114, 10, 32, 32, 32, 32, 32, 32, 32, 32,117,114,108, 46,117,110,101,115, 99, 97,
112,101, 40,115,116,114,105,110,103, 46,103,115,117, 98, 40,115,101,110,100,116,
 46,112, 97,116,104, 32,111,114, 32, 34, 34, 44, 32, 34, 94, 91, 47, 92, 92, 93,
 34, 44, 32, 34, 34, 41, 41, 10, 32, 32, 32, 32,105,102, 32, 97,114,103,117,109,
101,110,116, 32, 61, 61, 32, 34, 34, 32,116,104,101,110, 32, 97,114,103,117,109,
101,110,116, 32, 61, 32,110,105,108, 32,101,110,100, 10, 32, 32, 32, 32,108,111,
 99, 97,108, 32, 99,111,109,109, 97,110,100, 32, 61, 32,115,101,110,100,116, 46,
 99,111,109,109, 97,110,100, 32,111,114, 32, 34,115,116,111,114, 34, 10, 32, 32,
 32, 32, 45, 45, 32,115,101,110,100, 32,116,104,101, 32,116,114, 97,110,115,102,
101,114, 32, 99,111,109,109, 97,110,100, 32, 97,110,100, 32, 99,104,101, 99,107,
 32,116,104,101, 32,114,101,112,108,121, 10, 32, 32, 32, 32,115,101,108,102, 46,
116,114,121, 40,115,101,108,102, 46,116,112, 58, 99,111,109,109, 97,110,100, 40,
 99,111,109,109, 97,110,100, 44, 32, 97,114,103,117,109,101,110,116, 41, 41, 10,
 32, 32, 32, 32,108,111, 99, 97,108, 32, 99,111,100,101, 44, 32,114,101,112,108,
121, 32, 61, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112,
 58, 99,104,101, 99,107,123, 34, 50, 46, 46, 34, 44, 32, 34, 49, 46, 46, 34,125,
 41, 10, 32, 32, 32, 32, 45, 45, 32,105,102, 32,116,104,101,114,101, 32,105,115,
 32,110,111,116, 32, 97, 32, 97, 32,112, 97,115,118,116, 32,116, 97, 98,108,101,
 44, 32,116,104,101,110, 32,116,104,101,114,101, 32,105,115, 32, 97, 32,115,101,
114,118,101,114, 10, 32, 32, 32, 32, 45, 45, 32, 97,110,100, 32,119,101, 32, 97,
108,114,101, 97,100,121, 32,115,101,110,116, 32, 97, 32, 80, 79, 82, 84, 32, 99,
111,109,109, 97,110,100, 10, 32, 32, 32, 32,105,102, 32,110,111,116, 32,115,101,
108,102, 46,112, 97,115,118,116, 32,116,104,101,110, 32,115,101,108,102, 58,112,
111,114,116, 99,111,110,110,101, 99,116, 40, 41, 32,101,110,100, 10, 32, 32, 32,
 32, 45, 45, 32,103,101,116, 32,116,104,101, 32,115,105,110,107, 44, 32,115,111,
117,114, 99,101, 32, 97,110,100, 32,115,116,101,112, 32,102,111,114, 32,116,104,
101, 32,116,114, 97,110,115,102,101,114, 10, 32, 32, 32, 32,108,111, 99, 97,108,
 32,115,116,101,112, 32, 61, 32,115,101,110,100,116, 46,115,116,101,112, 32,111,
114, 32,108,116,110, 49, 50, 46,112,117,109,112, 46,115,116,101,112, 10, 32, 32,
 32, 32,108,111, 99, 97,108, 32,114,101, 97,100,116, 32, 61, 32,123,115,101,108,
102, 46,116,112, 46, 99,125, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32, 99,104,
101, 99,107,115,116,101,112, 32, 61, 32,102,117,110, 99,116,105,111,110, 40,115,
114, 99, 44, 32,115,110,107, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 45, 45, 32,
 99,104,101, 99,107, 32,115,116, 97,116,117,115, 32,105,110, 32, 99,111,110,116,
114,111,108, 32, 99,111,110,110,101, 99,116,105,111,110, 32,119,104,105,108,101,
 32,100,111,119,110,108,111, 97,100,105,110,103, 10, 32, 32, 32, 32, 32, 32, 32,
 32,108,111, 99, 97,108, 32,114,101, 97,100,121,116, 32, 61, 32,115,111, 99,107,
101,116, 46,115,101,108,101, 99,116, 40,114,101, 97,100,116, 44, 32,110,105,108,
 44, 32, 48, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,105,102, 32,114,101, 97,100,
121,116, 91,116,112, 93, 32,116,104,101,110, 32, 99,111,100,101, 32, 61, 32,115,
101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,112, 58, 99,104,101, 99,
107, 40, 34, 50, 46, 46, 34, 41, 41, 32,101,110,100, 10, 32, 32, 32, 32, 32, 32,
 32, 32,114,101,116,117,114,110, 32,115,116,101,112, 40,115,114, 99, 44, 32,115,
110,107, 41, 10, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32, 32,108,111, 99, 97,
108, 32,115,105,110,107, 32, 61, 32,115,111, 99,107,101,116, 46,115,105,110,107,
 40, 34, 99,108,111,115,101, 45,119,104,101,110, 45,100,111,110,101, 34, 44, 32,
115,101,108,102, 46,100, 97,116, 97, 41, 10, 32, 32, 32, 32, 45, 45, 32,116,114,
 97,110,115,102,101,114, 32, 97,108,108, 32,100, 97,116, 97, 32, 97,110,100, 32,
 99,104,101, 99,107, 32,101,114,114,111,114, 10, 32, 32, 32, 32,115,101,108,102,
 46,116,114,121, 40,108,116,110, 49, 50, 46,112,117,109,112, 46, 97,108,108, 40,
115,101,110,100,116, 46,115,111,117,114, 99,101, 44, 32,115,105,110,107, 44, 32,
 99,104,101, 99,107,115,116,101,112, 41, 41, 10, 32, 32, 32, 32,105,102, 32,115,
116,114,105,110,103, 46,102,105,110,100, 40, 99,111,100,101, 44, 32, 34, 49, 46,
 46, 34, 41, 32,116,104,101,110, 32,115,101,108,102, 46,116,114,121, 40,115,101,
108,102, 46,116,112, 58, 99,104,101, 99,107, 40, 34, 50, 46, 46, 34, 41, 41, 32,
101,110,100, 10, 32, 32, 32, 32, 45, 45, 32,100,111,110,101, 32,119,105,116,104,
 32,100, 97,116, 97, 32, 99,111,110,110,101, 99,116,105,111,110, 10, 32, 32, 32,
 32,115,101,108,102, 46,100, 97,116, 97, 58, 99,108,111,115,101, 40, 41, 10, 32,
 32, 32, 32, 45, 45, 32,102,105,110,100, 32,111,117,116, 32,104,111,119, 32,109,
 97,110,121, 32, 98,121,116,101,115, 32,119,101,114,101, 32,115,101,110,116, 10,
 32, 32, 32, 32,108,111, 99, 97,108, 32,115,101,110,116, 32, 61, 32,115,111, 99,
107,101,116, 46,115,107,105,112, 40, 49, 44, 32,115,101,108,102, 46,100, 97,116,
 97, 58,103,101,116,115,116, 97,116,115, 40, 41, 41, 10, 32, 32, 32, 32,115,101,
108,102, 46,100, 97,116, 97, 32, 61, 32,110,105,108, 10, 32, 32, 32, 32,114,101,
116,117,114,110, 32,115,101,110,116, 10,101,110,100, 10, 10,102,117,110, 99,116,
105,111,110, 32,109,101,116, 97,116, 46, 95, 95,105,110,100,101,120, 58,114,101,
 99,101,105,118,101, 40,114,101, 99,118,116, 41, 10, 32, 32, 32, 32,115,101,108,
102, 46,116,114,121, 40,115,101,108,102, 46,112, 97,115,118,116, 32,111,114, 32,
115,101,108,102, 46,115,101,114,118,101,114, 44, 32, 34,110,101,101,100, 32,112,
111,114,116, 32,111,114, 32,112, 97,115,118, 32,102,105,114,115,116, 34, 41, 10,
 32, 32, 32, 32,105,102, 32,115,101,108,102, 46,112, 97,115,118,116, 32,116,104,
101,110, 32,115,101,108,102, 58,112, 97,115,118, 99,111,110,110,101, 99,116, 40,
 41, 32,101,110,100, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32, 97,114,103,117,
109,101,110,116, 32, 61, 32,114,101, 99,118,116, 46, 97,114,103,117,109,101,110,
116, 32,111,114, 10, 32, 32, 32, 32, 32, 32, 32, 32,117,114,108, 46,117,110,101,
115, 99, 97,112,101, 40,115,116,114,105,110,103, 46,103,115,117, 98, 40,114,101,
 99,118,116, 46,112, 97,116,104, 32,111,114, 32, 34, 34, 44, 32, 34, 94, 91, 47,
 92, 92, 93, 34, 44, 32, 34, 34, 41, 41, 10, 32, 32, 32, 32,105,102, 32, 97,114,
103,117,109,101,110,116, 32, 61, 61, 32, 34, 34, 32,116,104,101,110, 32, 97,114,
103,117,109,101,110,116, 32, 61, 32,110,105,108, 32,101,110,100, 10, 32, 32, 32,
 32,108,111, 99, 97,108, 32, 99,111,109,109, 97,110,100, 32, 61, 32,114,101, 99,
118,116, 46, 99,111,109,109, 97,110,100, 32,111,114, 32, 34,114,101,116,114, 34,
 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,
112, 58, 99,111,109,109, 97,110,100, 40, 99,111,109,109, 97,110,100, 44, 32, 97,
114,103,117,109,101,110,116, 41, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,
 99,111,100,101, 32, 61, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102,
 46,116,112, 58, 99,104,101, 99,107,123, 34, 49, 46, 46, 34, 44, 32, 34, 50, 46,
 46, 34,125, 41, 10, 32, 32, 32, 32,105,102, 32,110,111,116, 32,115,101,108,102,
 46,112, 97,115,118,116, 32,116,104,101,110, 32,115,101,108,102, 58,112,111,114,
116, 99,111,110,110,101, 99,116, 40, 41, 32,101,110,100, 10, 32, 32, 32, 32,108,
111, 99, 97,108, 32,115,111,117,114, 99,101, 32, 61, 32,115,111, 99,107,101,116,
 46,115,111,117,114, 99,101, 40, 34,117,110,116,105,108, 45, 99,108,111,115,101,
100, 34, 44, 32,115,101,108,102, 46,100, 97,116, 97, 41, 10, 32, 32, 32, 32,108,
111, 99, 97,108, 32,115,116,101,112, 32, 61, 32,114,101, 99,118,116, 46,115,116,
101,112, 32,111,114, 32,108,116,110, 49, 50, 46,112,117,109,112, 46,115,116,101,
112, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,108,116,110, 49, 50,
 46,112,117,109,112, 46, 97,108,108, 40,115,111,117,114, 99,101, 44, 32,114,101,
 99,118,116, 46,115,105,110,107, 44, 32,115,116,101,112, 41, 41, 10, 32, 32, 32,
 32,105,102, 32,115,116,114,105,110,103, 46,102,105,110,100, 40, 99,111,100,101,
 44, 32, 34, 49, 46, 46, 34, 41, 32,116,104,101,110, 32,115,101,108,102, 46,116,
114,121, 40,115,101,108,102, 46,116,112, 58, 99,104,101, 99,107, 40, 34, 50, 46,
 46, 34, 41, 41, 32,101,110,100, 10, 32, 32, 32, 32,115,101,108,102, 46,100, 97,
116, 97, 58, 99,108,111,115,101, 40, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,
100, 97,116, 97, 32, 61, 32,110,105,108, 10, 32, 32, 32, 32,114,101,116,117,114,
110, 32, 49, 10,101,110,100, 10, 10,102,117,110, 99,116,105,111,110, 32,109,101,
116, 97,116, 46, 95, 95,105,110,100,101,120, 58, 99,119,100, 40,100,105,114, 41,
 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,116,
112, 58, 99,111,109,109, 97,110,100, 40, 34, 99,119,100, 34, 44, 32,100,105,114,
 41, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102,
 46,116,112, 58, 99,104,101, 99,107, 40, 50, 53, 48, 41, 41, 10, 32, 32, 32, 32,
114,101,116,117,114,110, 32, 49, 10,101,110,100, 10, 10,102,117,110, 99,116,105,
111,110, 32,109,101,116, 97,116, 46, 95, 95,105,110,100,101,120, 58,116,121,112,
101, 40,116,121,112,101, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121,
 40,115,101,108,102, 46,116,112, 58, 99,111,109,109, 97,110,100, 40, 34,116,121,
112,101, 34, 44, 32,116,121,112,101, 41, 41, 10, 32, 32, 32, 32,115,101,108,102,
 46,116,114,121, 40,115,101,108,102, 46,116,112, 58, 99,104,101, 99,107, 40, 50,
 48, 48, 41, 41, 10, 32, 32, 32, 32,114,101,116,117,114,110, 32, 49, 10,101,110,
100, 10, 10,102,117,110, 99,116,105,111,110, 32,109,101,116, 97,116, 46, 95, 95,
105,110,100,101,120, 58,103,114,101,101,116, 40, 41, 10, 32, 32, 32, 32,108,111,
 99, 97,108, 32, 99,111,100,101, 32, 61, 32,115,101,108,102, 46,116,114,121, 40,
115,101,108,102, 46,116,112, 58, 99,104,101, 99,107,123, 34, 49, 46, 46, 34, 44,
 32, 34, 50, 46, 46, 34,125, 41, 10, 32, 32, 32, 32,105,102, 32,115,116,114,105,
110,103, 46,102,105,110,100, 40, 99,111,100,101, 44, 32, 34, 49, 46, 46, 34, 41,
 32,116,104,101,110, 32,115,101,108,102, 46,116,114,121, 40,115,101,108,102, 46,
116,112, 58, 99,104,101, 99,107, 40, 34, 50, 46, 46, 34, 41, 41, 32,101,110,100,
 10, 32, 32, 32, 32,114,101,116,117,114,110, 32, 49, 10,101,110,100, 10, 10,102,
117,110, 99,116,105,111,110, 32,109,101,116, 97,116, 46, 95, 95,105,110,100,101,
120, 58,113,117,105,116, 40, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,
121, 40,115,101,108,102, 46,116,112, 58, 99,111,109,109, 97,110,100, 40, 34,113,
117,105,116, 34, 41, 41, 10, 32, 32, 32, 32,115,101,108,102, 46,116,114,121, 40,
115,101,108,102, 46,116,112, 58, 99,104,101, 99,107, 40, 34, 50, 46, 46, 34, 41,
 41, 10, 32, 32, 32, 32,114,101,116,117,114,110, 32, 49, 10,101,110,100, 10, 10,
102,117,110, 99,116,105,111,110, 32,109,101,116, 97,116, 46, 95, 95,105,110,100,
101,120, 58, 99,108,111,115,101, 40, 41, 10, 32, 32, 32, 32,105,102, 32,115,101,
108,102, 46,100, 97,116, 97, 32,116,104,101,110, 32,115,101,108,102, 46,100, 97,
116, 97, 58, 99,108,111,115,101, 40, 41, 32,101,110,100, 10, 32, 32, 32, 32,105,
102, 32,115,101,108,102, 46,115,101,114,118,101,114, 32,116,104,101,110, 32,115,
101,108,102, 46,115,101,114,118,101,114, 58, 99,108,111,115,101, 40, 41, 32,101,
110,100, 10, 32, 32, 32, 32,114,101,116,117,114,110, 32,115,101,108,102, 46,116,
112, 58, 99,108,111,115,101, 40, 41, 10,101,110,100, 10, 10, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 45, 45, 32, 72,105,103,104,
 32,108,101,118,101,108, 32, 70, 84, 80, 32, 65, 80, 73, 10, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10,108,111, 99, 97,108, 32,102,
117,110, 99,116,105,111,110, 32,111,118,101,114,114,105,100,101, 40,116, 41, 10,
 32, 32, 32, 32,105,102, 32,116, 46,117,114,108, 32,116,104,101,110, 10, 32, 32,
 32, 32, 32, 32, 32, 32,108,111, 99, 97,108, 32,117, 32, 61, 32,117,114,108, 46,
112, 97,114,115,101, 40,116, 46,117,114,108, 41, 10, 32, 32, 32, 32, 32, 32, 32,
 32,102,111,114, 32,105, 44,118, 32,105,110, 32, 98, 97,115,101, 46,112, 97,105,
114,115, 40,116, 41, 32,100,111, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32,117, 91,105, 93, 32, 61, 32,118, 10, 32, 32, 32, 32, 32, 32, 32, 32,101,110,
100, 10, 32, 32, 32, 32, 32, 32, 32, 32,114,101,116,117,114,110, 32,117, 10, 32,
 32, 32, 32,101,108,115,101, 32,114,101,116,117,114,110, 32,116, 32,101,110,100,
 10,101,110,100, 10, 10,108,111, 99, 97,108, 32,102,117,110, 99,116,105,111,110,
 32,116,112,117,116, 40,112,117,116,116, 41, 10, 32, 32, 32, 32,112,117,116,116,
 32, 61, 32,111,118,101,114,114,105,100,101, 40,112,117,116,116, 41, 10, 32, 32,
 32, 32,115,111, 99,107,101,116, 46,116,114,121, 40,112,117,116,116, 46,104,111,
115,116, 44, 32, 34,109,105,115,115,105,110,103, 32,104,111,115,116,110, 97,109,
101, 34, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,102, 32, 61, 32,111,112,
101,110, 40,112,117,116,116, 46,104,111,115,116, 44, 32,112,117,116,116, 46,112,
111,114,116, 44, 32,112,117,116,116, 46, 99,114,101, 97,116,101, 41, 10, 32, 32,
 32, 32,102, 58,103,114,101,101,116, 40, 41, 10, 32, 32, 32, 32,102, 58,108,111,
103,105,110, 40,112,117,116,116, 46,117,115,101,114, 44, 32,112,117,116,116, 46,
112, 97,115,115,119,111,114,100, 41, 10, 32, 32, 32, 32,105,102, 32,112,117,116,
116, 46,116,121,112,101, 32,116,104,101,110, 32,102, 58,116,121,112,101, 40,112,
117,116,116, 46,116,121,112,101, 41, 32,101,110,100, 10, 32, 32, 32, 32,102, 58,
112, 97,115,118, 40, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,115,101,110,
116, 32, 61, 32,102, 58,115,101,110,100, 40,112,117,116,116, 41, 10, 32, 32, 32,
 32,102, 58,113,117,105,116, 40, 41, 10, 32, 32, 32, 32,102, 58, 99,108,111,115,
101, 40, 41, 10, 32, 32, 32, 32,114,101,116,117,114,110, 32,115,101,110,116, 10,
101,110,100, 10, 10,108,111, 99, 97,108, 32,100,101,102, 97,117,108,116, 32, 61,
 32,123, 10,  9,112, 97,116,104, 32, 61, 32, 34, 47, 34, 44, 10,  9,115, 99,104,
101,109,101, 32, 61, 32, 34,102,116,112, 34, 10,125, 10, 10,108,111, 99, 97,108,
 32,102,117,110, 99,116,105,111,110, 32,112, 97,114,115,101, 40,117, 41, 10, 32,
 32, 32, 32,108,111, 99, 97,108, 32,116, 32, 61, 32,115,111, 99,107,101,116, 46,
116,114,121, 40,117,114,108, 46,112, 97,114,115,101, 40,117, 44, 32,100,101,102,
 97,117,108,116, 41, 41, 10, 32, 32, 32, 32,115,111, 99,107,101,116, 46,116,114,
121, 40,116, 46,115, 99,104,101,109,101, 32, 61, 61, 32, 34,102,116,112, 34, 44,
 32, 34,119,114,111,110,103, 32,115, 99,104,101,109,101, 32, 39, 34, 32, 46, 46,
 32,116, 46,115, 99,104,101,109,101, 32, 46, 46, 32, 34, 39, 34, 41, 10, 32, 32,
 32, 32,115,111, 99,107,101,116, 46,116,114,121, 40,116, 46,104,111,115,116, 44,
 32, 34,109,105,115,115,105,110,103, 32,104,111,115,116,110, 97,109,101, 34, 41,
 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,112, 97,116, 32, 61, 32, 34, 94,116,
121,112,101, 61, 40, 46, 41, 36, 34, 10, 32, 32, 32, 32,105,102, 32,116, 46,112,
 97,114, 97,109,115, 32,116,104,101,110, 10, 32, 32, 32, 32, 32, 32, 32, 32,116,
 46,116,121,112,101, 32, 61, 32,115,111, 99,107,101,116, 46,115,107,105,112, 40,
 50, 44, 32,115,116,114,105,110,103, 46,102,105,110,100, 40,116, 46,112, 97,114,
 97,109,115, 44, 32,112, 97,116, 41, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32,115,
111, 99,107,101,116, 46,116,114,121, 40,116, 46,116,121,112,101, 32, 61, 61, 32,
 34, 97, 34, 32,111,114, 32,116, 46,116,121,112,101, 32, 61, 61, 32, 34,105, 34,
 44, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 34,105,110,118, 97,108,
105,100, 32,116,121,112,101, 32, 39, 34, 32, 46, 46, 32,116, 46,116,121,112,101,
 32, 46, 46, 32, 34, 39, 34, 41, 10, 32, 32, 32, 32,101,110,100, 10, 32, 32, 32,
 32,114,101,116,117,114,110, 32,116, 10,101,110,100, 10, 10,108,111, 99, 97,108,
 32,102,117,110, 99,116,105,111,110, 32,115,112,117,116, 40,117, 44, 32, 98,111,
100,121, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,112,117,116,116, 32, 61,
 32,112, 97,114,115,101, 40,117, 41, 10, 32, 32, 32, 32,112,117,116,116, 46,115,
111,117,114, 99,101, 32, 61, 32,108,116,110, 49, 50, 46,115,111,117,114, 99,101,
 46,115,116,114,105,110,103, 40, 98,111,100,121, 41, 10, 32, 32, 32, 32,114,101,
116,117,114,110, 32,116,112,117,116, 40,112,117,116,116, 41, 10,101,110,100, 10,
 10,112,117,116, 32, 61, 32,115,111, 99,107,101,116, 46,112,114,111,116,101, 99,
116, 40,102,117,110, 99,116,105,111,110, 40,112,117,116,116, 44, 32, 98,111,100,
121, 41, 10, 32, 32, 32, 32,105,102, 32, 98, 97,115,101, 46,116,121,112,101, 40,
112,117,116,116, 41, 32, 61, 61, 32, 34,115,116,114,105,110,103, 34, 32,116,104,
101,110, 32,114,101,116,117,114,110, 32,115,112,117,116, 40,112,117,116,116, 44,
 32, 98,111,100,121, 41, 10, 32, 32, 32, 32,101,108,115,101, 32,114,101,116,117,
114,110, 32,116,112,117,116, 40,112,117,116,116, 41, 32,101,110,100, 10,101,110,
100, 41, 10, 10,108,111, 99, 97,108, 32,102,117,110, 99,116,105,111,110, 32,116,
103,101,116, 40,103,101,116,116, 41, 10, 32, 32, 32, 32,103,101,116,116, 32, 61,
 32,111,118,101,114,114,105,100,101, 40,103,101,116,116, 41, 10, 32, 32, 32, 32,
115,111, 99,107,101,116, 46,116,114,121, 40,103,101,116,116, 46,104,111,115,116,
 44, 32, 34,109,105,115,115,105,110,103, 32,104,111,115,116,110, 97,109,101, 34,
 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,102, 32, 61, 32,111,112,101,110,
 40,103,101,116,116, 46,104,111,115,116, 44, 32,103,101,116,116, 46,112,111,114,
116, 44, 32,103,101,116,116, 46, 99,114,101, 97,116,101, 41, 10, 32, 32, 32, 32,
102, 58,103,114,101,101,116, 40, 41, 10, 32, 32, 32, 32,102, 58,108,111,103,105,
110, 40,103,101,116,116, 46,117,115,101,114, 44, 32,103,101,116,116, 46,112, 97,
115,115,119,111,114,100, 41, 10, 32, 32, 32, 32,105,102, 32,103,101,116,116, 46,
116,121,112,101, 32,116,104,101,110, 32,102, 58,116,121,112,101, 40,103,101,116,
116, 46,116,121,112,101, 41, 32,101,110,100, 10, 32, 32, 32, 32,102, 58,112, 97,
115,118, 40, 41, 10, 32, 32, 32, 32,102, 58,114,101, 99,101,105,118,101, 40,103,
101,116,116, 41, 10, 32, 32, 32, 32,102, 58,113,117,105,116, 40, 41, 10, 32, 32,
 32, 32,114,101,116,117,114,110, 32,102, 58, 99,108,111,115,101, 40, 41, 10,101,
110,100, 10, 10,108,111, 99, 97,108, 32,102,117,110, 99,116,105,111,110, 32,115,
103,101,116, 40,117, 41, 10, 32, 32, 32, 32,108,111, 99, 97,108, 32,103,101,116,
116, 32, 61, 32,112, 97,114,115,101, 40,117, 41, 10, 32, 32, 32, 32,108,111, 99,
 97,108, 32,116, 32, 61, 32,123,125, 10, 32, 32, 32, 32,103,101,116,116, 46,115,
105,110,107, 32, 61, 32,108,116,110, 49, 50, 46,115,105,110,107, 46,116, 97, 98,
108,101, 40,116, 41, 10, 32, 32, 32, 32,116,103,101,116, 40,103,101,116,116, 41,
 10, 32, 32, 32, 32,114,101,116,117,114,110, 32,116, 97, 98,108,101, 46, 99,111,
110, 99, 97,116, 40,116, 41, 10,101,110,100, 10, 10, 99,111,109,109, 97,110,100,
 32, 61, 32,115,111, 99,107,101,116, 46,112,114,111,116,101, 99,116, 40,102,117,
110, 99,116,105,111,110, 40, 99,109,100,116, 41, 10, 32, 32, 32, 32, 99,109,100,
116, 32, 61, 32,111,118,101,114,114,105,100,101, 40, 99,109,100,116, 41, 10, 32,
 32, 32, 32,115,111, 99,107,101,116, 46,116,114,121, 40, 99,109,100,116, 46,104,
111,115,116, 44, 32, 34,109,105,115,115,105,110,103, 32,104,111,115,116,110, 97,
109,101, 34, 41, 10, 32, 32, 32, 32,115,111, 99,107,101,116, 46,116,114,121, 40,
 99,109,100,116, 46, 99,111,109,109, 97,110,100, 44, 32, 34,109,105,115,115,105,
110,103, 32, 99,111,109,109, 97,110,100, 34, 41, 10, 32, 32, 32, 32,108,111, 99,
 97,108, 32,102, 32, 61, 32,111,112,101,110, 40, 99,109,100,116, 46,104,111,115,
116, 44, 32, 99,109,100,116, 46,112,111,114,116, 44, 32, 99,109,100,116, 46, 99,
114,101, 97,116,101, 41, 10, 32, 32, 32, 32,102, 58,103,114,101,101,116, 40, 41,
 10, 32, 32, 32, 32,102, 58,108,111,103,105,110, 40, 99,109,100,116, 46,117,115,
101,114, 44, 32, 99,109,100,116, 46,112, 97,115,115,119,111,114,100, 41, 10, 32,
 32, 32, 32,102, 46,116,114,121, 40,102, 46,116,112, 58, 99,111,109,109, 97,110,
100, 40, 99,109,100,116, 46, 99,111,109,109, 97,110,100, 44, 32, 99,109,100,116,
 46, 97,114,103,117,109,101,110,116, 41, 41, 10, 32, 32, 32, 32,105,102, 32, 99,
109,100,116, 46, 99,104,101, 99,107, 32,116,104,101,110, 32,102, 46,116,114,121,
 40,102, 46,116,112, 58, 99,104,101, 99,107, 40, 99,109,100,116, 46, 99,104,101,
 99,107, 41, 41, 32,101,110,100, 10, 32, 32, 32, 32,102, 58,113,117,105,116, 40,
 41, 10, 32, 32, 32, 32,114,101,116,117,114,110, 32,102, 58, 99,108,111,115,101,
 40, 41, 10,101,110,100, 41, 10, 10,103,101,116, 32, 61, 32,115,111, 99,107,101,
116, 46,112,114,111,116,101, 99,116, 40,102,117,110, 99,116,105,111,110, 40,103,
101,116,116, 41, 10, 32, 32, 32, 32,105,102, 32, 98, 97,115,101, 46,116,121,112,
101, 40,103,101,116,116, 41, 32, 61, 61, 32, 34,115,116,114,105,110,103, 34, 32,
116,104,101,110, 32,114,101,116,117,114,110, 32,115,103,101,116, 40,103,101,116,
116, 41, 10, 32, 32, 32, 32,101,108,115,101, 32,114,101,116,117,114,110, 32,116,
103,101,116, 40,103,101,116,116, 41, 32,101,110,100, 10,101,110,100, 41, 10, 10,

 0 };
  return luaL_dostring(L, (const char*)B); 
} /* end of embedded lua code */

