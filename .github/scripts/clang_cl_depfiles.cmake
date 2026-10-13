# CMake 4.3+ has clang-cl write gcc-style depfiles through -clang:-MD/-MT/-MF, which ccache refuses
# to cache. /showIncludes, which it does cache, is what CMake used before.
foreach (lang C CXX)
  set(CMAKE_DEPFILE_FLAGS_${lang} "/showIncludes")
  set(CMAKE_${lang}_DEPFILE_FORMAT msvc)
endforeach ()
