# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.20)

# define the project
project(
    funkin
    LANGUAGES    C ASM
    VERSION      1.0.0
    DESCRIPTION  "PSXFunkin on PSn00bSDK"
    HOMEPAGE_URL "http://lameguy64.net/?page=psn00bsdk"
)

# find tools
find_program(_chartpak
    funkinchartpak REQUIRED
    HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_arcpak
    funkinarcpak REQUIRED
    HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_timconv
    funkintimconv REQUIRED
    HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_chrpak
    funkinchrpak REQUIRED
    HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_vagpak
    funkinvagpak REQUIRED
    HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_psxavenc
    psxavenc REQUIRED
    HINTS ${PROJECT_SOURCE_DIR}/tools/build
)


function(funkin_add_archive _out)
    set(_built_archives ${_built_archives} ${_out} PARENT_SCOPE)

    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${_arcpak} ${_out} ${ARGN}
        DEPENDS ${ARGN}
        COMMENT "Building archive ${_out}"
    )
endfunction()

# build charts
file(
    GLOB_RECURSE _charts
    RELATIVE ${PROJECT_SOURCE_DIR}
    iso/chart/*.json
)
list(
    TRANSFORM _charts REPLACE "\.json$" ".cht"
    OUTPUT_VARIABLE _built_charts
)

foreach(_in _out IN ZIP_LISTS _charts _built_charts)
    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${_chartpak} ${_out} ${PROJECT_SOURCE_DIR}/${_in}
        DEPENDS ${PROJECT_SOURCE_DIR}/${_in}
        COMMENT "Building chart ${_out}"
    )
endforeach()

# build .tim images
file(
    GLOB_RECURSE _images
    RELATIVE ${PROJECT_SOURCE_DIR}
    iso/*.png
)
list(
    TRANSFORM _images REPLACE "\.png$" ".tim"
    OUTPUT_VARIABLE _built_images
)

foreach(_in _out IN ZIP_LISTS _images _built_images)
    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${_timconv} ${_out} ${PROJECT_SOURCE_DIR}/${_in}
        DEPENDS ${PROJECT_SOURCE_DIR}/${_in}
        COMMENT "Building image ${_out}"
    )
endforeach()

# build chr files
file(
    GLOB_RECURSE _characters
    RELATIVE ${PROJECT_SOURCE_DIR}
    iso/characters/*.json
)
list(
    TRANSFORM _characters REPLACE "\.json$" ".chr"
    OUTPUT_VARIABLE _built_characters
)

foreach(_in _out IN ZIP_LISTS _characters _built_characters)
    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${_chrpak} ${_out} ${PROJECT_SOURCE_DIR}/${_in}
        DEPENDS ${PROJECT_SOURCE_DIR}/${_in}
        COMMENT "Building chr ${_out}"
        VERBATIM 
    )
endforeach()

# build streamed .vag files
file(
    GLOB_RECURSE _music_files
    RELATIVE ${PROJECT_SOURCE_DIR}
    iso/music/*.txt
)
list(
    TRANSFORM _music_files REPLACE "\.txt$" ".vag"
    OUTPUT_VARIABLE _built_music
)

foreach(_in _out IN ZIP_LISTS _music_files _built_music)
    cmake_path(GET _in PARENT_PATH _dir)
    file(
        GLOB _in_files
        RELATIVE ${PROJECT_SOURCE_DIR}
        ${_dir}/*
    )
    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${_vagpak} ${_out} ${PROJECT_SOURCE_DIR}/${_in}
        DEPENDS ${_in_files}
        COMMENT "Building vag file ${_out}"
    )
endforeach()

# build .arc files
# menu
funkin_add_archive(iso/menu/menu.arc
    iso/menu/back.tim
    iso/menu/ng.tim
    iso/menu/story.tim
    iso/menu/title.tim
)

# bf
funkin_add_archive(iso/characters/bf/main.arc
    iso/characters/bf/bf0.tim 
    iso/characters/bf/bf1.tim 
    iso/characters/bf/bf2.tim 
    iso/characters/bf/bf3.tim 
    iso/characters/bf/bf4.tim 
    iso/characters/bf/bf5.tim 
    iso/characters/bf/bf6.tim 
    iso/characters/bf/dead0.tim
)
funkin_add_archive(iso/characters/bf/dead.arc
    iso/characters/bf/dead1.tim 
    iso/characters/bf/dead2.tim 
    iso/characters/bf/retry.tim
)
funkin_add_archive(iso/characters/bf/weeb.arc
    iso/characters/bf/weeb0.tim 
    iso/characters/bf/weeb1.tim
)
funkin_add_archive(iso/characters/bf/xmas.arc
    iso/characters/bf/xmasbf0.tim 
    iso/characters/bf/xmasbf1.tim 
    iso/characters/bf/xmasbf2.tim 
    iso/characters/bf/xmasbf3.tim 
    iso/characters/bf/xmasbf4.tim 
    iso/characters/bf/xmasbf5.tim
    iso/characters/bf/dead0.tim
)

# gf
funkin_add_archive(iso/characters/gf/main.arc
    iso/characters/gf/gf0.tim 
    iso/characters/gf/gf1.tim 
    iso/characters/gf/gf2.tim
)
funkin_add_archive(iso/characters/gf/xmas.arc
    iso/characters/gf/xmasgf0.tim 
    iso/characters/gf/xmasgf1.tim 
    iso/characters/gf/xmasgf2.tim
)
funkin_add_archive(iso/characters/gf/tut.arc
    iso/characters/gf/tut0.tim
    iso/characters/gf/tut1.tim
)
funkin_add_archive(iso/characters/gf/weeb.arc
    iso/characters/gf/weeb0.tim
    iso/characters/gf/weeb1.tim
)

# dad
funkin_add_archive(iso/characters/dad/main.arc
    iso/characters/dad/idle.tim 
    iso/characters/dad/left.tim 
    iso/characters/dad/down.tim 
    iso/characters/dad/up.tim 
    iso/characters/dad/right.tim
)

# week 1
funkin_add_archive(iso/week1/back.arc
    iso/week1/back0.tim
    iso/week1/back1.tim
)

# ADD ARCHIVES HERE

# build str files
file(
    GLOB_RECURSE _strs
    RELATIVE ${PROJECT_SOURCE_DIR}
    iso/movie/*.mp4
)
list(
    TRANSFORM _strs REPLACE "\.mp4$" ".str"
    OUTPUT_VARIABLE _built_strs
)

foreach(_in _out IN ZIP_LISTS _strs _built_strs)
    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${_psxavenc} -t str2 -f 37800 -b 4 -c 2 -s 320x240 -r 15 -x 2 ${PROJECT_SOURCE_DIR}/${_in} ${_out}
        DEPENDS ${PROJECT_SOURCE_DIR}/${_in}
        COMMENT "Building str ${_out}"
        VERBATIM 
    )
endforeach()

# compile the source code
file(GLOB _sources src/*.c src/*/*.c)

psn00bsdk_add_executable(funkin STATIC ${_sources})

# create bin and cue
psn00bsdk_add_cd_image(iso
    n00bfunkin # file name
    funkin.xml # path to xml
    DEPENDS 
    funkin   
    ${_built_images}
    ${_built_charts} 
    ${_built_music} 
    ${_built_archives}
    ${_built_characters}
    ${_built_audio}
    ${_built_strs}
)
