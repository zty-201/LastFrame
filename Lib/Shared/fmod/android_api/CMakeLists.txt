#Only build this library if built for android


if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_include_directories(fmod INTERFACE "${CMAKE_CURRENT_LIST_DIR}/core/inc")
    set_target_properties(fmod PROPERTIES
        IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_LIST_DIR}/core/lib/arm64/libfmodL.so"
        IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/core/lib/arm64/libfmod.so"
    )
endif()