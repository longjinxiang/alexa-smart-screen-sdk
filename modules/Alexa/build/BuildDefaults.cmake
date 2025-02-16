# Commmon build settings across all AlexaClientSDK modules.

# Append custom CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

macro(include_once module)
    if(NOT DEFINED "BuildDefaults_Include_${module}_Set")
        set("BuildDefaults_Include_${module}_Set" ON)
        include("${module}")
    endif()
endmacro()

# Disallow out-of-source-builds.
include_once(DisallowOutOfSourceBuilds)

# Setup default build options, like compiler flags and build type.
include_once(BuildOptions)

# Setup code coverage environment. This must be called after BuildOptions since it uses the variables defined there.
include_once(CodeCoverage/CodeCoverage)

# Setup package requirement variables.
include_once(PackageConfigs)

# Setup logging variables.
include_once(Logger)

# Setup keyword requirement variables.
include_once(KeywordDetector)

# Setup media player variables.
include_once(MediaPlayer)

# Setup PortAudio variables.
include_once(PortAudio)

# Setup Test Options variables.
include_once(TestOptions)

# Setup Bluetooth variables.
include_once(Bluetooth)

# Setup platform dependant variables.
include_once(Platforms)

# Setup ESP variables.
include_once(ESP)

# Setup Comms variables.
include_once(Comms)

# Setup PCC variables.
include_once(PCC)

# Setup MCC variables.
include_once(MCC)

# Setup android variables.
include_once(Android)

# Setup ffmpeg variables.
include_once(FFmpeg)

# Setup MRM variables.
include_once(MRM)

# Setup A4B variables.
include_once(A4B)

# Setup speech encoder variables.
include_once(SpeechEncoder)

# Setup Metrics variables.
include_once(Metrics)

# Setup Captions variables.
include_once(Captions)

# Setup Config Validation variables.
include_once(ConfigValidation)

if (HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS)
    include_once(ExternalMediaPlayerAdapters)
endif()

# Setup ducking options.
include_once(LocalDucking)
