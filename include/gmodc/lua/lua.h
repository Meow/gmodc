#ifndef GMODC_LUA_LUA_H
#define GMODC_LUA_LUA_H

/*
  Minimal lua_State layout: enough padding to reach the `luabase` pointer
  that Garry's Mod stores in every state. Only lua_get_base reads it; module
  code should treat lua_State as opaque. Skipped when the C++ Interface.h has
  already defined the struct.
*/

#ifndef GARRYSMOD_LUA_INTERFACE_H

typedef struct {
#if defined(__x86_64__) || defined(_M_X64)
  unsigned char _ignore_this_common_lua_header_[92 + 22];
#elif defined(__i386) || defined(_M_IX86)
  unsigned char _ignore_this_common_lua_header_[48 + 22];
#else
#error Unknown architecture detected
#endif

  void *luabase;
} lua_State;

#endif

#endif
