add_library(ddmodcore SHARED
    src/dllmain.cpp
    src/console.cpp
    src/config.cpp
    src/hooks.cpp
)

target_include_directories(ddmodcore PRIVATE src)
target_link_libraries(ddmodcore PRIVATE minhook)

set_target_properties(ddmodcore PROPERTIES
    OUTPUT_NAME "DDModCore"
)
