add_library(alchemy MODULE
  "alchemy.cpp"
  "alchemy_types.cpp"
  "alchemy_common.cpp"
  "alchemy_math.cpp"
  "alchemy_alchemy.cpp"
  "alchemy_fxruntime.cpp"
  "alchemy_effectarchetype.cpp"
  "alchemy_effectobject.cpp"
  "alchemy_float_forward.cpp"
  "alchemy_effectlibrary.cpp"
  "alchemy_genericfx.cpp"
  "alchemy_fields.cpp"
  "alchemy_emitters.cpp"
  "alchemy_appearances.cpp"
  "alchemy_ramp.cpp"
  "alchemy_nodelibrary.cpp"
  "alchemy_factory.cpp"
  "alchemy_material.cpp"
  "alchemy.rc"
)
target_link_libraries(alchemy PRIVATE fl_build_options)
target_include_directories(alchemy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
# Preserve the original per-file ABI/precision overrides in all configurations.
set_property(SOURCE alchemy_math.cpp APPEND PROPERTY COMPILE_OPTIONS /O2 /Oi /clang:-ffp-model=fast /arch:IA32)
set_property(SOURCE alchemy_float_forward.cpp APPEND PROPERTY COMPILE_OPTIONS /O2 /arch:SSE2)
