macro(enable_options)
    foreach (OPTION_TO_SET IN ITEMS ${ARGN})
        set(${OPTION_TO_SET} ON CACHE BOOL "")
    endforeach ()
endmacro()

macro(disable_options)
    foreach (OPTION_TO_SET IN ITEMS ${ARGN})
        set(${OPTION_TO_SET} OFF CACHE BOOL "")
    endforeach ()
endmacro()

macro(configure_sdk_program target)
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "" FOLDER "game__sdk__tools")

    target_link_libraries(${target} PRIVATE game_sdk_shared)

    add_dependencies(${target} assets copy_assets)

    if (WIN32)
        add_dependencies(${target} copydlls)
        target_sources(${target} PRIVATE ../app.manifest ${target}.rc)
    endif()
endmacro()