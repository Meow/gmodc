#ifndef GMODC_LUA_TYPES_H
#define GMODC_LUA_TYPES_H

/*
  Type IDs for lua_is_type, lua_get_type, lua_check_type and
  lua_push_meta_table. Each entry below becomes a TYPE_ constant with the
  same numeric value as GarrysMod::Lua::Type in the C++ headers:

    TYPE_NIL, TYPE_BOOL, TYPE_LIGHTUSERDATA, TYPE_NUMBER, TYPE_STRING,
    TYPE_TABLE, TYPE_FUNCTION, TYPE_USERDATA, TYPE_THREAD,
    TYPE_ENTITY, TYPE_VECTOR, TYPE_ANGLE, ... TYPE_SURFACEINFO

  NONE (-1) is what lua_get_type returns for an empty stack slot. TYPE_COUNT
  is one past the last built-in type; IDs returned by lua_create_meta_table
  start there.

  The Source SDK defines ENTITY and VECTOR macros; they are undefined here so
  the enum names stay usable.
*/

#ifdef ENTITY
#undef ENTITY
#endif

#ifdef VECTOR
#undef VECTOR
#endif

#define GMODC_TYPES(_)                                                                            \
  /* Lua Types */                                                                                 \
  _(NIL) _(BOOL) _(LIGHTUSERDATA) _(NUMBER) _(STRING) _(TABLE) _(FUNCTION) _(USERDATA) _(THREAD)  \
  /* Gmod Types */                                                                                \
  _(ENTITY) _(VECTOR) _(ANGLE) _(PHYSOBJ) _(SAVE) _(RESTORE) _(DAMAGEINFO) _(EFFECTDATA)          \
  _(MOVEDATA) _(RECIPIENTFILTER) _(USERCMD) _(SCRIPTEDVEHICLE) _(MATERIAL) _(PANEL)               \
  _(PARTICLE) _(PARTICLEEMITTER) _(TEXTURE) _(USERMSG) _(CONVAR) _(IMESH) _(MATRIX)               \
  _(SOUND) _(PIXELVISHANDLE) _(DLIGHT) _(VIDEO) _(FILE) _(LOCOMOTION) _(PATH) _(NAVAREA)          \
  _(SOUNDHANDLE) _(NAVLADDER) _(PARTICLESYSTEM) _(PROJECTEDTEXTURE) _(PHYSCOLLIDE)                \
  _(SURFACEINFO) _(COUNT)
  
enum {
  NONE = -1,
#define _TDEF(name) TYPE_##name,
  GMODC_TYPES(_TDEF)
#undef _TDEF
  LAST
};

#endif
