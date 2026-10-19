add_library(OfferBribeResources MODULE
  "OfferBribeResources.rc"
)
set_target_properties(OfferBribeResources PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(OfferBribeResources PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(OfferBribeResources PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(OfferBribeResources PROPERTIES OUTPUT_NAME "OfferBribeResources")
