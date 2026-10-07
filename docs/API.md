# gmodc API reference

This is the C API exposed by [`include/GarrysMod/Lua/CCompat.h`](../include/GarrysMod/Lua/CCompat.h) and the macros in [`include/gmodc/lua/interface.h`](../include/gmodc/lua/interface.h). Every function is a one-to-one wrapper over a method of `GarrysMod::Lua::ILuaBase`; the C++ name is listed next to each one so you can cross-reference the [Facepunch headers](https://github.com/Facepunch/gmod-module-base).

All examples are ANSI C (C89) and assume `#include <gmodc/lua/interface.h>`.

## Contents

- [Conventions](#conventions)
- [Module entry points](#module-entry-points)
- [State and stack](#state-and-stack)
- [Tables](#tables)
- [Metatables](#metatables)
- [Calling and errors](#calling-and-errors)
- [Reading values](#reading-values)
- [Pushing values](#pushing-values)
- [Types](#types)
- [User types](#user-types)
- [Raw userdata](#raw-userdata)
- [References](#references)
- [Comparison](#comparison)
- [Complete example: a Counter class](#complete-example-a-counter-class)

## Conventions

**The `LUA` handle.** Every function takes a `luabase_t *` as its first argument. Inside `GMOD_MODULE_OPEN()`, `GMOD_MODULE_CLOSE()` and `LUA_FUNCTION()` it is available as `LUA`. If you receive a raw `lua_State *` from somewhere else, convert it with `lua_get_base`.

**Stack positions.** Lua passes values on a stack. Positive indices count from the bottom, so inside a `LUA_FUNCTION` position 1 is the first argument. Negative indices count from the top: -1 is the last value pushed, -2 the one below it. Diagrams in this document show the stack bottom to top, left to right.

**Return values.** A `LUA_FUNCTION` pushes its results and returns how many it pushed. Returning 0 means the function returns nothing.

**Errors do not return.** `lua_throw_error`, `lua_check_type`, `lua_arg_error`, `lua_check_string`, `lua_check_number` and a failing `lua_call` unwind with `longjmp`. Code after them does not run, so do argument checks before allocating, or use `lua_pcall` and check the result yourself.

**Booleans** are `int`: 0 is false, anything else is true.

## Module entry points

### `GMOD_MODULE_OPEN()`

Defines the exported `gmod13_open` function that Garry's Mod calls when Lua runs `require("name")`. Use it to register functions and types. Return the number of values left on the stack; they become the return values of `require`. Normally return 0.

### `GMOD_MODULE_CLOSE()`

Defines the exported `gmod13_close` function, called when the Lua state shuts down. Free references created with `lua_reference_create` and any C resources here. Return 0.

### `LUA_FUNCTION(name)`

Defines a function with the `cfunc_t` signature that Lua can call. The macro fetches the `luabase_t`, binds the current state and gives you `LUA`.

```c
#include <gmodc/lua/interface.h>

LUA_FUNCTION(add) {
  double a = lua_check_number(LUA, 1);
  double b = lua_check_number(LUA, 2);
  lua_push_number(LUA, a + b);
  return 1;
}

GMOD_MODULE_OPEN() {
  lua_push_special(LUA, LUA_SPECIAL_GLOB);
  lua_push_cfunc(LUA, add);
  lua_set_field(LUA, -2, "add");
  lua_pop(LUA, 1);
  return 0;
}

GMOD_MODULE_CLOSE() {
  return 0;
}
```

```lua
require("example")
print(add(2, 3)) -- 5
```

### `LUA_SPECIAL_GLOB`, `LUA_SPECIAL_ENV`, `LUA_SPECIAL_REG`

Arguments for `lua_push_special`: the global table `_G`, the environment of the running function, and the registry (a table only C code can reach, useful for private storage).

## State and stack

### `luabase_t *lua_get_base(lua_State *L)`

Returns the `luabase_t` stored in a `lua_State`. The macros call it for you.

### `void lua_set_state(luabase_t *L, lua_State *state)` — `SetState`

Binds the implementation to a `lua_State`. `LUA_FUNCTION` does this on every call; you only need it when writing a `cfunc_t` by hand.

### `int lua_top(luabase_t *L)` — `Top`

Returns the number of values on the stack. In a `LUA_FUNCTION` this is the argument count.

```c
LUA_FUNCTION(count_args) {
  lua_push_number(LUA, lua_top(LUA));
  return 1;
}
```

### `void lua_push(luabase_t *L, int stack_pos)` — `Push`

Pushes a copy of the value at `stack_pos`.

```c
/* stack: a, b  ->  a, b, a */
lua_push(LUA, 1);
```

### `void lua_pop(luabase_t *L, int amt)` — `Pop`

Pops `amt` values. Unlike the C++ method there is no default, so pass 1 explicitly.

### `void lua_insert(luabase_t *L, int stack_pos)` — `Insert`

Moves the top value into `stack_pos`, shifting the values above it up.

```c
/* stack: a, b, c  ->  c, a, b */
lua_insert(LUA, 1);
```

### `void lua_remove(luabase_t *L, int stack_pos)` — `Remove`

Removes the value at `stack_pos`, shifting the values above it down.

```c
/* stack: a, b, c  ->  a, c */
lua_remove(LUA, 2);
```

## Tables

### `void lua_create_table(luabase_t *L)` — `CreateTable`

Pushes a new empty table.

### `void lua_get_table(luabase_t *L, int stack_pos)` — `GetTable`

Pops the key from the top of the stack and pushes `table[key]`, where the table is at `stack_pos`. Invokes `__index`.

```c
/* equivalent of  local v = t[5]  with t at position 1 */
lua_push_number(LUA, 5);
lua_get_table(LUA, 1);
/* stack: t, t[5] */
```

### `void lua_get_field(luabase_t *L, int stack_pos, const char *name)` — `GetField`

Pushes `table[name]`. The common way to walk into globals:

```c
lua_push_special(LUA, LUA_SPECIAL_GLOB);  /* _G                   */
lua_get_field(LUA, -1, "math");           /* _G, math             */
lua_get_field(LUA, -1, "pi");             /* _G, math, math.pi    */
printf("%f\n", lua_get_number(LUA, -1));
lua_pop(LUA, 3);
```

### `void lua_set_field(luabase_t *L, int stack_pos, const char *name)` — `SetField`

Pops the top value and stores it as `table[name]`. Invokes `__newindex`.

```c
/* MyModule = { version = "1.0" } */
lua_push_special(LUA, LUA_SPECIAL_GLOB);
lua_create_table(LUA);
lua_push_string(LUA, "1.0", 0);
lua_set_field(LUA, -2, "version");      /* pops "1.0" into the new table */
lua_set_field(LUA, -2, "MyModule");     /* pops the table into _G        */
lua_pop(LUA, 1);                        /* _G                            */
```

### `void lua_set_table(luabase_t *L, int stack_pos)` — `SetTable`

Pops a value (top) and a key (second from top) and stores `table[key] = value`. Because two values sit above the table, a relative table index is usually -3.

```c
/* t = { [1] = "first", [2] = "second" } */
lua_create_table(LUA);
lua_push_number(LUA, 1);
lua_push_string(LUA, "first", 0);
lua_set_table(LUA, -3);
lua_push_number(LUA, 2);
lua_push_string(LUA, "second", 0);
lua_set_table(LUA, -3);
```

### `void lua_raw_get(luabase_t *L, int stack_pos)` — `RawGet`
### `void lua_raw_set(luabase_t *L, int stack_pos)` — `RawSet`

Same stack behaviour as `lua_get_table` and `lua_set_table`, but metamethods are skipped. Use these when you need to read or write a table that has an `__index` or `__newindex` handler without triggering it.

### `int lua_next(luabase_t *L, int stack_pos)` — `Next`

Iterates the table at `stack_pos` like `pairs()`. Push `nil` to start. Each call pops the previous key and pushes the next key and value, returning non-zero. When there are no more entries nothing is pushed and 0 is returned.

```c
/* sum every numeric value in the table passed as argument 1 */
LUA_FUNCTION(sum) {
  double total = 0;
  lua_check_type(LUA, 1, TYPE_TABLE);
  lua_push_nil(LUA);
  while (lua_next(LUA, 1)) {
    /* stack: t, key, value */
    if (lua_is_type(LUA, -1, TYPE_NUMBER))
      total += lua_get_number(LUA, -1);
    lua_pop(LUA, 1);    /* drop value, keep key for the next call */
  }
  lua_push_number(LUA, total);
  return 1;
}
```

Do not call `lua_get_string` on a key while iterating: it converts a numeric key to a string in place, which breaks `lua_next`. Copy the key with `lua_push(LUA, -2)` first if you need it as text.

### `int lua_obj_len(luabase_t *L, int stack_pos)` — `ObjLen`

Returns the length (`#`) of a string, table or userdata.

```c
int n = lua_obj_len(LUA, 1);   /* #arg1 */
```

## Metatables

### `void lua_set_meta_table(luabase_t *L, int stack_pos)` — `SetMetaTable`

Pops the table on top and makes it the metatable of the value at `stack_pos`.

### `int lua_get_meta_table(luabase_t *L, int stack_pos)` — `GetMetaTable`

Pushes the metatable of the value at `stack_pos` and returns non-zero. If there is none, nothing is pushed and 0 is returned.

```c
if (lua_get_meta_table(LUA, 1)) {
  lua_get_field(LUA, -1, "__name");
  /* ... */
  lua_pop(LUA, 2);
}
```

### `int lua_create_meta_table(luabase_t *L, const char *name)` — `CreateMetaTable`

Pushes the metatable registered under `name`, creating it on first use, and returns the type ID that identifies it. Keep the ID in a static variable: it is what `lua_push_user_type`, `lua_get_user_type` and `lua_is_type` expect.

```c
static int counter_type;

counter_type = lua_create_meta_table(LUA, "Counter");   /* pushes the table */
lua_push(LUA, -1);
lua_set_field(LUA, -2, "__index");       /* mt.__index = mt, so methods resolve */
lua_push_cfunc(LUA, counter_increment);
lua_set_field(LUA, -2, "Increment");
lua_push_cfunc(LUA, counter_gc);
lua_set_field(LUA, -2, "__gc");
lua_pop(LUA, 1);
```

Calling it again with the same name pushes the same table and returns the same ID, which makes it safe to use from both a server and a client module.

### `int lua_push_meta_table(luabase_t *L, int type)` — `PushMetaTable`

Pushes the metatable registered for a type ID. Returns 0 and pushes nothing if the type is unknown. Works for built-in types too, for example `TYPE_ENTITY` to add a method to all entities:

```c
if (lua_push_meta_table(LUA, TYPE_ENTITY)) {
  lua_push_cfunc(LUA, entity_hello);
  lua_set_field(LUA, -2, "Hello");
  lua_pop(LUA, 1);
}
```

### `void lua_create_meta_table_type(luabase_t *L, const char *name, int type)` — `CreateMetaTableType`

Deprecated upstream. Registers a metatable for a type ID you pick yourself. Prefer `lua_create_meta_table`.

## Calling and errors

### `void lua_call(luabase_t *L, int args, int results)` — `Call`

Calls a function. Push the function, then each argument, then call with the argument count. The function and arguments are popped and `results` values are pushed. If the callee errors, the error propagates to the Lua caller and `lua_call` does not return.

```c
/* print("hello from C") */
lua_push_special(LUA, LUA_SPECIAL_GLOB);
lua_get_field(LUA, -1, "print");
lua_push_string(LUA, "hello from C", 0);
lua_call(LUA, 1, 0);
lua_pop(LUA, 1);
```

```c
/* local s = string.rep("ab", 3) */
lua_push_special(LUA, LUA_SPECIAL_GLOB);
lua_get_field(LUA, -1, "string");
lua_get_field(LUA, -1, "rep");
lua_push_string(LUA, "ab", 0);
lua_push_number(LUA, 3);
lua_call(LUA, 2, 1);
/* stack: _G, string, "ababab" */
printf("%s\n", lua_get_string(LUA, -1, NULL));
lua_pop(LUA, 3);
```

### `int lua_pcall(luabase_t *L, int args, int results, int error_func)` — `PCall`

Protected call. Same stack protocol as `lua_call`. Returns 0 on success. On error the error message is pushed and a non-zero Lua error code is returned. `error_func` is the stack index of a handler that receives the error object (for example `debug.traceback`), or 0 for none.

```c
LUA_FUNCTION(run_hook) {
  lua_push_special(LUA, LUA_SPECIAL_GLOB);
  lua_get_field(LUA, -1, "hook");
  lua_get_field(LUA, -1, "Run");
  lua_push_string(LUA, "MyModuleTick", 0);

  if (lua_pcall(LUA, 1, 0, 0) != 0) {
    printf("hook.Run failed: %s\n", lua_get_string(LUA, -1, NULL));
    lua_pop(LUA, 1);   /* the error message */
  }

  lua_pop(LUA, 2);     /* hook, _G */
  return 0;
}
```

### `void lua_throw_error(luabase_t *L, const char *message)` — `ThrowError`

Raises a Lua error with `message`. Does not return.

```c
if (fp == NULL)
  lua_throw_error(LUA, "could not open file");
```

### `void lua_check_type(luabase_t *L, int stack_pos, int type)` — `CheckType`

Raises an error unless the value at `stack_pos` has type `type`. Does not return on failure.

```c
lua_check_type(LUA, 1, TYPE_TABLE);
lua_check_type(LUA, 2, TYPE_FUNCTION);
```

### `void lua_arg_error(luabase_t *L, int arg_num, const char *message)` — `ArgError`

Raises `bad argument #arg_num to 'function' (message)`. Does not return.

```c
counter_t *c = lua_get_user_type(LUA, 1, counter_type);
if (c == NULL)
  lua_arg_error(LUA, 1, "Counter expected");
```

## Reading values

### `const char *lua_get_string(luabase_t *L, int stack_pos, unsigned int *out_len)` — `GetString`

Returns the string at `stack_pos`, or `NULL` if it is neither a string nor a number. Numbers are converted to strings in place. If `out_len` is not `NULL` it receives the byte length, which is the only way to handle strings containing NUL. The pointer is valid while the value stays on the stack; copy it if you keep it longer.

```c
unsigned int len;
const char *s = lua_get_string(LUA, 1, &len);
if (s != NULL)
  fwrite(s, 1, len, stdout);
```

### `double lua_get_number(luabase_t *L, int stack_pos)` — `GetNumber`

Returns the number at `stack_pos`, or 0 if it is not a number.

### `int lua_get_bool(luabase_t *L, int stack_pos)` — `GetBool`

Returns 1 or 0. Non-booleans yield 0.

### `cfunc_t lua_get_cfunc(luabase_t *L, int stack_pos)` — `GetCFunction`

Returns the C function at `stack_pos`, or `NULL` if the value is not a C function (Lua functions also return `NULL`).

### `const char *lua_check_string(luabase_t *L, int stack_pos)` — `CheckString`
### `double lua_check_number(luabase_t *L, int stack_pos)` — `CheckNumber`

Like the `get` variants but raise a `bad argument` error when the type does not match. They do not return on error.

```c
LUA_FUNCTION(greet) {
  const char *name = lua_check_string(LUA, 1);
  double times = lua_check_number(LUA, 2);
  /* ... */
  return 0;
}
```

### `const QAngle *lua_get_angle(luabase_t *L, int stack_pos)` — `GetAngle`
### `const Vector *lua_get_vector(luabase_t *L, int stack_pos)` — `GetVector`

Return a pointer to the Angle or Vector stored in the Lua value. Both are `struct { float x, y, z; }` in C. Check the type first; the pointer is only meaningful for `TYPE_ANGLE` / `TYPE_VECTOR` values, and only while the value is on the stack.

```c
#include <math.h>

LUA_FUNCTION(vector_length) {
  const Vector *v;
  lua_check_type(LUA, 1, TYPE_VECTOR);
  v = lua_get_vector(LUA, 1);
  lua_push_number(LUA, sqrt(v->x * v->x + v->y * v->y + v->z * v->z));
  return 1;
}
```

## Pushing values

### `void lua_push_nil(luabase_t *L)` — `PushNil`

Pushes `nil`.

### `void lua_push_string(luabase_t *L, const char *val, unsigned int len)` — `PushString`

Pushes a copy of the string. `len` 0 means "measure with `strlen`", so pass the length for binary data.

```c
lua_push_string(LUA, "plain text", 0);
lua_push_string(LUA, buffer, (unsigned int)bytes_read);
```

### `void lua_push_number(luabase_t *L, double val)` — `PushNumber`
### `void lua_push_bool(luabase_t *L, int val)` — `PushBool`

Push a number or a boolean (`val != 0`).

### `void lua_push_cfunc(luabase_t *L, cfunc_t val)` — `PushCFunction`

Pushes a function defined with `LUA_FUNCTION` so Lua can call it.

```c
lua_push_special(LUA, LUA_SPECIAL_GLOB);
lua_push_cfunc(LUA, my_function);
lua_set_field(LUA, -2, "my_function");
lua_pop(LUA, 1);
```

### `void lua_push_cclosure(luabase_t *L, cfunc_t val, int vars)` — `PushCClosure`

Pops `vars` values and pushes a C function that captures them as upvalues. Inside the function, upvalue *i* (1-based) is at the Lua 5.1 pseudo-index `-10002 - i`, which the stack-position arguments of `lua_push` and the `lua_get_*` functions accept.

```c
#define UPVALUE(i) (-10002 - (i))

LUA_FUNCTION(say) {
  printf("%s\n", lua_get_string(LUA, UPVALUE(1), NULL));
  return 0;
}

GMOD_MODULE_OPEN() {
  lua_push_special(LUA, LUA_SPECIAL_GLOB);
  lua_push_string(LUA, "hello", 0);
  lua_push_cclosure(LUA, say, 1);     /* pops "hello", pushes the closure */
  lua_set_field(LUA, -2, "say_hello");
  lua_pop(LUA, 1);
  return 0;
}
```

### `void lua_push_special(luabase_t *L, int type)` — `PushSpecial`

Pushes `_G` (`LUA_SPECIAL_GLOB`), the current environment (`LUA_SPECIAL_ENV`) or the registry (`LUA_SPECIAL_REG`).

### `void lua_push_angle(luabase_t *L, const QAngle *val)` — `PushAngle`
### `void lua_push_vector(luabase_t *L, const Vector *val)` — `PushVector`

Push a copy of an Angle or Vector.

```c
LUA_FUNCTION(make_vector) {
  Vector v;
  v.x = (float)lua_check_number(LUA, 1);
  v.y = (float)lua_check_number(LUA, 2);
  v.z = (float)lua_check_number(LUA, 3);
  lua_push_vector(LUA, &v);
  return 1;
}
```

## Types

The `TYPE_*` constants live in [`include/gmodc/lua/types.h`](../include/gmodc/lua/types.h) and match `GarrysMod::Lua::Type` numerically.

| Lua types | Garry's Mod types |
|-----------|-------------------|
| `TYPE_NIL`, `TYPE_BOOL`, `TYPE_LIGHTUSERDATA`, `TYPE_NUMBER`, `TYPE_STRING`, `TYPE_TABLE`, `TYPE_FUNCTION`, `TYPE_USERDATA`, `TYPE_THREAD` | `TYPE_ENTITY`, `TYPE_VECTOR`, `TYPE_ANGLE`, `TYPE_PHYSOBJ`, `TYPE_SAVE`, `TYPE_RESTORE`, `TYPE_DAMAGEINFO`, `TYPE_EFFECTDATA`, `TYPE_MOVEDATA`, `TYPE_RECIPIENTFILTER`, `TYPE_USERCMD`, `TYPE_SCRIPTEDVEHICLE`, `TYPE_MATERIAL`, `TYPE_PANEL`, `TYPE_PARTICLE`, `TYPE_PARTICLEEMITTER`, `TYPE_TEXTURE`, `TYPE_USERMSG`, `TYPE_CONVAR`, `TYPE_IMESH`, `TYPE_MATRIX`, `TYPE_SOUND`, `TYPE_PIXELVISHANDLE`, `TYPE_DLIGHT`, `TYPE_VIDEO`, `TYPE_FILE`, `TYPE_LOCOMOTION`, `TYPE_PATH`, `TYPE_NAVAREA`, `TYPE_SOUNDHANDLE`, `TYPE_NAVLADDER`, `TYPE_PARTICLESYSTEM`, `TYPE_PROJECTEDTEXTURE`, `TYPE_PHYSCOLLIDE`, `TYPE_SURFACEINFO` |

`NONE` (-1) is returned for empty stack slots and `TYPE_COUNT` is one past the last built-in ID.

### `int lua_is_type(luabase_t *L, int stack_pos, int type)` — `IsType`

Non-zero if the value at `stack_pos` has the given type. Accepts custom IDs from `lua_create_meta_table` too.

```c
if (lua_is_type(LUA, 1, TYPE_NUMBER))
  lua_push_number(LUA, lua_get_number(LUA, 1) * 2);
else
  lua_push_nil(LUA);
return 1;
```

### `int lua_get_type(luabase_t *L, int stack_pos)` — `GetType`

Returns the type ID of the value at `stack_pos`.

```c
switch (lua_get_type(LUA, 1)) {
case TYPE_STRING: /* ... */ break;
case TYPE_NUMBER: /* ... */ break;
default:
  lua_arg_error(LUA, 1, "string or number expected");
}
```

### `const char *lua_get_type_name(luabase_t *L, int type)` — `GetTypeName`

Returns the name of a built-in type ID (`"number"`, `"entity"`, ...). It does not know about types registered with `lua_create_meta_table`.

```c
lua_arg_error(LUA, 1, lua_get_type_name(LUA, TYPE_VECTOR));  /* "bad argument #1 ... (vector)" */
```

## User types

A user type is a Lua userdata that carries a type ID and a pointer, letting you hand C objects to Lua with a metatable full of methods. Register the type once in `GMOD_MODULE_OPEN` with `lua_create_meta_table`, then use the functions below.

### `void lua_push_user_type(luabase_t *L, void *data, int type)` — `PushUserType`

Pushes a new userdata of `type` that references `data`. The module keeps ownership of the memory. If a metatable is registered for `type` it is attached automatically. Free the memory from a `__gc` metamethod and invalidate the userdata with `lua_set_user_type` so later accesses return `NULL` instead of a dangling pointer.

```c
LUA_FUNCTION(counter_new) {
  counter_t *c = malloc(sizeof *c);
  c->value = lua_is_type(LUA, 1, TYPE_NUMBER) ? lua_get_number(LUA, 1) : 0;
  lua_push_user_type(LUA, c, counter_type);
  return 1;
}

LUA_FUNCTION(counter_gc) {
  counter_t *c = lua_get_user_type(LUA, 1, counter_type);
  if (c != NULL) {
    free(c);
    lua_set_user_type(LUA, 1, NULL);
  }
  return 0;
}
```

### `void lua_set_user_type(luabase_t *L, int stack_pos, void *data)` — `SetUserType`

Replaces the data pointer of the userdata at `stack_pos`. `NULL` invalidates it.

### `void *lua_get_user_type(luabase_t *L, int stack_pos, int type)` — `GetUserType<void>`

Returns the data pointer if the value at `stack_pos` is a userdata of `type` that has not been invalidated. Returns `NULL` otherwise, so one check covers "wrong type", "not a userdata" and "already freed".

```c
LUA_FUNCTION(counter_increment) {
  counter_t *c = lua_get_user_type(LUA, 1, counter_type);
  if (c == NULL)
    lua_arg_error(LUA, 1, "Counter expected");
  c->value += 1;
  lua_push_number(LUA, c->value);
  return 1;
}
```

### `void lua_push_user_type_value(luabase_t *L, const void *val, unsigned int size, int type)`

C counterpart of `PushUserType_Value`. Pushes a new userdata of `type` and copies `size` bytes from `val` into it, so Lua owns the memory and releases it with the value. No `__gc` is needed for plain structs. The copy is only guaranteed 8-byte alignment, and anything inside it that points at heap memory still needs cleanup in `__gc`.

```c
typedef struct { float x, y; } point_t;
static int point_type;

LUA_FUNCTION(point_new) {
  point_t p;
  p.x = (float)lua_check_number(LUA, 1);
  p.y = (float)lua_check_number(LUA, 2);
  lua_push_user_type_value(LUA, &p, sizeof p, point_type);
  return 1;
}

LUA_FUNCTION(point_x) {
  point_t *p = lua_get_user_type(LUA, 1, point_type);
  if (p == NULL)
    lua_arg_error(LUA, 1, "Point expected");
  lua_push_number(LUA, p->x);
  return 1;
}
```

## Raw userdata

These wrap methods that Facepunch marks deprecated. They remain available for code that needs the raw block.

### `void *lua_new_userdata(luabase_t *L, unsigned int size)` — `NewUserdata`

Allocates and pushes a userdata block of `size` bytes and returns its address. Garry's Mod expects the block to begin with a `userdata_t` header; `lua_push_user_type_value` shows how to lay it out.

### `void *lua_get_userdata(luabase_t *L, int stack_pos)` — `GetUserdata`

Returns the raw block at `stack_pos` or `NULL`. For Garry's Mod values it is a `userdata_t`:

```c
userdata_t *ud = lua_get_userdata(LUA, 1);
if (ud != NULL)
  printf("type id %d\n", ud->type);
```

### `void lua_push_userdata(luabase_t *L, void *val)` — `PushUserdata`

Pushes a light userdata holding the pointer. Garry's Mod discourages light userdata; prefer `lua_push_user_type`.

## References

References keep a Lua value alive across calls so C code can hold on to it, typically a callback.

### `int lua_reference_create(luabase_t *L)` — `ReferenceCreate`

Pops the top value and returns an integer reference to it.

### `void lua_reference_push(luabase_t *L, int ref)` — `ReferencePush`

Pushes the referenced value.

### `void lua_reference_free(luabase_t *L, int ref)` — `ReferenceFree`

Releases the reference so the value can be garbage collected.

```c
static int callback_ref = -1;

LUA_FUNCTION(set_callback) {
  lua_check_type(LUA, 1, TYPE_FUNCTION);
  if (callback_ref != -1)
    lua_reference_free(LUA, callback_ref);
  lua_push(LUA, 1);
  callback_ref = lua_reference_create(LUA);   /* pops the copy */
  return 0;
}

LUA_FUNCTION(fire_callback) {
  if (callback_ref == -1)
    return 0;
  lua_reference_push(LUA, callback_ref);
  lua_push_number(LUA, 42);
  lua_call(LUA, 1, 0);
  return 0;
}

GMOD_MODULE_CLOSE() {
  if (callback_ref != -1)
    lua_reference_free(LUA, callback_ref);
  return 0;
}
```

```lua
set_callback(function(n) print("got", n) end)
fire_callback() -- got 42
```

## Comparison

### `int lua_equal(luabase_t *L, int a, int b)` — `Equal`

Non-zero if the values at `a` and `b` are equal, honouring `__eq`.

### `int lua_raw_equal(luabase_t *L, int a, int b)` — `RawEqual`

Same without metamethods.

```c
lua_push_special(LUA, LUA_SPECIAL_GLOB);
lua_get_field(LUA, -1, "LocalPlayer");
lua_call(LUA, 0, 1);
if (lua_raw_equal(LUA, 1, -1))
  puts("argument 1 is the local player");
lua_pop(LUA, 2);
```

## Complete example: a Counter class

Everything together: a C struct exposed to Lua as `Counter`, with a constructor, a method, garbage collection and a `__tostring`.

```c
#include <stdio.h>
#include <stdlib.h>
#include <gmodc/lua/interface.h>

typedef struct {
  double value;
} counter_t;

static int counter_type;

static counter_t *check_counter(luabase_t *LUA, int pos) {
  counter_t *c = lua_get_user_type(LUA, pos, counter_type);
  if (c == NULL)
    lua_arg_error(LUA, pos, "Counter expected");
  return c;
}

LUA_FUNCTION(counter_new) {
  counter_t *c = malloc(sizeof *c);
  c->value = lua_is_type(LUA, 1, TYPE_NUMBER) ? lua_get_number(LUA, 1) : 0;
  lua_push_user_type(LUA, c, counter_type);
  return 1;
}

LUA_FUNCTION(counter_increment) {
  counter_t *c = check_counter(LUA, 1);
  c->value += lua_is_type(LUA, 2, TYPE_NUMBER) ? lua_get_number(LUA, 2) : 1;
  lua_push_number(LUA, c->value);
  return 1;
}

LUA_FUNCTION(counter_tostring) {
  char buf[64];
  counter_t *c = check_counter(LUA, 1);
  sprintf(buf, "Counter(%g)", c->value);
  lua_push_string(LUA, buf, 0);
  return 1;
}

LUA_FUNCTION(counter_gc) {
  counter_t *c = lua_get_user_type(LUA, 1, counter_type);
  if (c != NULL) {
    free(c);
    lua_set_user_type(LUA, 1, NULL);
  }
  return 0;
}

GMOD_MODULE_OPEN() {
  counter_type = lua_create_meta_table(LUA, "Counter");
  lua_push(LUA, -1);
  lua_set_field(LUA, -2, "__index");
  lua_push_cfunc(LUA, counter_increment);
  lua_set_field(LUA, -2, "Increment");
  lua_push_cfunc(LUA, counter_tostring);
  lua_set_field(LUA, -2, "__tostring");
  lua_push_cfunc(LUA, counter_gc);
  lua_set_field(LUA, -2, "__gc");
  lua_pop(LUA, 1);

  lua_push_special(LUA, LUA_SPECIAL_GLOB);
  lua_push_cfunc(LUA, counter_new);
  lua_set_field(LUA, -2, "Counter");
  lua_pop(LUA, 1);
  return 0;
}

GMOD_MODULE_CLOSE() {
  return 0;
}
```

```lua
require("example")

local c = Counter(10)
print(c:Increment())    -- 11
print(c:Increment(5))   -- 16
print(c)                -- Counter(16)
```
