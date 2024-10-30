# Glob all shader files in the current directory
file(GLOB SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/backpack"
    "${CMAKE_CURRENT_SOURCE_DIR}/scene"  
)

# Copy all shader files to the destination directory
file(COPY ${SHADER_FILES} DESTINATION "${CMAKE_BINARY_DIR}/Debug/Resources")
