# Only build this library if built for Windows


if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_include_directories(fmod INTERFACE "${CMAKE_CURRENT_LIST_DIR}/core/inc")
    set_target_properties(fmod PROPERTIES
        IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_LIST_DIR}/core/lib/x64/fmodL.dll"
        IMPORTED_IMPLIB_DEBUG  "${CMAKE_CURRENT_LIST_DIR}/core/lib/x64/fmodL_vc.lib"
        IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/core/lib/x64/fmod.dll"
        IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_LIST_DIR}/core/lib/x64/fmod_vc.lib"
    )
endif()

