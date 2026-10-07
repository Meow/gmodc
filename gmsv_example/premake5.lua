local config = {
  garrysmod = '/home/example/server',
  std = 'C89'
}

-- Garry's Mod picks the binary to load by its platform suffix:
-- https://wiki.facepunch.com/gmod/Creating_Binary_Modules:_Premake
local suffixes = {
  windows = { x86 = '_win32', x86_64 = '_win64' },
  linux = { x86 = '_linux', x86_64 = '_linux64' },
  macosx = { x86 = '_osx', x86_64 = '_osx64' }
}

local is_windows = os.target() == 'windows'
local is_linux = os.target() == 'linux'
local ansi_c = string.lower(config.std) == 'c89' or string.lower(config.std) == 'ansi'
local suffix = assert(suffixes[os.target()], 'unsupported target system: '..os.target())

workspace 'example'
  location './project'
  configurations { 'x86', 'x86_64' }
  symbols 'On'
  editandcontinue 'Off'
  defines { 'NDEBUG' }
  optimize 'Full'
  floatingpoint 'Fast'

  -- Link stdlib statically for compatibility.
  if is_linux then
    linkoptions{ '-static-libstdc++', '-static-libgcc' }
  elseif is_windows then
    staticruntime 'on'
  end

  filter "configurations:x86"
    architecture 'x86'
    vectorextensions 'SSE'
    targetsuffix(suffix.x86)

  filter "configurations:x86_64"
    architecture 'x86_64'
    targetsuffix(suffix.x86_64)

project 'example'
  kind 'SharedLib'
  -- The module itself is C, only the bundled CCompat.cpp is built as C++.
  language 'C'
  cppdialect 'C++11'
  location './project'
  targetdir './bin'
  -- libdirs { '../lib/'..string.lower(os.target()) }
  includedirs { '../include' }
  targetprefix 'gmsv_'
  targetextension '.dll'

  files {
    '../include/GarrysMod/Lua/CCompat.cpp',
    'src/**.c',
    'src/**.h'
  }

  if not is_windows then
    pic 'On'

    if ansi_c then
      cdialect 'C89'

      filter 'files:**.c'
        buildoptions { '-pedantic' }
      filter {}
    end
  end

  --[[
  if is_windows then
    links { }
  else
    links {  }
  end
  --]]

  --[[
  postbuildcommands {
    '{COPY} "%{cfg.buildtarget.abspath}" "'..config.garrysmod..'/garrysmod/lua/bin/%{cfg.buildtarget.name}"',
  }
  ]]
