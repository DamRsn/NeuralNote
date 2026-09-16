# TinySoundFont (MIT, Bernhard Schelling) + stb_vorbis (public domain / MIT).
# Both are single-file libraries; this target exists so their implementations
# live in exactly one translation unit each, and so their warnings do not land
# on the plugin's own sources.

add_library(tinysoundfont STATIC
        tsf_impl.cpp
        stb_vorbis_impl.c)

target_include_directories(tinysoundfont PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set_target_properties(tinysoundfont PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Third-party single-file libraries, taken as-is and not patched. Their warnings
# are noise here, and silencing them keeps the plugin's own build output worth
# reading.
if (MSVC)
  target_compile_options(tinysoundfont PRIVATE /w)
else ()
  target_compile_options(tinysoundfont PRIVATE -w)
endif ()

# The decode loop runs once per soundfont load, but that load is ~40 MB of
# Vorbis and happens while the user is waiting, so it is built optimised even in
# a Debug plugin -- same reasoning as the muscriptor targets.
if (NOT NEURALNOTE_MUSCRIPTOR_DEBUG)
  if (MSVC)
    target_compile_options(tinysoundfont PRIVATE /O2 /DNDEBUG)
  else ()
    target_compile_options(tinysoundfont PRIVATE -O3 -DNDEBUG)
  endif ()
endif ()
