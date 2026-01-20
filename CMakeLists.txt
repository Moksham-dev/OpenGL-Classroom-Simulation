cmake_minimum_required(VERSION 3.10)
project(CL3_Classroom_Renderer)

# Set C++ standard to 17 for modern language features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Dependencies ---
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

# --- Source Configuration ---
# Organized into logic (src) and framework utilities (common)
set(SOURCES
    src/main.cpp
    common/shader.cpp
    common/texture.cpp
    common/controls.cpp
    common/objloader.cpp
    common/vboindexer.cpp
    common/tangentspace.cpp
)

# Create the executable
add_executable(ClassroomRenderer ${SOURCES})

# Include directories for project-wide header access
target_include_directories(ClassroomRenderer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# Link Required Libraries
target_link_libraries(ClassroomRenderer PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glfw
    glm::glm
)

# --- Asset Deployment ---
# Synchronizes shaders and assets into the build directory for runtime access
add_custom_command(TARGET ClassroomRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders" 
            $<TARGET_FILE_DIR:ClassroomRenderer>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "${CMAKE_CURRENT_SOURCE_DIR}/assets" 
            $<TARGET_FILE_DIR:ClassroomRenderer>/assets
    COMMENT "Synchronizing shaders and assets to build directory..."
)
