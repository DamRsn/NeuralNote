// stb_vorbis's implementation, compiled as C. tsf only ever decodes from the
// soundfont already in memory, so the stdio half is left out; the memory API
// (stb_vorbis_open_memory) is what tsf_decode_ogg calls.
//
// Kept in its own C translation unit rather than folded into tsf_impl.cpp:
// stb_vorbis.c is C, and its implicit void* conversions do not compile as C++.

#define STB_VORBIS_NO_STDIO

#include "stb_vorbis.c"
