cmake_minimum_required (VERSION 2.8.8)
project (manager_mutator)

include_directories (${PROJECT_SOURCE_DIR}/../mutators/)

set(MANAGER_SRC ${PROJECT_SOURCE_DIR}/manager_mutator.c)
source_group("Library Sources" FILES ${MANAGER_SRC})

add_library(manager_mutator SHARED ${MANAGER_SRC}
  $<TARGET_OBJECTS:mutators_object> $<TARGET_OBJECTS:utils_object> $<TARGET_OBJECTS:jansson_object>)
target_compile_definitions(manager_mutator PUBLIC MANAGER_MUTATOR_EXPORTS)
target_compile_definitions(manager_mutator PUBLIC MUTATORS_NO_IMPORT)
target_compile_definitions(manager_mutator PUBLIC UTILS_NO_IMPORT)
target_compile_definitions(manager_mutator PUBLIC JANSSON_NO_IMPORT)

if (WIN32) # utils.dll needs Shlwapi
  target_link_libraries(manager_mutator Shlwapi)
endif (WIN32)
