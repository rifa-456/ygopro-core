if(NOT TARGET ocg_clone_boost)
    add_library(ocg_clone_boost INTERFACE)
    find_package(Boost 1.83 EXACT QUIET)
    if(Boost_FOUND)
        target_include_directories(ocg_clone_boost SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
    else()
        include(FetchContent)
        FetchContent_Declare(ocg_boost
            URL https://archives.boost.io/release/1.83.0/source/boost_1_83_0.tar.bz2
            URL_HASH SHA256=6478edfe2f3305127cffe8caf73ea0176c53769f4bf1585be237eb30798c3b8e
            SOURCE_SUBDIR ocg-header-only
        )
        FetchContent_MakeAvailable(ocg_boost)
        target_include_directories(ocg_clone_boost SYSTEM INTERFACE "${ocg_boost_SOURCE_DIR}")
    endif()
endif()
