#Only build this library if built for android


if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_include_directories(fmod INTERFACE "${CMAKE_CURRENT_LIST_DIR}/core/inc")
message(STATUS "Android Target ABI: ${ANDROID_ABI}")
if(${ANDROID_ABI} STREQUAL "arm64-v8a")
    set_target_properties(fmod PROPERTIES
        IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_LIST_DIR}/core/lib/arm64/libfmodL.so"
        IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/core/lib/arm64/libfmod.so"
	IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/core/lib/arm64/libfmod.so"
    )
elseif(${ANDROID_ABI} STREQUAL "x86_64")
    set_target_properties(fmod PROPERTIES
        IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_LIST_DIR}/core/lib/x86_64/libfmodL.so"
        IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/core/lib/x86_64/libfmod.so"
	IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/core/lib/arm64/libfmod.so"
    )
endif()
    
endif()