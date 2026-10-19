cmake_minimum_required(VERSION 3.20)
project(spiral_core_sdk VERSION 0.2.0 LANGUAGES CXX)
add_library(spiral_core INTERFACE)
add_library(Spiral::Core ALIAS spiral_core)
set_target_properties(spiral_core PROPERTIES EXPORT_NAME Core)
target_include_directories(spiral_core INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(spiral_core INTERFACE cxx_std_20)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS spiral_core EXPORT SpiralCoreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT SpiralCoreTargets
 NAMESPACE Spiral::
 DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpiralCore)
configure_package_config_file(
 cmake/SpiralCoreConfig.cmake.in
 ${CMAKE_CURRENT_BINARY_DIR}/SpiralCoreConfig.cmake
 INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpiralCore)
write_basic_package_version_file(
 ${CMAKE_CURRENT_BINARY_DIR}/SpiralCoreConfigVersion.cmake
 VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES
 ${CMAKE_CURRENT_BINARY_DIR}/SpiralCoreConfig.cmake
 ${CMAKE_CURRENT_BINARY_DIR}/SpiralCoreConfigVersion.cmake
 DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpiralCore)

include(CTest)
if(BUILD_TESTING)
 add_executable(spiral_core_tests tests/core_tests.cpp)
 target_link_libraries(spiral_core_tests PRIVATE Spiral::Core)
 add_test(NAME spiral.core COMMAND spiral_core_tests)
 add_executable(spiral_contract_tests tests/contract_tests.cpp)
 target_link_libraries(spiral_contract_tests PRIVATE Spiral::Core)
 if(MSVC)
  target_compile_options(spiral_core_tests PRIVATE /UNDEBUG)
  target_compile_options(spiral_contract_tests PRIVATE /UNDEBUG)
 else()
  target_compile_options(spiral_core_tests PRIVATE -UNDEBUG)
  target_compile_options(spiral_contract_tests PRIVATE -UNDEBUG)
 endif()
 add_test(NAME spiral.contract COMMAND spiral_contract_tests)
 add_executable(spiral_event_bus_publish_tests tests/event_bus_publish_tests.cpp)
 target_link_libraries(spiral_event_bus_publish_tests PRIVATE Spiral::Core)
 if(MSVC)
  target_compile_options(spiral_event_bus_publish_tests PRIVATE /UNDEBUG)
 else()
  target_compile_options(spiral_event_bus_publish_tests PRIVATE -UNDEBUG)
 endif()
 add_test(NAME spiral.event_bus_publish COMMAND spiral_event_bus_publish_tests)
 add_executable(spiral_event_bus_unsubscribe_tests tests/event_bus_unsubscribe_tests.cpp)
 target_link_libraries(spiral_event_bus_unsubscribe_tests PRIVATE Spiral::Core)
 add_test(NAME spiral.event_bus_unsubscribe COMMAND spiral_event_bus_unsubscribe_tests)
endif()
