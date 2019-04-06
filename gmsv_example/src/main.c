#include <stdio.h>
#include <stdlib.h>
#include <gmodc/lua/interface.h>

/*
  This is the same example as the one present in the C++
  Gmod-Module-Base repository, except that this one is
  written in ANSI C.
*/

LUA_FUNCTION(my_function) {
  char str_out[512];
  double number;

  if (lua_is_type(LUA, 1, TYPE_NUMBER)) {
    number = lua_get_number(LUA, 1);
    sprintf_s(str_out, sizeof(str_out), "Thanks for the number, I like %f!", number);
    lua_push_string(LUA, str_out, 0);

    return 1;
  }

  lua_push_string(LUA, "You didn't give me a number!", 0);

  return 1;
}

GMOD_MODULE_OPEN() {
  lua_push_special(LUA, LUA_SPECIAL_GLOB);
  lua_push_string(LUA, "my_function", 0);
  lua_push_cfunc(LUA, my_function);
  lua_set_table(LUA, -3);

  return 0;
}

GMOD_MODULE_CLOSE() {
  return 0;
}
