cmake_minimum_required(VERSION 3.14)

project(circular_buffer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create header-only library target
add_library(circular_buffer INTERFACE)
add_library(dod::circular_buffer ALIAS circular_buffer)

# Include directories
target_include_directories(circular_buffer
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Require C++17
target_compile_features(circular_buffer INTERFACE cxx_std_17)

# Platform-specific compile definitions
if(WIN32)
    target_compile_definitions(circular_buffer INTERFACE _CRT_SECURE_NO_WARNINGS)
endif()

# Install targets (if building as standalone project)
if(PROJECT_IS_TOP_LEVEL)
    include(GNUInstallDirs)
    
    install(TARGETS circular_buffer
        EXPORT circular_buffer_targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    install(FILES circular_buffer.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/circular_buffer
    )
    
    install(EXPORT circular_buffer_targets
        FILE circular_buffer-targets.cmake
        NAMESPACE dod::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/circular_buffer
    )
    
    # Create config file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        circular_buffer-config-version.cmake
        VERSION 1.0.0
        COMPATIBILITY AnyNewerVersion
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/circular_buffer-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/circular_buffer
    )
endif() 