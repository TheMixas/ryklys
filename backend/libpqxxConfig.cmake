include(CMakeFindDependencyMacro)
find_dependency(PostgreSQL)
message(STATUS "postgresql found: ${PostgreSQL_FOUND}")

include("${CMAKE_CURRENT_LIST_DIR}/libpqxx-targets.cmake")