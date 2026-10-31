cmake_minimum_required(VERSION 3.22)
project(HEX1 VERSION 1.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

# Use a local JUCE checkout if present, otherwise download it.
if(EXISTS "${CMAKE_SOURCE_DIR}/JUCE/CMakeLists.txt")
  add_subdirectory(JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(HEX1
  COMPANY_NAME "YRAZIEL"
  COMPANY_WEBSITE "https://yraziel.com"
  BUNDLE_ID com.yraziel.wadahex
  PLUGIN_MANUFACTURER_CODE Yraz
  PLUGIN_CODE Hex1
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  FORMATS VST3 Standalone
  VST3_CATEGORIES Instrument Drum
  PRODUCT_NAME "WADAHEX")

juce_add_binary_data(HEX1Data SOURCES
  Resources/fonts/BarlowCondensed-600.ttf
  Resources/fonts/BarlowCondensed-700.ttf
  Resources/skin/plate.png
  Resources/skin/logo_lockup.png)

target_sources(HEX1 PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(HEX1 PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(HEX1
  PRIVATE HEX1Data juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Optional: headless processor tests (cmake -DHEX1_TESTS=ON)
option(HEX1_TESTS "Build processor tests" OFF)
if(HEX1_TESTS)
  add_executable(hex1_tests tests/processor_test.cpp)
  target_include_directories(hex1_tests PRIVATE $<TARGET_PROPERTY:HEX1,INCLUDE_DIRECTORIES>)
  target_compile_definitions(hex1_tests PRIVATE $<TARGET_PROPERTY:HEX1,COMPILE_DEFINITIONS>)
  target_compile_features(hex1_tests PRIVATE cxx_std_17)
  target_link_libraries(hex1_tests PRIVATE HEX1)
endif()
