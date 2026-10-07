#ifndef GARRYSMOD_LUA_CCOMPAT_H
#define GARRYSMOD_LUA_CCOMPAT_H

#include <gmodc/lua/types.h>
#include <gmodc/lua/lua.h>
#include <gmodc/lua/source_compat.h>

#ifdef __cplusplus
#include "LuaBase.h"
#include "SourceCompat.h"
extern "C" {
#endif

/*
  C API over GarrysMod::Lua::ILuaBase.

  Every function below is a thin wrapper around the matching ILuaBase method
  (CCompat.cpp). The first argument is always the luabase_t handle that
  GMOD_MODULE_OPEN(), GMOD_MODULE_CLOSE() and LUA_FUNCTION() expose as `LUA`.

  Stack positions follow the Lua convention: positive indices count from the
  bottom (1 is the first argument of the running function), negative indices
  count from the top (-1 is the most recently pushed value).

  Functions marked "does not return on error" raise a Lua error with longjmp.
  Nothing after the call runs, so release any resource you own before calling
  them, or defer the check until nothing needs cleaning up.

  A worked reference with longer examples lives in docs/API.md.
*/

/* Opaque handle to the ILuaBase instance that drives a lua_State. */
typedef void luabase_t;

/*
  Memory layout of a Garry's Mod userdata block. `data` points at the module's
  own object (or NULL once invalidated), `type` is the type ID returned by
  lua_create_meta_table. Prefer the lua_*_user_type functions over touching
  this directly.
*/
typedef struct {
  void *data;
  unsigned char type;
} userdata_t;

/*
  Signature of a function callable from Lua. Define them with LUA_FUNCTION()
  (gmodc/lua/interface.h) rather than by hand; the macro fetches the
  luabase_t for you. The return value is the number of results left on the
  stack.
*/
typedef int (*cfunc_t)(lua_State *L);

/* ------------------------------------------------------------------------ */
/* State and stack                                                          */
/* ------------------------------------------------------------------------ */

/*
  Returns the luabase_t stored in a lua_State. The module macros call this for
  you; you only need it when Garry's Mod hands you a raw lua_State.
*/
luabase_t *lua_get_base(lua_State *L);

/*
  Binds the ILuaBase implementation to `state`. LUA_FUNCTION() does this on
  every call, so plain modules never need it.
*/
void lua_set_state(luabase_t *L, lua_State *state);

/*
  Returns the number of values on the stack. Inside a LUA_FUNCTION this is the
  number of arguments the caller passed.

    if (lua_top(LUA) < 2)
      lua_throw_error(LUA, "expected two arguments");
*/
int lua_top(luabase_t *L);

/* Pushes a copy of the value at stack_pos onto the top of the stack. */
void lua_push(luabase_t *L, int stack_pos);

/* Pops amt values from the top of the stack. */
void lua_pop(luabase_t *L, int amt);

/*
  Moves the value at the top of the stack into stack_pos, shifting everything
  above that position up by one.
*/
void lua_insert(luabase_t *L, int stack_pos);

/*
  Removes the value at stack_pos, shifting everything above it down by one.
*/
void lua_remove(luabase_t *L, int stack_pos);

/* ------------------------------------------------------------------------ */
/* Tables                                                                   */
/* ------------------------------------------------------------------------ */

/* Creates an empty table and pushes it. */
void lua_create_table(luabase_t *L);

/*
  Pushes table[key], where table is the value at stack_pos and key is the
  value at the top of the stack. The key is popped first, so the result ends
  up where the key was. Invokes __index metamethods.

    lua_push_special(LUA, LUA_SPECIAL_GLOB);
    lua_push_string(LUA, "print", 0);
    lua_get_table(LUA, -2);   -- stack: _G, _G.print
*/
void lua_get_table(luabase_t *L, int stack_pos);

/*
  Pushes table[name], where table is the value at stack_pos. Shorthand for
  pushing a string key followed by lua_get_table.

    lua_push_special(LUA, LUA_SPECIAL_GLOB);
    lua_get_field(LUA, -1, "hook");   -- stack: _G, hook
    lua_get_field(LUA, -1, "Run");    -- stack: _G, hook, hook.Run
*/
void lua_get_field(luabase_t *L, int stack_pos, const char *name);

/*
  Sets table[name] to the value at the top of the stack and pops that value.
  table is the value at stack_pos. Invokes __newindex metamethods.

    lua_push_special(LUA, LUA_SPECIAL_GLOB);
    lua_push_cfunc(LUA, my_function);
    lua_set_field(LUA, -2, "my_function");   -- _G.my_function = my_function
    lua_pop(LUA, 1);
*/
void lua_set_field(luabase_t *L, int stack_pos, const char *name);

/*
  Sets table[key] = value where table is at stack_pos, key is the second value
  from the top and value is the top value. Pops both key and value. When using
  a relative index for the table, remember that it sits below the two pushed
  values (hence -3 in the example).

    lua_create_table(LUA);
    lua_push_number(LUA, 1);
    lua_push_string(LUA, "first", 0);
    lua_set_table(LUA, -3);   -- t[1] = "first"
*/
void lua_set_table(luabase_t *L, int stack_pos);

/* Like lua_get_table but bypasses __index metamethods. */
void lua_raw_get(luabase_t *L, int stack_pos);

/* Like lua_set_table but bypasses __newindex metamethods. */
void lua_raw_set(luabase_t *L, int stack_pos);

/*
  Iterates a table like pairs(). Push nil first; each call pops the previous
  key and pushes the next key and value, returning non-zero. When the table
  is exhausted it pushes nothing and returns 0.

    lua_push_nil(LUA);
    while (lua_next(LUA, 1)) {
      -- key at -2, value at -1
      lua_pop(LUA, 1);   -- drop the value, keep the key for the next step
    }

  Do not call lua_get_string on a numeric key during traversal; it converts
  the key in place and confuses lua_next.
*/
int lua_next(luabase_t *L, int stack_pos);

/*
  Returns the length of the value at stack_pos (the # operator) for strings,
  tables and userdata.
*/
int lua_obj_len(luabase_t *L, int stack_pos);

/* ------------------------------------------------------------------------ */
/* Metatables                                                               */
/* ------------------------------------------------------------------------ */

/*
  Pops the table at the top of the stack and sets it as the metatable of the
  value at stack_pos.
*/
void lua_set_meta_table(luabase_t *L, int stack_pos);

/*
  Pushes the metatable of the value at stack_pos and returns non-zero. If the
  value has no metatable nothing is pushed and 0 is returned.
*/
int lua_get_meta_table(luabase_t *L, int stack_pos);

/*
  Pushes the metatable registered under `name`, creating it on first use, and
  returns the type ID associated with it. Store the ID; it is what the
  lua_*_user_type functions and lua_is_type expect.

    static int counter_type;

    counter_type = lua_create_meta_table(LUA, "Counter");
    lua_push(LUA, -1);
    lua_set_field(LUA, -2, "__index");   -- methods are looked up on itself
    lua_push_cfunc(LUA, counter_increment);
    lua_set_field(LUA, -2, "Increment");
    lua_pop(LUA, 1);
*/
int lua_create_meta_table(luabase_t *L, const char *name);

/*
  Pushes the metatable registered for the given type ID and returns non-zero.
  Returns 0 and pushes nothing if no such type exists.
*/
int lua_push_meta_table(luabase_t *L, int type);

/*
  Deprecated: registers a metatable named `name` for a type ID you choose.
  Use lua_create_meta_table, which allocates the ID for you.
*/
void lua_create_meta_table_type(luabase_t *L, const char *name, int type);

/* ------------------------------------------------------------------------ */
/* Calling and errors                                                       */
/* ------------------------------------------------------------------------ */

/*
  Calls a function. Push the function, then its arguments in order, then call
  with the argument count. The function and arguments are popped and
  `results` return values are pushed. Errors propagate to the Lua caller and
  do not return here.

    lua_push_special(LUA, LUA_SPECIAL_GLOB);
    lua_get_field(LUA, -1, "print");
    lua_push_string(LUA, "hello from C", 0);
    lua_call(LUA, 1, 0);
    lua_pop(LUA, 1);   -- _G
*/
void lua_call(luabase_t *L, int args, int results);

/*
  Like lua_call but catches errors. Returns 0 on success. On failure the error
  message is pushed and the Lua error code (LUA_ERRRUN etc.) is returned.
  error_func is the stack index of a handler that receives the error object,
  or 0 for none.

    if (lua_pcall(LUA, 1, 0, 0) != 0) {
      printf("call failed: %s\n", lua_get_string(LUA, -1, NULL));
      lua_pop(LUA, 1);
    }
*/
int lua_pcall(luabase_t *L, int args, int results, int error_func);

/*
  Raises a Lua error with the given message. Does not return.
*/
void lua_throw_error(luabase_t *L, const char *message);

/*
  Raises a Lua error unless the value at stack_pos has the given TYPE_*.
  Does not return on error.

    lua_check_type(LUA, 1, TYPE_TABLE);
*/
void lua_check_type(luabase_t *L, int stack_pos, int type);

/*
  Raises a formatted "bad argument #arg_num to 'name' (message)" error.
  Does not return.

    if (!lua_get_user_type(LUA, 1, counter_type))
      lua_arg_error(LUA, 1, "Counter expected");
*/
void lua_arg_error(luabase_t *L, int arg_num, const char *message);

/* ------------------------------------------------------------------------ */
/* Reading values                                                           */
/* ------------------------------------------------------------------------ */

/*
  Returns the string at stack_pos, or NULL if the value is not a string or a
  number. Numbers are converted to strings in place. If out_len is not NULL
  it receives the length in bytes, which matters for strings that contain
  NUL. The pointer stays valid while the value remains on the stack.

    unsigned int len;
    const char *s = lua_get_string(LUA, 1, &len);
*/
const char *lua_get_string(luabase_t *L, int stack_pos, unsigned int *out_len);

/* Returns the number at stack_pos, or 0 if the value is not a number. */
double lua_get_number(luabase_t *L, int stack_pos);

/* Returns 1 or 0 for the boolean at stack_pos; 0 if it is not a boolean. */
int lua_get_bool(luabase_t *L, int stack_pos);

/*
  Returns the C function at stack_pos, or NULL if the value is not a C
  function.
*/
cfunc_t lua_get_cfunc(luabase_t *L, int stack_pos);

/*
  Like lua_get_string but raises a Lua error (does not return) if the value
  is not a string or number.

    const char *name = lua_check_string(LUA, 1);
*/
const char *lua_check_string(luabase_t *L, int stack_pos);

/*
  Like lua_get_number but raises a Lua error (does not return) if the value
  is not a number.
*/
double lua_check_number(luabase_t *L, int stack_pos);

/*
  Returns a pointer to the Angle at stack_pos. The pointer refers to the Lua
  value itself, so copy it out if you need it after the value is popped.
  Check the type first with lua_check_type(LUA, pos, TYPE_ANGLE).
*/
const QAngle *lua_get_angle(luabase_t *L, int stack_pos);

/*
  Returns a pointer to the Vector at stack_pos. Same lifetime rules as
  lua_get_angle; check with TYPE_VECTOR first.

    const Vector *v = lua_get_vector(LUA, 1);
    double len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
*/
const Vector *lua_get_vector(luabase_t *L, int stack_pos);

/* ------------------------------------------------------------------------ */
/* Pushing values                                                           */
/* ------------------------------------------------------------------------ */

/* Pushes nil. */
void lua_push_nil(luabase_t *L);

/*
  Pushes a copy of the string. If len is 0 the string is measured with
  strlen, so pass the length explicitly for data that contains NUL bytes.

    lua_push_string(LUA, "hello", 0);
    lua_push_string(LUA, buffer, buffer_len);
*/
void lua_push_string(luabase_t *L, const char *val, unsigned int len);

/* Pushes a number. */
void lua_push_number(luabase_t *L, double val);

/* Pushes a boolean; any non-zero val becomes true. */
void lua_push_bool(luabase_t *L, int val);

/*
  Pushes a C function so Lua can call it.

    lua_push_special(LUA, LUA_SPECIAL_GLOB);
    lua_push_cfunc(LUA, my_function);
    lua_set_field(LUA, -2, "my_function");
*/
void lua_push_cfunc(luabase_t *L, cfunc_t val);

/*
  Pops `vars` values from the stack and pushes a C function that captures
  them as upvalues. Inside the function, upvalue i (1-based) is at the Lua
  5.1 pseudo-index -10002 - i, which lua_push and the lua_get_* functions
  accept as a stack position.

    lua_push_string(LUA, "greeting", 0);
    lua_push_cclosure(LUA, print_upvalue, 1);
*/
void lua_push_cclosure(luabase_t *L, cfunc_t val, int vars);

/*
  Pushes the global table (LUA_SPECIAL_GLOB), the current environment table
  (LUA_SPECIAL_ENV) or the registry (LUA_SPECIAL_REG). The enum is declared
  in gmodc/lua/interface.h.
*/
void lua_push_special(luabase_t *L, int type);

/*
  Pushes an Angle by value.

    QAngle a;
    a.x = 0; a.y = 90; a.z = 0;
    lua_push_angle(LUA, &a);
*/
void lua_push_angle(luabase_t *L, const QAngle *val);

/* Pushes a Vector by value. */
void lua_push_vector(luabase_t *L, const Vector *val);

/* ------------------------------------------------------------------------ */
/* Types                                                                    */
/* ------------------------------------------------------------------------ */

/*
  Returns non-zero if the value at stack_pos has the given type. Accepts the
  TYPE_* constants from gmodc/lua/types.h and the IDs returned by
  lua_create_meta_table.

    if (lua_is_type(LUA, 1, TYPE_NUMBER)) ...
*/
int lua_is_type(luabase_t *L, int stack_pos, int type);

/*
  Returns the type ID of the value at stack_pos (TYPE_NIL, TYPE_STRING, a
  custom ID...). Returns NONE for a position with no value.
*/
int lua_get_type(luabase_t *L, int stack_pos);

/*
  Returns the name of a built-in type ID ("number", "entity", ...). Does not
  know about types created with lua_create_meta_table.
*/
const char *lua_get_type_name(luabase_t *L, int type);

/* ------------------------------------------------------------------------ */
/* User types                                                               */
/* ------------------------------------------------------------------------ */

/*
  Pushes a new userdata of the given type that points at `data`. The module
  keeps ownership of `data`; free it from a __gc metamethod and invalidate
  the userdata with lua_set_user_type(LUA, 1, NULL). If a metatable has been
  registered for `type` it is attached to the new value.

    counter_t *c = malloc(sizeof *c);
    c->value = 0;
    lua_push_user_type(LUA, c, counter_type);
*/
void lua_push_user_type(luabase_t *L, void *data, int type);

/*
  Replaces the data pointer of the userdata at stack_pos. Passing NULL marks
  the value as invalid, after which lua_get_user_type returns NULL for it.
*/
void lua_set_user_type(luabase_t *L, int stack_pos, void *data);

/*
  Returns the data pointer of the userdata at stack_pos if it is a userdata
  of the given type and has not been invalidated, otherwise NULL.

    counter_t *c = lua_get_user_type(LUA, 1, counter_type);
    if (c == NULL)
      lua_arg_error(LUA, 1, "Counter expected");
*/
void *lua_get_user_type(luabase_t *L, int stack_pos, int type);

/*
  C counterpart of ILuaBase::PushUserType_Value: pushes a new userdata of the
  given type and copies `size` bytes from `val` into it, so Lua owns the
  memory and frees it with the value. The copy is only guaranteed an
  alignment of 8. Suitable for plain structs; anything holding pointers to
  heap memory still needs a __gc metamethod.

    point_t p;
    p.x = 1; p.y = 2;
    lua_push_user_type_value(LUA, &p, sizeof p, point_type);
*/
void lua_push_user_type_value(luabase_t *L, const void *val, unsigned int size,
                              int type);

/* ------------------------------------------------------------------------ */
/* Raw userdata (deprecated upstream)                                       */
/* ------------------------------------------------------------------------ */

/*
  Allocates a userdata block of `size` bytes, pushes it and returns its
  address. Used internally by lua_push_user_type_value; prefer the user type
  functions.
*/
void *lua_new_userdata(luabase_t *L, unsigned int size);

/*
  Returns the raw userdata block at stack_pos, or NULL if the value is not a
  userdata. For Garry's Mod values this is a userdata_t.
*/
void *lua_get_userdata(luabase_t *L, int stack_pos);

/*
  Pushes a light userdata wrapping the pointer. Garry's Mod discourages light
  userdata; use lua_push_user_type instead.
*/
void lua_push_userdata(luabase_t *L, void *val);

/* ------------------------------------------------------------------------ */
/* References                                                               */
/* ------------------------------------------------------------------------ */

/*
  Pops the value at the top of the stack and returns a reference that keeps
  it alive beyond the current call, for example to store a Lua callback in a
  C global. Free it with lua_reference_free when done.

    static int callback_ref = -1;

    lua_check_type(LUA, 1, TYPE_FUNCTION);
    lua_push(LUA, 1);
    callback_ref = lua_reference_create(LUA);
*/
int lua_reference_create(luabase_t *L);

/* Releases a reference so the value can be collected. */
void lua_reference_free(luabase_t *L, int ref);

/*
  Pushes the value a reference points to.

    lua_reference_push(LUA, callback_ref);
    lua_push_number(LUA, 42);
    lua_call(LUA, 1, 0);
*/
void lua_reference_push(luabase_t *L, int ref);

/* ------------------------------------------------------------------------ */
/* Comparison                                                               */
/* ------------------------------------------------------------------------ */

/*
  Returns non-zero if the values at positions a and b are equal, invoking
  __eq metamethods.
*/
int lua_equal(luabase_t *L, int a, int b);

/* Like lua_equal but without metamethods (primitive equality). */
int lua_raw_equal(luabase_t *L, int a, int b);

#ifdef __cplusplus
}
#endif

#endif
