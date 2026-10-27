cmake_minimum_required(VERSION 3.22)
project(LB303 VERSION 0.99.1 LANGUAGES C CXX)

# Same JUCE 8 and static Windows CRT policy as LJuno-116 TEST95.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(LB303HelpData
    SOURCES
        "Help/LB303Help.html"
)

juce_add_plugin(LB303
    COMPANY_NAME "Lo-lo"
    BUNDLE_ID "org.lolo.lb303"
    PLUGIN_MANUFACTURER_CODE LoLo
    PLUGIN_CODE Lb03
    FORMATS VST3
    PRODUCT_NAME "LB-303"
    DESCRIPTION "Accessible monophonic bassline synthesizer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(LB303 PRIVATE
    Source/ParameterCatalog.h
    Source/ParameterCatalog.cpp
    Source/BassEngine.h
    Source/BassEngine.cpp
    Source/Oversampling.h
    Source/SharedEffects.h
    Source/SharedEffects.cpp
    Source/SequencerState.h
    Source/SequencerState.cpp
    Source/SongArrangement.h
    Source/SongArrangement.cpp
    Source/SequencerTiming.h
    Source/SequencerPlayback.h
    Source/SequencerPlayback.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/SequencerStepEditor.h
    Source/SequencerStepEditor.cpp
    Source/PresetManager.h
    Source/PresetManager.cpp
    Source/PresetBrowser.h
    Source/PresetBrowser.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/ScreenReaderAnnouncer.h
    Source/ScreenReaderAnnouncer.cpp
)
target_compile_features(LB303 PRIVATE cxx_std_20)
target_compile_definitions(LB303 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(LB303 PRIVATE
    juce::juce_audio_utils
    LB303HelpData
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
if(WIN32)
    target_link_libraries(LB303 PRIVATE Uiautomationcore OleAut32)
endif()
