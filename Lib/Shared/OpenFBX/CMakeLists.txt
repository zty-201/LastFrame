add_library(openfbx STATIC 
${CMAKE_CURRENT_SOURCE_DIR}/openfbx/libdeflate.c 
${CMAKE_CURRENT_SOURCE_DIR}/openfbx/libdeflate.h 
${CMAKE_CURRENT_SOURCE_DIR}/openfbx/ofbx.cpp 
${CMAKE_CURRENT_SOURCE_DIR}/openfbx/ofbx.h) 

target_include_directories(openfbx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/openfbx)