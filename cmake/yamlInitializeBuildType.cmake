
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

  # Default build type to use if none was specified
  if(NOT DEFINED CMAKE_DEFAULT_BUILD_TYPE)
    set(CMAKE_DEFAULT_BUILD_TYPE "Release")
  endif()

  message(STATUS "Setting build type to '${CMAKE_DEFAULT_BUILD_TYPE}' as none was specified.")

  set(CMAKE_BUILD_TYPE ${CMAKE_DEFAULT_BUILD_TYPE} CACHE STRING "Choose the type of build." FORCE)
  mark_as_advanced(CMAKE_BUILD_TYPE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug"
    "Release"
    "MinSizeRel"
    "RelWithDebInfo"
    )
endif()

