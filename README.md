# Gmod Module C Wrapper
An ANSI C wrapper for Garry's Mod binary module interface. Designed for compatibility, speed and code maintainability as primary goals.

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
