local config = {
  garrysmod = '/home/example/server',
  libname = 'gmsv_example_'..(os.target() == 'windows' and 'win32' or 'linux')..'.dll',
  std = 'C89'
}

local is_windows = os.target() == 'windows'
local is_linux = not is_windows
local ansi_c = string.lower(config.std) == 'c89' or string.lower(config.std) == 'ansi'
local suffix = is_windows and '_win' or '_linux'

workspace 'example'
  location './project'
  configurations { 'x86', 'x86_64' }
  flags { 'NoPCH', 'NoImportLib'}
  symbols 'On'
  editandcontinue 'Off'
  vectorextensions 'SSE'
  defines { 'NDEBUG' }
  optimize 'Full'
  floatingpoint 'Fast'

  -- Link stdlib statically for compatibility.
  if is_linux then
    linkoptions{ '-static-libgcc' }
  else
    staticruntime 'on'
  end

  filter "configurations:x86"
    architecture 'x86'
    targetsuffix(suffix..(is_windows and '32' or ''))

  filter "configurations:x86_64"
    architecture 'x86_64'
    targetsuffix(suffix..'64')

project 'example'
  kind 'SharedLib'
  language 'C'
  location './project'
  targetdir './bin'
  -- libdirs { '../lib/'..string.lower(os.target()) }
  includedirs { '../include' }
  targetprefix 'gmsv_'
  targetextension '.dll'

  if is_linux then
    if ansi_c then
      buildoptions { '-ansi', '-pedantic' }
    end

    pic 'On'
  end

  language 'C++'
    files { '../include/GarrysMod/Lua/CCompat.cpp' }
  language 'C'
    files {
      'src/**.c',
      'src/**.h'
    }

  --[[
  if is_windows then
    links { }
  else
    links {  }
  end
  --]]

  --[[
  postbuildcommands {
    '{COPY} "%{cfg.buildtarget.abspath}" "'..config.garrysmod..'/garrysmod/lua/bin/'..config.libname..'"*',
  }
  ]]
