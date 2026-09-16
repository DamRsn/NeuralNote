# minimp3 (CC0, lieff). A header-only library; this target exists so its
# implementation lives in exactly one translation unit, and so its warnings do
# not land on the plugin's own sources.

add_library(minimp3 STATIC minimp3_impl.cpp)

target_include_directories(minimp3 PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set_target_properties(minimp3 PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Third-party, taken as-is and not patched -- same reasoning as tinysoundfont.
if (MSVC)
  target_compile_options(minimp3 PRIVATE /w)
else ()
  target_compile_options(minimp3 PRIVATE -w)
endif ()
