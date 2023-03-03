include(CPM)

CPMAddPackage("gh:catchorg/Catch2#devel")
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)