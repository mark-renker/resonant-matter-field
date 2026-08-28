cmake_minimum_required(VERSION 3.22)

project(ResonantMatterField VERSION 1.0.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")

if(NOT DEFINED JUCE_ROOT)
    set(JUCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/JUCE")
endif()

if(NOT EXISTS "${JUCE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found. Configure with -DJUCE_ROOT=/path/to/JUCE")
endif()

add_subdirectory("${JUCE_ROOT}" JUCE EXCLUDE_FROM_ALL)

juce_add_plugin(ResonantMatterField
    COMPANY_NAME "Resonant Matter Lab"
    BUNDLE_ID "com.resonantmatterlab.rmf"
    PLUGIN_MANUFACTURER_CODE Rmfl
    PLUGIN_CODE Rmf1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Resonant Matter Field"
    DESCRIPTION "Spatial microcontact and electromechanical modal synthesizer"
    VERSION ${PROJECT_VERSION}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_SANDBOX_SAFE TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

juce_generate_juce_header(ResonantMatterField)

target_sources(ResonantMatterField PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/RMFEditorKit.cpp
    Source/RMFEditorKit.h
    Source/DSP/ScalaTuning.cpp
    Source/DSP/ScalaTuning.h
    Source/DSP/RMFEngine.h)

target_include_directories(ResonantMatterField PRIVATE Source)

target_compile_definitions(ResonantMatterField PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(ResonantMatterField PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if(APPLE)
    # JUCE 8.0.2 may emit an optional moduleinfo.json with trailing commas.
    # Remove that nonessential file and seal the final bundle again so strict
    # hosts never encounter malformed metadata.
    add_custom_command(TARGET ResonantMatterField_VST3 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_BUNDLE_DIR:ResonantMatterField_VST3>/Contents/Resources/moduleinfo.json"
        COMMAND /usr/bin/codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:ResonantMatterField_VST3>"
        VERBATIM)
endif()

option(RMF_BUILD_TESTS "Build parser and DSP smoke tests" ON)
if(RMF_BUILD_TESTS)
    enable_testing()
    add_executable(rmf_tests Tests/ScalaTests.cpp Source/DSP/ScalaTuning.cpp)
    target_include_directories(rmf_tests PRIVATE Source)
    target_compile_definitions(rmf_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(rmf_tests PRIVATE juce::juce_core juce::juce_audio_basics)
    add_test(NAME rmf_tests COMMAND rmf_tests "${CMAKE_CURRENT_SOURCE_DIR}/Tunings")

    add_executable(rmf_render Tools/RenderDemos.cpp Source/DSP/ScalaTuning.cpp)
    target_include_directories(rmf_render PRIVATE Source)
    target_compile_definitions(rmf_render PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(rmf_render PRIVATE juce::juce_core juce::juce_audio_basics juce::juce_audio_formats)

    add_executable(rmf_state_tests Tests/StateTests.cpp)
    target_include_directories(rmf_state_tests PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/ResonantMatterField_artefacts/JuceLibraryCode")
    target_link_libraries(rmf_state_tests PRIVATE ResonantMatterField juce::juce_audio_utils juce::juce_dsp)
    add_test(NAME rmf_state_tests COMMAND rmf_state_tests "${CMAKE_CURRENT_SOURCE_DIR}/Tunings")
endif()
