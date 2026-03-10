# Compressonator, a static texture compression library.
if(WIN32)
    message("Linking Compressonator")

    set(CMP_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/bin/x64")

    # Validate both import libs exist
    if(NOT EXISTS "${CMP_LIB_DIR}/Compressonator_MD_DLL.lib")
        message(FATAL_ERROR "Compressonator release import lib not found")
    endif()
    if(NOT EXISTS "${CMP_LIB_DIR}/Compressonator_MDd_DLL.lib")
        message(FATAL_ERROR "Compressonator debug import lib not found")
    endif()

    # SHARED IMPORTED because these are DLL import libs
    add_library(compressonator SHARED IMPORTED GLOBAL)

    set_target_properties(compressonator PROPERTIES
        # Import libs (.lib stubs) for the linker
        IMPORTED_IMPLIB_RELEASE "${CMP_LIB_DIR}/Compressonator_MD_DLL.lib"
        IMPORTED_IMPLIB_DEBUG   "${CMP_LIB_DIR}/Compressonator_MDd_DLL.lib"

        # Actual DLLs needed at runtime
        IMPORTED_LOCATION_RELEASE "${CMP_LIB_DIR}/Compressonator_MD_DLL.dll"
        IMPORTED_LOCATION_DEBUG   "${CMP_LIB_DIR}/Compressonator_MDd_DLL.dll"

        INTERFACE_LINK_OPTIONS "/ignore:4099"
    )

    target_include_directories(compressonator INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
    )
endif()