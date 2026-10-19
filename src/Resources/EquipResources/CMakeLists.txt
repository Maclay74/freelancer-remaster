add_library(EquipResources MODULE
  "EquipResources.rc"
)
set_target_properties(EquipResources PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(EquipResources PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(EquipResources PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(EquipResources PROPERTIES OUTPUT_NAME "EquipResources")
