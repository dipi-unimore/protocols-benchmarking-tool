add_library(pbt_compiler_options INTERFACE)
target_compile_features(pbt_compiler_options INTERFACE cxx_std_23)
target_compile_options(pbt_compiler_options INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
    -Wall -Wextra -Wpedantic -O2>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2 /std:c++latest>)
