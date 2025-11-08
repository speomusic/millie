add_library(av-realtime STATIC
    include/merc/av/realtime/audio.h
    include/merc/av/realtime/video.h
    include/merc/av/realtime/video-impl.h)
target_include_directories(av-realtime PUBLIC include)
target_link_libraries(av-realtime PUBLIC av)
if(WIN32)
    target_sources(av-realtime PRIVATE
        source/audio.cpp
        source/video.cpp
    )
elseif(APPLE)
    set(OBJC_SOURCES
        source/audio.mm
        source/video.mm
    )
    target_sources(av-realtime PRIVATE ${OBJC_SOURCES})
    set_source_files_properties(${OBJC_SOURCES}
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(av-realtime INTERFACE "-framework AudioToolbox")
endif()
