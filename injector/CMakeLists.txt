add_executable(ddmod_injector src/main.cpp)
target_link_libraries(ddmod_injector PRIVATE psapi)
set_target_properties(ddmod_injector PROPERTIES OUTPUT_NAME "DDModInjector")
