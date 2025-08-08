set(srcs)

if(ESP_PLATFORM)
    list(APPEND srcs src/input_m5.cpp)
    set(requires m5unified)
else()
    list(APPEND srcs src/input_sim.c)
    set(requires )
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS include
                       REQUIRES ${requires})
