# Gmod Module C Wrapper

An ANSI C wrapper for the Garry's Mod binary module interface. Designed for compatibility, speed and code maintainability as primary goals.

Garry's Mod only ships a C++ interface (`GarrysMod::Lua::ILuaBase`) for binary modules. This project wraps every method of that interface in a plain C function and reproduces the `GMOD_MODULE_OPEN` / `LUA_FUNCTION` macros, so a module can be written entirely in C89 with no C++ in your own sources. The only C++ in the build is the small shim `CCompat.cpp`, compiled once alongside your code.

## Repository layout

| Path | Purpose |
|------|---------|
| [include/gmodc/lua/interface.h](include/gmodc/lua/interface.h) | The header your module includes. Defines `GMOD_MODULE_OPEN()`, `GMOD_MODULE_CLOSE()`, `LUA_FUNCTION()` and `LUA_SPECIAL_*`. |
| [include/GarrysMod/Lua/CCompat.h](include/GarrysMod/Lua/CCompat.h) | The C API: one `lua_*` function per `ILuaBase` method, each documented in place. |
| [include/GarrysMod/Lua/CCompat.cpp](include/GarrysMod/Lua/CCompat.cpp) | The C++ shim that forwards each `lua_*` call to `ILuaBase`. Add it to your build. |
| [include/gmodc/lua/types.h](include/gmodc/lua/types.h) | `TYPE_*` constants (`TYPE_NUMBER`, `TYPE_ENTITY`, ...). |
| [include/gmodc/lua/source_compat.h](include/gmodc/lua/source_compat.h) | C `Vector` and `QAngle` structs. |
| [include/GarrysMod/Lua/](include/GarrysMod/Lua/) | Facepunch's upstream C++ headers, used by the shim. |
| [gmsv_example/](gmsv_example/) | A complete example module with a premake5 script. |
| [docs/API.md](docs/API.md) | Full API reference with examples for every function. |

## Quick start

1. Copy the `include/` directory into your project, or add it as an include path.
2. Compile `include/GarrysMod/Lua/CCompat.cpp` as C++11 together with your C sources into a shared library.
3. Name the output `gmsv_<name>_<platform>.dll` (server) or `gmcl_<name>_<platform>.dll` (client) and place it in `garrysmod/lua/bin/`.
4. Load it from Lua with `require("<name>")`.

A minimal module, the same one found in [gmsv_example/src/main.c](gmsv_example/src/main.c):

```c
#include <stdio.h>
#include <gmodc/lua/interface.h>

LUA_FUNCTION(my_function) {
  char str_out[64];

  if (lua_is_type(LUA, 1, TYPE_NUMBER)) {
    sprintf(str_out, "Thanks for the number, I like %f!", lua_get_number(LUA, 1));
    lua_push_string(LUA, str_out, 0);
    return 1;
  }

  lua_push_string(LUA, "You didn't give me a number!", 0);
  return 1;
}

GMOD_MODULE_OPEN() {
  lua_push_special(LUA, LUA_SPECIAL_GLOB);   /* _G                           */
  lua_push_string(LUA, "my_function", 0);    /* _G, "my_function"            */
  lua_push_cfunc(LUA, my_function);          /* _G, "my_function", function  */
  lua_set_table(LUA, -3);                    /* _G.my_function = function    */
  return 0;
}

GMOD_MODULE_CLOSE() {
  return 0;
}
```

```lua
require("example")
print(my_function(42))   -- Thanks for the number, I like 42.000000!
print(my_function("no")) -- You didn't give me a number!
```

## Writing a module

**Entry points.** `GMOD_MODULE_OPEN()` runs on `require` and is where you register globals and types. `GMOD_MODULE_CLOSE()` runs when the Lua state shuts down. Both give you the `LUA` handle and return an `int` (normally 0).

**Functions.** `LUA_FUNCTION(name)` defines a function Lua can call. Arguments are on the Lua stack at positions 1, 2, 3...; push your results and return how many you pushed.

**Stack positions.** Positive indices count from the bottom (1 is the first argument), negative ones from the top (-1 is the last pushed value). Most mistakes come from forgetting that pushing a value shifts every negative index by one, which is why `lua_set_table` in the example above uses -3.

**Errors.** `lua_throw_error`, `lua_check_type`, `lua_arg_error`, `lua_check_string` and `lua_check_number` raise a Lua error and never return to your C code. Validate arguments before allocating memory, or free it before raising.

**Custom types.** Register a metatable with `lua_create_meta_table`, keep the returned type ID, and hand C objects to Lua with `lua_push_user_type`. Free them in a `__gc` metamethod. A full `Counter` class is walked through at the end of [docs/API.md](docs/API.md#complete-example-a-counter-class).

**Calling Lua.** Push the function, push the arguments, then `lua_call` or `lua_pcall`:

```c
lua_push_special(LUA, LUA_SPECIAL_GLOB);
lua_get_field(LUA, -1, "print");
lua_push_string(LUA, "hello from C", 0);
lua_call(LUA, 1, 0);
lua_pop(LUA, 1);
```

Every function is documented with an example in [docs/API.md](docs/API.md), and the same notes appear as comments above each declaration in [CCompat.h](include/GarrysMod/Lua/CCompat.h).

## Building

A premake5.lua file is included in the `/gmsv_example` folder. Please check it out and edit to suit your own project.

Configurations for both x86 and x86_64 are included. Generate the project files with [premake5](https://premake.github.io/) and build the configuration that matches the Garry's Mod branch you are targeting:

```sh
cd gmsv_example
premake5 gmake                 # called gmake2 in older premake releases; vs2022 on Windows
make -C project config=x86     # 32-bit
make -C project config=x86_64  # 64-bit (x86-64 branch)
```

The binaries are written to `gmsv_example/bin`, already named the way Garry's Mod [expects them](https://wiki.facepunch.com/gmod/Creating_Binary_Modules:_Premake), ready to be copied to `garrysmod/lua/bin`:

| Configuration | Windows                  | Linux                      | macOS                    |
|---------------|--------------------------|----------------------------|--------------------------|
| x86           | `gmsv_example_win32.dll` | `gmsv_example_linux.dll`   | `gmsv_example_osx.dll`   |
| x86_64        | `gmsv_example_win64.dll` | `gmsv_example_linux64.dll` | `gmsv_example_osx64.dll` |

Building the x86 configuration on a 64-bit Linux host requires a multilib toolchain (e.g. `gcc-multilib` and `g++-multilib` on Debian/Ubuntu).

### Without premake

The build is two compiler invocations and a link. On Linux, for the 64-bit branch:

```sh
gcc -std=c89 -pedantic -fPIC -Iinclude -c gmsv_example/src/main.c -o main.o
g++ -std=c++11 -fPIC -Iinclude -c include/GarrysMod/Lua/CCompat.cpp -o ccompat.o
g++ -shared -static-libstdc++ -static-libgcc main.o ccompat.o -o gmsv_example_linux64.dll
```

Use the `gmcl_` prefix instead of `gmsv_` for a client-side module.

## Notes

- The C API is ANSI C (C89); the example premake script builds your sources with `-pedantic` to keep them that way. Set `std = 'C99'` in `premake5.lua` if you want a newer dialect.
- `Vector` and `QAngle` are plain `float x, y, z` structs in C. When the C++ `SourceCompat.h` is included first, its definitions are used instead.
- `GMOD_DLL_EXPORT` handles symbol visibility on Windows and ELF platforms; you do not need an export file.

## License

MIT, see [LICENSE](LICENSE).
