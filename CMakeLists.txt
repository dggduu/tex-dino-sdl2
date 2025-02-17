cmake_minimum_required(VERSION 3.12)
project(tex-dino)

add_executable(main main.c)


find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(main
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
find_package(SDL2_image CONFIG REQUIRED)
target_link_libraries(main PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
find_package(SDL2_mixer CONFIG REQUIRED)
target_link_libraries(main PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
find_package(SDL2_net CONFIG REQUIRED)
target_link_libraries(main PRIVATE $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>)
find_package(SDL2_ttf CONFIG REQUIRED)
target_link_libraries(main PRIVATE $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>)
