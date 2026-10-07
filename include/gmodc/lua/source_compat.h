#ifndef GMODC_LUA_SOURCE_COMPAT_H
#define GMODC_LUA_SOURCE_COMPAT_H

/*
  C views of the Source engine Vector and QAngle used by lua_get_vector,
  lua_push_vector, lua_get_angle and lua_push_angle. Both are three floats;
  for an angle x is pitch, y is yaw and z is roll. When the C++ SourceCompat.h
  is already included these definitions are skipped.
*/

#ifndef GARRYSMOD_LUA_SOURCECOMPAT_H

typedef struct {
  float x, y, z;
} Vector;

typedef struct {
  float x, y, z;
} QAngle;

#endif

#endif
