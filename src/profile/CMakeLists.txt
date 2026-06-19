add_library(profile
    profile.cpp
)

target_link_libraries(profile PUBLIC nlohmann_json::nlohmann_json)

target_include_directories(profile PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
