/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <acsdkManufactory/Manufactory.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include "SampleApp/ExternalCapabilitiesBuilder.h"
#include "SampleApp/KeywordObserver.h"
#include "SampleApp/LocaleAssetsManager.h"
#include "SampleApp/SampleApplication.h"
#include "SampleApp/SampleApplicationComponent.h"
#include "SampleApp/SmartScreenCaptionPresenter.h"

#ifdef ENABLE_REVOKE_AUTH
#include "SampleApp/RevokeAuthorizationObserver.h"
#endif

#ifdef ENABLE_PCC
#include "SampleApp/PhoneCaller.h"
#endif

#ifdef ENABLE_MCC
#include "SampleApp/CalendarClient.h"
#include "SampleApp/MeetingClient.h"
#endif

#ifdef KWD
#include <KWDProvider/KeywordDetectorProvider.h>
#endif

#ifdef PORTAUDIO
#include <SampleApp/PortAudioMicrophoneWrapper.h>
#elif defined(UWP_BUILD)
#include "SSSDKCommon/NullMicrophone.h"
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#elif defined(UWP_BUILD)
#include <SSSDKCommon/NullMediaSpeaker.h>
#include <SSSDKCommon/NullEqualizer.h>
#endif

#ifdef ANDROID
#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
#include <AndroidUtilities/AndroidSLESEngine.h>
#endif

#ifdef ANDROID_MEDIA_PLAYER
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#include <AndroidSLESMediaPlayer/AndroidSLESSpeaker.h>
#endif

#ifdef ANDROID_MICROPHONE
#include <AndroidUtilities/AndroidSLESMicrophone.h>
#endif

#ifdef ANDROID_LOGGER
#include <AndroidUtilities/AndroidLogger.h>
#endif

#endif

#ifdef BLUETOOTH_BLUEZ
#include <BlueZ/BlueZBluetoothDeviceManager.h>
#endif

#ifdef POWER_CONTROLLER
#include "SampleApp/PeripheralEndpoint/PeripheralEndpointPowerControllerHandler.h"
#endif

#ifdef TOGGLE_CONTROLLER
#include <ToggleController/ToggleControllerAttributeBuilder.h>
#include "SampleApp/PeripheralEndpoint/PeripheralEndpointToggleControllerHandler.h"
#include "SampleApp/DefaultEndpoint/DefaultEndpointToggleControllerHandler.h"
#endif

#ifdef RANGE_CONTROLLER
#include <RangeController/RangeControllerAttributeBuilder.h>
#include "SampleApp/PeripheralEndpoint/PeripheralEndpointRangeControllerHandler.h"
#include "SampleApp/DefaultEndpoint/DefaultEndpointRangeControllerHandler.h"
#endif

#ifdef MODE_CONTROLLER
#include <ModeController/ModeControllerAttributeBuilder.h>
#include "SampleApp/PeripheralEndpoint/PeripheralEndpointModeControllerHandler.h"
#include "SampleApp/DefaultEndpoint/DefaultEndpointModeControllerHandler.h"
#endif

#ifdef ENABLE_LPM
#include <AVSCommon/Utils/Power/NoOpPowerResourceManager.h>
#endif

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/ProtocolTracerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <Audio/MicrophoneInterface.h>
#include <acsdkBluetooth/BasicDeviceConnectionRule.h>
#include <acsdkBluetooth/SQLiteBluetoothStorage.h>
#include <acsdkNotifications/SQLiteNotificationsStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <SampleApp/SampleEqualizerModeController.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>
#include <SSSDKCommon/ConfigValidator.h>

#include <acsdkEqualizerImplementations/InMemoryEqualizerConfiguration.h>
#include <acsdkEqualizerImplementations/MiscDBEqualizerStorage.h>
#include <acsdkEqualizerImplementations/SDKConfigEqualizerConfiguration.h>
#include <acsdkEqualizerInterfaces/EqualizerInterface.h>
#include <InterruptModel/config/InterruptModelConfiguration.h>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <rapidjson/istreamwrapper.h>

#ifndef UWP_BUILD
#include <Communication/WebSocketServer.h>
#else
#include "UWPSampleApp/UWPSampleApp/include/UWPSampleApp/NullSocketServer.h"
#endif

#include "SampleApp/JsonUIManager.h"

#ifdef CUSTOM_MEDIA_PLAYER
namespace alexaSmartScreenSDK {

std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ApplicationMediaInterfaces> createCustomMediaPlayer(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>
        contentFetcherFactory,
    bool enableEqualizer,
    const std::string& name,
    bool enableLiveMode);

}  // namespace alexaSmartScreenSDK
#endif

namespace alexaSmartScreenSDK {
namespace sampleApp {

/**
 * WebSocket interface to listen on.
 * WARNING: If this is changed to listen on a publicly accessible interface then additional
 * security precautions will need to be taken to secure client connections and authenticate
 * connecting clients.
 */
static const std::string DEFAULT_WEBSOCKET_INTERFACE = "127.0.0.1";

/// WebSocket port to listen on.
static const int DEFAULT_WEBSOCKET_PORT = 8933;

/// The sample rate of microphone audio data.
static const unsigned int SAMPLE_RATE_HZ = 16000;

/// The number of audio channels.
static const unsigned int NUM_CHANNELS = 1;

/// The size of each word within the stream.
static const size_t WORD_SIZE = 2;

/// The maximum number of readers of the stream.
static const size_t MAX_READERS = 10;

/// The default number of MediaPlayers used by AudioPlayer CA/
/// Can be overridden in the Configuration using @c AUDIO_MEDIAPLAYER_POOL_SIZE_KEY
static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2;

/// The amount of audio data to keep in the ring buffer.
static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);

/// The size of the ring buffer.
static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

/// Key for the root node value containing configuration values for SampleApp.
static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");

/// Key for the root node value containing configuration values for Equalizer.
static const std::string EQUALIZER_CONFIG_KEY("equalizer");

/// Key for the @c firmwareVersion value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");

/// Key for the @c endpoint value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY("endpoint");

/// Key for setting the interface which websockets will bind to @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string WEBSOCKET_INTERFACE_KEY("websocketInterface");

/// Key for setting the port number which websockets will listen on @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string WEBSOCKET_PORT_KEY("websocketPort");

/// Key for setting the certificate file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG configuration
/// node.
static const std::string WEBSOCKET_CERTIFICATE("websocketCertificate");

/// Key for setting the private key file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG configuration
/// node.
static const std::string WEBSOCKET_PRIVATE_KEY("websocketPrivateKey");

/// Key for setting the certificate authority file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG
/// configuration node.
static const std::string WEBSOCKET_CERTIFICATE_AUTHORITY("websocketCertificateAuthority");

/// Key for the Audio MediaPlayer pool size.
static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");

/// Key for cache reuse period for imported packages in seconds.
static const std::string CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY("contentCacheReusePeriodInSeconds");

/// Default value for cache reuse period.
static const std::string DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS("600");

/// Key for max number of cache entries for imported packages.
static const std::string CONTENT_CACHE_MAX_SIZE_KEY("contentCacheMaxSize");

/// Default value for max number of cache entries for imported packages.
static const std::string DEFAULT_CONTENT_CACHE_MAX_SIZE("50");

/// The key in our config file to find the maxNumberOfConcurrentDownloads configuration.
static const std::string MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY = "maxNumberOfConcurrentDownloads";

// The default value for the maximum number of concurrent downloads.
static const int DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD = 5;

using namespace alexaClientSDK;
using namespace alexaClientSDK::acsdkExternalMediaPlayer;
using namespace alexaClientSDK::acsdkManufactory;
using namespace alexaClientSDK::avsCommon;
using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using MediaPlayerInterface = alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface;
using ApplicationMediaInterfaces = alexaClientSDK::avsCommon::sdkInterfaces::ApplicationMediaInterfaces;

/// The @c m_playerToMediaPlayerMap Map of the adapter to their speaker-type and MediaPlayer creation methods.
std::unordered_map<std::string, alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface::Type>
    SampleApplication::m_playerToSpeakerTypeMap;

/// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
std::unordered_map<std::string, ExternalMediaPlayer::AdapterCreateFunction> SampleApplication::m_adapterToCreateFuncMap;

/// String to identify log entries originating from this file.
static const std::string TAG("SampleApplication");

#ifdef ENABLE_ENDPOINT_CONTROLLERS
/// The instance name for the toggle controller in default endpoint
static const std::string DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME("DefaultEndpoint.Light");

/// The friendly name of the toggle controller in default endpoint.
static const std::string DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME("Light");

/// The instance name for the range controller in the default endpoint.
static const std::string DEFAULT_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME("DefaultEndpoint.FanSpeed");

/// The instance name for the mode controller in the default endpoint.
static const std::string DEFAULT_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME("DefaultEndpoint.Mode");

#ifdef RANGE_CONTROLLER
/// The range value for the prest 'high' for range controller in the default endpoint.
static const double DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH = 5;

/// The range value for the prest 'medium' for range controller in the default endpoint.
static const double DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM = 3;

/// The range value for the prest 'low' for range controller in the default endpoint.
static const double DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW = 1;
#endif  // RANGE_CONTROLLER

// Note: Discoball is an imaginary peripheral endpoint that is connected to the device with the following
// capabilities : power, light (toggle), rotation speed (range) and the color of light (mode).

/// The derived endpoint Id used in endpoint creation.
static const std::string PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID("Discoball");

/// The description of the endpoint.
static const std::string PERIPHERAL_ENDPOINT_DESCRIPTION("Sample Discoball Description");

/// The friendly name of the peripheral endpoint. This is used in utterance.
static const std::string PERIPHERAL_ENDPOINT_FRIENDLYNAME("Discoball");

/// The manufacturer of peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_MANUFACTURER_NAME("Sample Manufacturer");

/// The display category of the peripheral endpoint.
static const std::vector<std::string> PERIPHERAL_ENDPOINT_DISPLAYCATEGORY({"OTHER"});

/// The instance name for the toggle controller in peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME("Discoball.Light");

/// The friendly name of the toggle controller on peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME("Light");

/// The instance name for the range controller peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME("Discoball.Height");

/// The friendly name of the range controller on peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_FRIENDLY_NAME("Height");

/// The instance name for the mode controller peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME("Discoball.Mode");

/// The friendly name of the mode controller on peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_MODE_CONTROLLER_FRIENDLY_NAME("Light");

/// The model of the peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL("Model1");

/// Serial number of the peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER("123456789");

/// Firmware version number peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION("1.0");

/// Software Version number peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION("1.0");

/// The custom identifier peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER("SampleApp");

#ifdef RANGE_CONTROLLER
/// The range controller preset 'high' in peripheral endpoint.
static const double PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH = 10;

/// The range controller preset 'medium' in peripheral endpoint.
static const double PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM = 5;

/// The range controller preset 'low' in peripheral endpoint.
static const double PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW = 1;

/// The action ID for "raise" semantic annotations.
static const std::string SEMANTICS_ACTION_ID_RAISE("Alexa.Actions.Raise");

/// The action ID for "lower" semantic annotations.
static const std::string SEMANTICS_ACTION_ID_LOWER("Alexa.Actions.Lower");

/// The range controller SetRangeValue directive name.
static const std::string SETRANGE_DIRECTIVE_NAME("SetRangeValue");

/// The range controller SetRangeValue payload mapped to the raise action.
static const std::string PERIPHERAL_ENDPOINT_RAISE_PAYLOAD =
    R"({"rangeValue":)" + std::to_string(PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH) + R"(})";

/// The range controller SetRangeValue payload mapped to the lower action.
static const std::string PERIPHERAL_ENDPOINT_LOWER_PAYLOAD =
    R"({"rangeValue":)" + std::to_string(PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW) + R"(})";

#endif  // RANGE_CONTROLLER

/// US English locale string.
static const std::string EN_US("en-US");
#endif  // ENABLE_ENDPOINT_CONTROLLERS

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// A set of all log levels.
static const std::set<alexaClientSDK::avsCommon::utils::logger::Level> allLevels = {
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG9,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG8,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG7,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG6,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG5,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG4,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG3,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG2,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG1,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG0,
    alexaClientSDK::avsCommon::utils::logger::Level::INFO,
    alexaClientSDK::avsCommon::utils::logger::Level::WARN,
    alexaClientSDK::avsCommon::utils::logger::Level::ERROR,
    alexaClientSDK::avsCommon::utils::logger::Level::CRITICAL,
    alexaClientSDK::avsCommon::utils::logger::Level::NONE};

#ifdef ENABLE_ENDPOINT_CONTROLLERS

/// Struct representing the friendly name.
struct FriendlyName {
    /// Type of the friendly name.
    enum class Type {
        /// Friendly name as asset type.
        ASSET,
        /// Friendly name as text type.
        TEXT
    };

    /// Holds the type of the friendly name.
    Type type;

    /// Value contains the assetId when Type::ASSET or it contains the text when Type::TEXT.
    std::string value;
};

/// The capability resources of primitive controllers.
struct CapabilityResources {
    /// Represents the friendly name of capability.
    std::vector<FriendlyName> friendlyNames;
};

/// This struct represents the Range Controller preset and its friendly names.
struct RangeControllerPresetResources {
    /// The value of a preset.
    double presetValue;

    /// The friendly names of the presets.
    std::vector<FriendlyName> friendlyNames;
};

/// This struct represents the Mode Controller modes and its friendly names.
struct ModeControllerModeResources {
    /// The mode in the Mode Controller.
    std::string mode;

    /// The friendly names of the @c mode.
    std::vector<FriendlyName> friendlyNames;
};

#endif

/**
 * Gets a log level consumable by the SDK based on the user input string for log level.
 *
 * @param userInputLogLevel The string to be parsed into a log level.
 * @return The log level. This will default to NONE if the input string is not properly parsable.
 */
static alexaClientSDK::avsCommon::utils::logger::Level getLogLevelFromUserInput(std::string userInputLogLevel) {
    std::transform(userInputLogLevel.begin(), userInputLogLevel.end(), userInputLogLevel.begin(), ::toupper);
    return alexaClientSDK::avsCommon::utils::logger::convertNameToLevel(userInputLogLevel);
}

/**
 * Allows the process to ignore the SIGPIPE signal.
 * The SIGPIPE signal may be received when the application performs a write to a closed socket.
 * This is a case that arises in the use of certain networking libraries.
 *
 * @return true if the action for handling SIGPIPEs was correctly set to ignore, else false.
 */
static bool ignoreSigpipeSignals() {
#ifndef NO_SIGPIPE
    if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return false;
    }
#endif
    return true;
}

#ifdef ENABLE_ENDPOINT_CONTROLLERS
#ifdef TOGGLE_CONTROLLER
/**
 * Helper function to build the @c ToggleControllerAttributes
 *
 * @param capabilityResources The @c CapabilityResources containing friendly names of controller.
 * @return A non optional @c ToggleControllerAttributes if the build succeeds; otherwise, an empty
 * @c ToggleControllerAttributes object.
 */
static avsCommon::utils::Optional<avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes>
buildToggleControllerAttributes(const CapabilityResources& capabilityResources) {
    auto controllerAttribute =
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes>();
    auto toggleControllerAttributeBuilder =
        capabilityAgents::toggleController::ToggleControllerAttributeBuilder::create();
    if (!toggleControllerAttributeBuilder) {
        ACSDK_ERROR(LX("Failed to create default endpoint toggle controller attribute builder!"));
        return controllerAttribute;
    }

    if (capabilityResources.friendlyNames.size() == 0) {
        ACSDK_ERROR(LX("buildToggleControllerAttributesFailed").m("noFriendlyNames"));
        return controllerAttribute;
    }

    auto toggleControllerCapabilityResources = avsCommon::avs::CapabilityResources();
    for (auto& friendlyName : capabilityResources.friendlyNames) {
        if (FriendlyName::Type::ASSET == friendlyName.type) {
            if (!toggleControllerCapabilityResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                ACSDK_ERROR(LX("buildToggleControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
        if (FriendlyName::Type::TEXT == friendlyName.type) {
            if (!toggleControllerCapabilityResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                ACSDK_ERROR(LX("buildToggleControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
    }

    toggleControllerAttributeBuilder->withCapabilityResources(toggleControllerCapabilityResources);

    return toggleControllerAttributeBuilder->build();
}
#endif  // TOGGLE_CONTROLLER

#ifdef RANGE_CONTROLLER
/**
 * Helper function to build the @c RangeControllerAttributes
 *
 * @param capabilityResources The @c CapabilityResources containing friendly names of controller.
 * @param rangeControllerPresetResources A vector representing the preset resources used for building the configuration.
 * @param semantics An optional @c CapabilitySemantics for the range controller.
 * @return A non optional @c RangeControllerAttributes if the build succeeds; otherwise, an empty
 * @c RangeControllerAttributes object.
 */
static avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>
buildRangeControllerAttributes(
    const CapabilityResources& capabilityResources,
    const std::vector<RangeControllerPresetResources>& rangeControllerPresetResources,
    const avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics> semantics =
        avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics>()) {
    auto controllerAttribute =
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>();

    auto rangeControllerAttributeBuilder = capabilityAgents::rangeController::RangeControllerAttributeBuilder::create();
    if (!rangeControllerAttributeBuilder) {
        ACSDK_ERROR(LX("Failed to create range controller attribute builder!"));
        return controllerAttribute;
    }

    if (capabilityResources.friendlyNames.size() == 0) {
        ACSDK_ERROR(LX("buildRangeControllerAttributesFailed").m("emptyCapabilityFriendlyNames"));
        return controllerAttribute;
    }

    auto rangeControllerCapabilityResources = avsCommon::avs::CapabilityResources();
    for (auto& friendlyName : capabilityResources.friendlyNames) {
        if (FriendlyName::Type::ASSET == friendlyName.type) {
            if (!rangeControllerCapabilityResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
        if (FriendlyName::Type::TEXT == friendlyName.type) {
            if (!rangeControllerCapabilityResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
    }

    rangeControllerAttributeBuilder->withCapabilityResources(rangeControllerCapabilityResources);

    if (rangeControllerPresetResources.size() > 0) {
        for (auto& presetResource : rangeControllerPresetResources) {
            if (presetResource.friendlyNames.size() == 0) {
                ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                .m("buildRangeControllerAttributesFailed")
                                .m("noPresetFriendlyNames")
                                .d("presetValue", presetResource.presetValue));
                return controllerAttribute;
            }
            auto presetResources = avsCommon::avs::CapabilityResources();
            for (auto& friendlyName : presetResource.friendlyNames) {
                if (FriendlyName::Type::ASSET == friendlyName.type) {
                    if (!presetResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                        ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("presetValue", presetResource.presetValue));
                        return controllerAttribute;
                    }
                }
                if (FriendlyName::Type::TEXT == friendlyName.type) {
                    if (!presetResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                        ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("presetValue", presetResource.presetValue));
                        return controllerAttribute;
                    }
                }
            }
            rangeControllerAttributeBuilder->addPreset(std::make_pair(presetResource.presetValue, presetResources));
        }
    }

    if (semantics.hasValue()) {
        if (!semantics.value().isValid()) {
            ACSDK_ERROR(LX("buildRangeControllerAttributes").m("invalidSemantics"));
            return controllerAttribute;
        }
        rangeControllerAttributeBuilder->withSemantics(semantics.value());
    }

    return rangeControllerAttributeBuilder->build();
}
#endif  // RANGE_CONTROLLER

#ifdef MODE_CONTROLLER
/**
 * Helper function to build the @c ModeControllerAttributes
 *
 * @param capabilityResources The @c CapabilityResources containing friendly names of controller.
 * @param presetResources A vector representing the mode resources used for building the configuration.
 * @return A non optional @c ModeControllerAttributes if the build succeeds; otherwise, an empty
 * @c ModeControllerAttributes object.
 */
static avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes>
buildModeControllerAttributes(
    const CapabilityResources& capabilityResources,
    const std::vector<ModeControllerModeResources>& modeControllerModeResources) {
    auto controllerAttribute =
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes>();

    auto modeControllerAttributeBuilder = capabilityAgents::modeController::ModeControllerAttributeBuilder::create();
    if (!modeControllerAttributeBuilder) {
        ACSDK_ERROR(LX("Failed to create mode controller attribute builder!"));
        return controllerAttribute;
    }

    if (capabilityResources.friendlyNames.size() == 0) {
        ACSDK_ERROR(LX("buildModeControllerAttributesFailed").m("emptyCapabilityFriendlyNames"));
        return controllerAttribute;
    }

    auto modeControllerCapabilityResources = avsCommon::avs::CapabilityResources();
    for (auto& friendlyName : capabilityResources.friendlyNames) {
        if (FriendlyName::Type::ASSET == friendlyName.type) {
            if (!modeControllerCapabilityResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                ACSDK_ERROR(LX("buildModeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
        if (FriendlyName::Type::TEXT == friendlyName.type) {
            if (!modeControllerCapabilityResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                ACSDK_ERROR(LX("buildModeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
    }

    modeControllerAttributeBuilder->withCapabilityResources(modeControllerCapabilityResources);

    if (modeControllerModeResources.size() > 0) {
        for (auto& modeResource : modeControllerModeResources) {
            if (modeResource.friendlyNames.size() == 0) {
                ACSDK_ERROR(LX("buildModeControllerAttributes")
                                .m("buildModeControllerAttributesFailed")
                                .m("noPresetFriendlyNames")
                                .d("mode", modeResource.mode));
                return controllerAttribute;
            }
            auto modeResources = avsCommon::avs::CapabilityResources();
            for (auto& friendlyName : modeResource.friendlyNames) {
                if (FriendlyName::Type::ASSET == friendlyName.type) {
                    if (!modeResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                        ACSDK_ERROR(LX("buildModeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("mode", modeResource.mode));
                        return controllerAttribute;
                    }
                }
                if (FriendlyName::Type::TEXT == friendlyName.type) {
                    if (!modeResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                        ACSDK_ERROR(LX("buildModeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("mode", modeResource.mode));
                        return controllerAttribute;
                    }
                }
            }
            modeControllerAttributeBuilder->addMode(modeResource.mode, modeResources);
        }
        modeControllerAttributeBuilder->setOrdered(true);
    }

    return modeControllerAttributeBuilder->build();
}
#endif  // MODE_CONTROLLER
#endif  // ENABLE_ENDPOINT_CONTROLLERS

std::unique_ptr<SampleApplication> SampleApplication::create(
    const std::vector<std::string>& configFiles,
    const std::string& pathToInputFolder,
    const std::string& logLevel,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics) {
    auto clientApplication = std::unique_ptr<SampleApplication>(new SampleApplication);
    if (!clientApplication->initialize(configFiles, pathToInputFolder, logLevel, diagnostics)) {
        ACSDK_CRITICAL(LX("Failed to initialize SampleApplication"));
        return nullptr;
    }
    if (!ignoreSigpipeSignals()) {
        ACSDK_CRITICAL(LX("Failed to set a signal handler for SIGPIPE"));
        return nullptr;
    }

    return clientApplication;
}

SampleApplication::AdapterRegistration::AdapterRegistration(
    const std::string& playerId,
    ExternalMediaPlayer::AdapterCreateFunction createFunction) {
    ACSDK_DEBUG0(LX(__func__).d("id", playerId));
    if (m_adapterToCreateFuncMap.find(playerId) != m_adapterToCreateFuncMap.end()) {
        ACSDK_WARN(LX("Adapter already exists").d("playerID", playerId));
    }

    m_adapterToCreateFuncMap[playerId] = createFunction;
}

SampleApplication::MediaPlayerRegistration::MediaPlayerRegistration(
    const std::string& playerId,
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type speakerType) {
    ACSDK_DEBUG0(LX(__func__).d("id", playerId));
    if (m_playerToSpeakerTypeMap.find(playerId) != m_playerToSpeakerTypeMap.end()) {
        ACSDK_WARN(LX("MediaPlayer already exists").d("playerId", playerId));
    }

    m_playerToSpeakerTypeMap[playerId] = speakerType;
}

SampleAppReturnCode SampleApplication::run() {
    return m_guiClient->run();
}

SampleApplication::~SampleApplication() {
    if (m_aplClientBridge) {
        m_aplClientBridge->shutdown();
    }

    if (m_guiManager) {
        m_guiManager->shutdown();
    }

    if (m_guiClient) {
        m_guiClient->shutdown();
    }
    // Clean up anything that depends on the the MediaPlayers.
    m_externalMusicProviderMediaPlayersMap.clear();

    for (auto& shutdownable : m_shutdownRequiredList) {
        if (!shutdownable) {
            ACSDK_ERROR(LX("shutdownFailed").m("Component requiring shutdown was null"));
        }
        shutdownable->shutdown();
    }

    m_sdkInit.reset();
}

bool SampleApplication::createMediaPlayersForAdapters(
    const std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>&
        httpContentFetcherFactory,
    const std::shared_ptr<smartScreenClient::EqualizerRuntimeSetup> equalizerRuntimeSetup) {
    bool equalizerEnabled = equalizerRuntimeSetup->isEnabled();

    for (auto& entry : m_playerToSpeakerTypeMap) {
        auto applicationMediaInterfaces = createApplicationMediaPlayer(
            httpContentFetcherFactory, equalizerEnabled, entry.first + "MediaPlayer", false);
        if (applicationMediaInterfaces) {
            m_externalMusicProviderMediaPlayersMap[entry.first] = applicationMediaInterfaces->mediaPlayer;
            m_externalMusicProviderSpeakersMap[entry.first] = applicationMediaInterfaces->speaker;
            if (equalizerEnabled) {
                equalizerRuntimeSetup->addEqualizer(applicationMediaInterfaces->equalizer);
            }
        } else {
            ACSDK_CRITICAL(LX("Failed to create application media interface").d("playerId", entry.first));
            return false;
        }
    }

    return true;
}

bool SampleApplication::initialize(
    const std::vector<std::string>& configFiles,
    const std::string& pathToInputFolder,
    const std::string& logLevel,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics) {
    avsCommon::utils::logger::Level logLevelValue = avsCommon::utils::logger::Level::UNKNOWN;

    if (!logLevel.empty()) {
        logLevelValue = getLogLevelFromUserInput(logLevel);
        if (alexaClientSDK::avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
            alexaSmartScreenSDK::sampleApp::ConsolePrinter::simplePrint("Unknown log level input!");
            alexaSmartScreenSDK::sampleApp::ConsolePrinter::simplePrint("Possible log level options are: ");
            for (auto it = allLevels.begin(); it != allLevels.end(); ++it) {
                alexaSmartScreenSDK::sampleApp::ConsolePrinter::simplePrint(
                    alexaClientSDK::avsCommon::utils::logger::convertLevelToName(*it));
            }
            return false;
        }

        alexaSmartScreenSDK::sampleApp::ConsolePrinter::simplePrint(
            "Running app with log level: " +
            alexaClientSDK::avsCommon::utils::logger::convertLevelToName(logLevelValue));
    }

    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().setLevel(logLevelValue);

    auto configJsonStreams = std::make_shared<std::vector<std::shared_ptr<std::istream>>>();

    for (auto configFile : configFiles) {
        if (configFile.empty()) {
            alexaSmartScreenSDK::sampleApp::ConsolePrinter::simplePrint("Config filename is empty!");
            return false;
        }

        auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
        if (!configInFile->good()) {
            ACSDK_CRITICAL(LX("Failed to read config file").d("filename", configFile));
            alexaSmartScreenSDK::sampleApp::ConsolePrinter::simplePrint("Failed to read config file " + configFile);
            return false;
        }

        configJsonStreams->push_back(configInFile);
    }

    bool enableDucking = true;

#ifdef DISABLE_DUCKING
    enableDucking = false;
#endif

    // Add the InterruptModel Configuration.
    configJsonStreams->push_back(
        alexaClientSDK::afml::interruptModel::InterruptModelConfiguration::getConfig(enableDucking));

    auto builder = initialization::InitializationParametersBuilder::create();
    if (!builder) {
        ACSDK_ERROR(LX("createInitializeParamsFailed").d("reason", "nullBuilder"));
        return false;
    }

    builder->withJsonStreams(configJsonStreams);

#ifdef ENABLE_LPM
    builder->withPowerResourceManager(std::make_shared<avsCommon::utils::power::NoOpPowerResourceManager>());
#endif

    auto initParams = builder->build();
    auto sampleAppComponent = getComponent(std::move(initParams), m_shutdownRequiredList);

    auto manufactory = Manufactory<
        std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
        std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
        std::shared_ptr<avsCommon::utils::DeviceInfo>,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>::create(sampleAppComponent);

    m_sdkInit = manufactory->get<std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>>();
    if (!m_sdkInit) {
        ACSDK_CRITICAL(LX("Failed to get SDKInit!"));
        return false;
    }

    auto configPtr = manufactory->get<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>();
    if (!configPtr) {
        ACSDK_CRITICAL(LX("Failed to acquire the configuration"));
        return false;
    }
    auto& config = *configPtr;

#ifdef ENABLE_CONFIG_VALIDATION

    /*
     * Creating config validator
     */
    auto configValidator = alexaSmartScreenSDK::sssdkCommon::ConfigValidator::create();
    if (!configValidator) {
        ACSDK_CRITICAL(LX("Failed to create config validator!"));
        return false;
    }

    auto schemaStringInAscii = decodeHexToAscii(SCHEMA_HEX);
    auto schemaStream = std::stringstream(schemaStringInAscii);

    if (schemaStream.good()) {
        rapidjson::IStreamWrapper isw(schemaStream);
        rapidjson::Document jsonSchema;
        if (!jsonSchema.ParseStream(isw).HasParseError()) {
            // Validating config parameters
            if (!configValidator->validate(config, jsonSchema)) {
                ACSDK_ERROR(LX("Configuration validation failed!"));
                return false;
            }
        } else {
            ACSDK_ERROR(LX("Configuration file could not be validated!").d("reason", "invalid json schema"));
            return false;
        }
    } else {
        ACSDK_ERROR(LX("Configuration file could not be validated!").d("reason", "failed to read schema string"));
        return false;
    }

#endif

    auto sampleAppConfig = config[SAMPLE_APP_CONFIG_KEY];

    auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

    // Creating the misc DB object to be used by various components.
    std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
        alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(config);

    /*
     * Creating Equalizer specific implementations
     */
    auto equalizerConfigBranch = config[EQUALIZER_CONFIG_KEY];
    auto equalizerConfiguration = acsdkEqualizer::SDKConfigEqualizerConfiguration::create(equalizerConfigBranch);
    std::shared_ptr<smartScreenClient::EqualizerRuntimeSetup> equalizerRuntimeSetup =
        std::make_shared<smartScreenClient::EqualizerRuntimeSetup>(false);

    bool equalizerEnabled = false;
    if (equalizerConfiguration && equalizerConfiguration->isEnabled()) {
        equalizerEnabled = true;
        equalizerRuntimeSetup = std::make_shared<smartScreenClient::EqualizerRuntimeSetup>();
        auto equalizerStorage = acsdkEqualizer::MiscDBEqualizerStorage::create(miscStorage);
        auto equalizerModeController = sampleApp::SampleEqualizerModeController::create();

        equalizerRuntimeSetup->setStorage(equalizerStorage);
        equalizerRuntimeSetup->setConfiguration(equalizerConfiguration);
        equalizerRuntimeSetup->setModeController(equalizerModeController);
    }

#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
    m_openSlEngine = applicationUtilities::androidUtilities::AndroidSLESEngine::create();
    if (!m_openSlEngine) {
        ACSDK_ERROR(LX("createAndroidMicFailed").d("reason", "failed to create engine"));
        return false;
    }
#endif

    auto speakerMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "SpeakMediaPlayer");
    if (!speakerMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for speech!"));
        return false;
    }
    m_speakMediaPlayer = speakerMediaInterfaces->mediaPlayer;

    int poolSize;
    sampleAppConfig.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);
    std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>> audioSpeakers;

    for (int index = 0; index < poolSize; index++) {
        std::shared_ptr<MediaPlayerInterface> mediaPlayer;
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker;
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer;

        auto audioMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, equalizerEnabled, "AudioMediaPlayer");
        if (!audioMediaInterfaces) {
            ACSDK_CRITICAL(LX("Failed to create application media interfaces for audio!"));
            return false;
        }
        m_audioMediaPlayerPool.push_back(audioMediaInterfaces->mediaPlayer);
        audioSpeakers.push_back(audioMediaInterfaces->speaker);
        // Creating equalizers
        if (equalizerRuntimeSetup->isEnabled()) {
            equalizerRuntimeSetup->addEqualizer(audioMediaInterfaces->equalizer);
        }
    }

    avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::Fingerprint> fingerprint =
        (*(m_audioMediaPlayerPool.begin()))->getFingerprint();
    auto audioMediaPlayerFactory = std::unique_ptr<mediaPlayer::PooledMediaPlayerFactory>();
    if (fingerprint.hasValue()) {
        audioMediaPlayerFactory =
            mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool, fingerprint.value());
    } else {
        audioMediaPlayerFactory = mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool);
    }
    if (!audioMediaPlayerFactory) {
        ACSDK_CRITICAL(LX("Failed to create media player factory for content!"));
        return false;
    }

    auto notificationMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "NotificationsMediaPlayer");
    if (!notificationMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for notifications!"));
        return false;
    }
    m_notificationsMediaPlayer = notificationMediaInterfaces->mediaPlayer;

    auto bluetoothMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "BluetoothMediaPlayer");
    if (!bluetoothMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for bluetooth!"));
        return false;
    }
    m_bluetoothMediaPlayer = bluetoothMediaInterfaces->mediaPlayer;

    auto ringtoneMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "RingtoneMediaPlayer");
    if (!ringtoneMediaInterfaces) {
        alexaSmartScreenSDK::sampleApp::ConsolePrinter::simplePrint(
            "Failed to create application media interfaces for ringtones!");
        return false;
    }
    m_ringtoneMediaPlayer = ringtoneMediaInterfaces->mediaPlayer;

#ifdef ENABLE_COMMS_AUDIO_PROXY
    auto commsMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "CommsMediaPlayer", true);
    if (!commsMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for comms!"));
        return false;
    }
    m_commsMediaPlayer = commsMediaInterfaces->mediaPlayer;
    auto commsSpeaker = commsMediaInterfaces->speaker;
#endif

    auto alertsMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "AlertsMediaPlayer");
    if (!alertsMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for alerts!"));
        return false;
    }
    m_alertsMediaPlayer = alertsMediaInterfaces->mediaPlayer;

    auto systemSoundMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "SystemSoundMediaPlayer");
    if (!systemSoundMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for system sound player!"));
        return false;
    }
    m_systemSoundMediaPlayer = systemSoundMediaInterfaces->mediaPlayer;

#ifdef ENABLE_PCC
    auto phoneMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "PhoneMediaPlayer");
    if (!phoneMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for phone!"));
        return false;
    }
    auto phoneSpeaker = phoneMediaInterfaces->speaker;
#endif

#ifdef ENABLE_MCC
    auto meetingMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "MeetingMediaPlayer");
    if (!meetingMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for meeting client!"));
        return false;
    }
    auto meetingSpeaker = meetingMediaInterfaces->speaker;
#endif

    if (!createMediaPlayersForAdapters(httpContentFetcherFactory, equalizerRuntimeSetup)) {
        ACSDK_CRITICAL(LX("Could not create mediaPlayers for adapters"));
        return false;
    }

    auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();

    // Creating the alert storage object to be used for rendering and storing alerts.
    auto alertStorage =
        alexaClientSDK::acsdkAlerts::storage::SQLiteAlertStorage::create(config, audioFactory->alerts());

    // Creating the message storage object to be used for storing message to be sent later.
    auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);

    /*
     * Creating notifications storage object to be used for storing notification indicators.
     */
    auto notificationsStorage = alexaClientSDK::acsdkNotifications::SQLiteNotificationsStorage::create(config);

    /*
     * Creating new device settings storage object to be used for storing AVS Settings.
     */
    auto deviceSettingsStorage = alexaClientSDK::settings::storage::SQLiteDeviceSettingStorage::create(config);

    /*
     * Creating bluetooth storage object to be used for storing uuid to mac mappings for devices.
     */
    auto bluetoothStorage = alexaClientSDK::acsdkBluetooth::SQLiteBluetoothStorage::create(config);

    /*
     * Create sample locale asset manager.
     */
    auto localeAssetsManager = manufactory->get<std::shared_ptr<LocaleAssetsManagerInterface>>();
    if (!localeAssetsManager) {
        ACSDK_CRITICAL(LX("Failed to create Locale Assets Manager!"));
        return false;
    }

    std::string APLVersion;
    std::string websocketInterface;
    sampleAppConfig.getString(WEBSOCKET_INTERFACE_KEY, &websocketInterface, DEFAULT_WEBSOCKET_INTERFACE);

    int websocketPortNumber;
    sampleAppConfig.getInt(WEBSOCKET_PORT_KEY, &websocketPortNumber, DEFAULT_WEBSOCKET_PORT);

    // Create the websocket server that handles communications with websocket clients

#ifdef UWP_BUILD
    auto webSocketServer = std::make_shared<NullSocketServer>();
#else
    auto webSocketServer = std::make_shared<communication::WebSocketServer>(websocketInterface, websocketPortNumber);

#ifdef ENABLE_WEBSOCKET_SSL
    std::string sslCaFile;
    sampleAppConfig.getString(WEBSOCKET_CERTIFICATE_AUTHORITY, &sslCaFile);

    std::string sslCertificateFile;
    sampleAppConfig.getString(WEBSOCKET_CERTIFICATE, &sslCertificateFile);

    std::string sslPrivateKeyFile;
    sampleAppConfig.getString(WEBSOCKET_PRIVATE_KEY, &sslPrivateKeyFile);

    webSocketServer->setCertificateFile(sslCaFile, sslCertificateFile, sslPrivateKeyFile);
#endif  // ENABLE_WEBSOCKET_SSL

#endif  // UWP_BUILD

    /*
     * Creating customerDataManager which will be used by the registrationManager and all classes that extend
     * CustomerDataHandler
     */
    auto customerDataManager = manufactory->get<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>();
    if (!customerDataManager) {
        ACSDK_CRITICAL(LX("Failed to get CustomerDataManager!"));
        return false;
    }

    m_guiClient = gui::GUIClient::create(webSocketServer, miscStorage, customerDataManager);

    if (!m_guiClient) {
        ACSDK_CRITICAL(LX("Creation of GUIClient failed!"));
        return false;
    }

    std::string cachePeriodInSeconds;
    std::string maxCacheSize;

    sampleAppConfig.getString(
        CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY,
        &cachePeriodInSeconds,
        DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS);
    sampleAppConfig.getString(CONTENT_CACHE_MAX_SIZE_KEY, &maxCacheSize, DEFAULT_CONTENT_CACHE_MAX_SIZE);

    auto contentDownloadManager = std::make_shared<CachingDownloadManager>(
        httpContentFetcherFactory,
        std::stol(cachePeriodInSeconds),
        std::stol(maxCacheSize),
        miscStorage,
        customerDataManager);

    int maxNumberOfConcurrentDownloads;
    sampleAppConfig.getInt(
        MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY,
        &maxNumberOfConcurrentDownloads,
        DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD);

    if (1 > maxNumberOfConcurrentDownloads) {
        maxNumberOfConcurrentDownloads = DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD;
        ACSDK_ERROR(LX("Invalid values for maxNumberOfConcurrentDownloads"));
    }

    auto parameters = AplClientBridgeParameter{maxNumberOfConcurrentDownloads};
    m_aplClientBridge = AplClientBridge::create(contentDownloadManager, m_guiClient, parameters);

    m_guiClient->setAplClientBridge(m_aplClientBridge);

    /*
     * Create the presentation layer for the captions.
     */
    auto captionPresenter = std::make_shared<alexaSmartScreenSDK::sampleApp::SmartScreenCaptionPresenter>(m_guiClient);

#ifdef ENABLE_PCC
    auto phoneCaller = std::make_shared<alexaClientSDK::sampleApp::PhoneCaller>();
#endif

#ifdef ENABLE_MCC
    auto meetingClient = std::make_shared<alexaClientSDK::sampleApp::MeetingClient>();
    auto calendarClient = std::make_shared<alexaClientSDK::sampleApp::CalendarClient>();
#endif

    /*
     * Creating the deviceInfo object
     */
    auto deviceInfo = manufactory->get<std::shared_ptr<avsCommon::utils::DeviceInfo>>();
    if (!deviceInfo) {
        ACSDK_CRITICAL(LX("Creation of DeviceInfo failed!"));
        return false;
    }

    // Creating the UI component that observes various components and prints to the console accordingly.

    auto userInterfaceManager = std::make_shared<alexaSmartScreenSDK::sampleApp::JsonUIManager>(
        std::static_pointer_cast<alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIClientInterface>(m_guiClient),
        deviceInfo);
    m_guiClient->setObserver(userInterfaceManager);

    APLVersion = m_guiClient->getMaxAPLVersion();

    /*
     * Supply a SALT for UUID generation, this should be as unique to each individual device as possible
     */
    alexaClientSDK::avsCommon::utils::uuidGeneration::setSalt(
        deviceInfo->getClientId() + deviceInfo->getDeviceSerialNumber());

    // Creating the AuthDelegate - this component takes care of LWA and authorization of the client.
    auto authDelegateStorage = authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::create(config);
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate =
        authorization::cblAuthDelegate::CBLAuthDelegate::create(
            config, customerDataManager, std::move(authDelegateStorage), userInterfaceManager, nullptr, deviceInfo);

    if (!authDelegate) {
        ACSDK_CRITICAL(LX("Creation of AuthDelegate failed!"));
        return false;
    }

    /*
     * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
     * Capabilities API.
     */
    auto capabilitiesDelegateStorage =
        alexaClientSDK::capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::create(config);

    m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
        authDelegate, std::move(capabilitiesDelegateStorage), customerDataManager);

    if (!m_capabilitiesDelegate) {
        ACSDK_CRITICAL(LX("Creation of CapabilitiesDelegate failed!"));
        return false;
    }

    m_shutdownRequiredList.push_back(m_capabilitiesDelegate);
    authDelegate->addAuthObserver(userInterfaceManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);

    // INVALID_FIRMWARE_VERSION is passed to @c getInt() as a default in case FIRMWARE_VERSION_KEY is not found.
    int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
    sampleAppConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

    /*
     * Creating the InternetConnectionMonitor that will notify observers of internet connection status changes.
     */
    auto internetConnectionMonitor =
        avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
    if (!internetConnectionMonitor) {
        ACSDK_CRITICAL(LX("Failed to create InternetConnectionMonitor"));
        return false;
    }

    /*
     * Creating the Context Manager - This component manages the context of each of the components to update to AVS.
     * It is required for each of the capability agents so that they may provide their state just before any event is
     * fired off.
     */
    auto contextManager = manufactory->get<std::shared_ptr<ContextManagerInterface>>();
    if (!contextManager) {
        ACSDK_CRITICAL(LX("Creation of ContextManager failed."));
        return false;
    }

    auto avsGatewayManagerStorage = avsGatewayManager::storage::AVSGatewayManagerStorage::create(miscStorage);
    if (!avsGatewayManagerStorage) {
        ACSDK_CRITICAL(LX("Creation of AVSGatewayManagerStorage failed"));
        return false;
    }
    auto avsGatewayManager =
        avsGatewayManager::AVSGatewayManager::create(std::move(avsGatewayManagerStorage), customerDataManager, config);
    if (!avsGatewayManager) {
        ACSDK_CRITICAL(LX("Creation of AVSGatewayManager failed"));
        return false;
    }

    auto synchronizeStateSenderFactory = synchronizeStateSender::SynchronizeStateSenderFactory::create(contextManager);
    if (!synchronizeStateSenderFactory) {
        ACSDK_CRITICAL(LX("Creation of SynchronizeStateSenderFactory failed"));
        return false;
    }

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>> providers;
    providers.push_back(synchronizeStateSenderFactory);
    providers.push_back(avsGatewayManager);
    providers.push_back(m_capabilitiesDelegate);

    /*
     * Create a factory for creating objects that handle tasks that need to be performed right after establishing
     * a connection to AVS.
     */
    auto postConnectSequencerFactory = acl::PostConnectSequencerFactory::create(providers);

    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::ProtocolTracerInterface> deviceProtocolTracer;
    if (diagnostics) {
        /*
         * Create the deviceProtocolTracer to trace events and directives.
         */
        deviceProtocolTracer = diagnostics->getProtocolTracer();

        if (deviceProtocolTracer) {
            const std::string DIAGNOSTICS_KEY = "diagnostics";
            const std::string MAX_TRACED_MESSAGES_KEY = "maxTracedMessages";
            const std::string TRACE_FROM_STARTUP = "protocolTraceFromStartup";

            int configMaxValue = -1;
            bool configProtocolTraceEnabled = false;

            if (alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[DIAGNOSTICS_KEY].getInt(
                    MAX_TRACED_MESSAGES_KEY, &configMaxValue)) {
                if (configMaxValue < 0) {
                    ACSDK_WARN(LX("ignoringMaxTracedMessages")
                                   .d("reason", "negativeValue")
                                   .d("maxTracedMessages", configMaxValue));
                } else {
                    deviceProtocolTracer->setMaxMessages(static_cast<unsigned int>(configMaxValue));
                }
            }

            if (alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[DIAGNOSTICS_KEY].getBool(
                    TRACE_FROM_STARTUP, &configProtocolTraceEnabled)) {
                if (configProtocolTraceEnabled) {
                    deviceProtocolTracer->setProtocolTraceFlag(configProtocolTraceEnabled);
                    ACSDK_DEBUG9(LX("Protocol Trace has been enabled at startup"));
                }
            }
        }
    }

    /*
     * Create a factory to create objects that establish a connection with AVS.
     */
    auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(
        std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
        postConnectSequencerFactory,
        nullptr,
        deviceProtocolTracer);

    /*
     * Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
     */
    size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
        BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
    auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
        alexaClientSDK::avsCommon::avs::AudioInputStream::create(buffer, WORD_SIZE, MAX_READERS);

    if (!sharedDataStream) {
        ACSDK_CRITICAL(LX("Failed to create shared data stream!"));
        return false;
    }

    /*
     * Create the BluetoothDeviceManager to communicate with the Bluetooth stack.
     */
    std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager;

    /*
     * Create the connectionRules to communicate with the Bluetooth stack.
     */
    std::unordered_set<
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        enabledConnectionRules;
    enabledConnectionRules.insert(alexaClientSDK::acsdkBluetooth::BasicDeviceConnectionRule::create());

#ifdef BLUETOOTH_BLUEZ
    auto eventBus = std::make_shared<avsCommon::utils::bluetooth::BluetoothEventBus>();

#ifdef BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS
    /*
     * Create PulseAudio initializer object. Subscribe to BLUETOOTH_DEVICE_MANAGER_INITIALIZED event before we create
     * the BT Device Manager, otherwise may miss it.
     */
    m_pulseAudioInitializer = bluetoothImplementations::blueZ::PulseAudioBluetoothInitializer::create(eventBus);
#endif

    bluetoothDeviceManager = bluetoothImplementations::blueZ::BlueZBluetoothDeviceManager::create(eventBus);
#endif

    alexaClientSDK::avsCommon::utils::AudioFormat compatibleAudioFormat;
    compatibleAudioFormat.sampleRateHz = SAMPLE_RATE_HZ;
    compatibleAudioFormat.sampleSizeInBits = WORD_SIZE * CHAR_BIT;
    compatibleAudioFormat.numChannels = NUM_CHANNELS;
    compatibleAudioFormat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
    compatibleAudioFormat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;
    compatibleAudioFormat.dataSigned = false;

    /*
     * Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
     * of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
     * stream is used since this sample application will only have one microphone.
     */

    // Creating tap to talk audio provider
    bool tapAlwaysReadable = true;
    bool tapCanOverride = true;
    bool tapCanBeOverridden = true;

    alexaClientSDK::capabilityAgents::aip::AudioProvider tapToTalkAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        tapAlwaysReadable,
        tapCanOverride,
        tapCanBeOverridden);

    // Creating hold to talk audio provider
    bool holdAlwaysReadable = false;
    bool holdCanOverride = true;
    bool holdCanBeOverridden = false;

    alexaClientSDK::capabilityAgents::aip::AudioProvider holdToTalkAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK,
        holdAlwaysReadable,
        holdCanOverride,
        holdCanBeOverridden);

    // Creating wake word audio provider, if necessary
#ifdef KWD
    bool wakeAlwaysReadable = true;
    bool wakeCanOverride = false;
    bool wakeCanBeOverridden = true;

    alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        wakeAlwaysReadable,
        wakeCanOverride,
        wakeCanBeOverridden);
#endif  // KWD

#ifdef PORTAUDIO
    std::shared_ptr<PortAudioMicrophoneWrapper> micWrapper = PortAudioMicrophoneWrapper::create(sharedDataStream);
#elif defined(ANDROID_MICROPHONE)
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESMicrophone> micWrapper =
        m_openSlEngine->createAndroidMicrophone(sharedDataStream);
#elif AUDIO_INJECTION
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface> audioInjector;
    if (diagnostics) {
        audioInjector = diagnostics->getAudioInjector();
    }

    if (!audioInjector) {
        ACSDK_CRITICAL(LX("No audio injector provided!"));
        return false;
    }
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper =
        audioInjector->getMicrophone(sharedDataStream, compatibleAudioFormat);
#elif defined(UWP_BUILD)
    std::shared_ptr<alexaSmartScreenSDK::sssdkCommon::NullMicrophone> micWrapper =
        std::make_shared<alexaSmartScreenSDK::sssdkCommon::NullMicrophone>(sharedDataStream);
#else
    ACSDK_CRITICAL(LX("No microphone module provided!"));
    return false;
#endif
    if (!micWrapper) {
        ACSDK_CRITICAL(LX("Failed to create microphone wrapper!"));
        return false;
    }

#ifdef KWD
    // If wake word is enabled, then creating the GUI manager with a wake word audio provider.
    m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
        m_guiClient,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        micWrapper,
        wakeWordAudioProvider);
#else
    // If wake word is not enabled, then creating the gui manager without a wake word audio provider.
    m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
        m_guiClient,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        micWrapper,
        alexaClientSDK::capabilityAgents::aip::AudioProvider::null());
#endif  // KWD

    auto metricRecorder = manufactory->get<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>();

    /*
     * Creating the SmartScreenClient - this component serves as an out-of-box default object that instantiates and
     * "glues" together all the modules.
     */
    std::shared_ptr<smartScreenClient::SmartScreenClient> client = smartScreenClient::SmartScreenClient::create(
        deviceInfo,
        customerDataManager,
        m_externalMusicProviderMediaPlayersMap,
        m_externalMusicProviderSpeakersMap,
        m_adapterToCreateFuncMap,
        m_speakMediaPlayer,
        std::move(audioMediaPlayerFactory),
        m_alertsMediaPlayer,
        m_notificationsMediaPlayer,
        m_bluetoothMediaPlayer,
        m_ringtoneMediaPlayer,
        m_systemSoundMediaPlayer,
        speakerMediaInterfaces->speaker,
        audioSpeakers,
        alertsMediaInterfaces->speaker,
        notificationMediaInterfaces->speaker,
        bluetoothMediaInterfaces->speaker,
        ringtoneMediaInterfaces->speaker,
        systemSoundMediaInterfaces->speaker,
        {},
#ifdef ENABLE_PCC
        phoneSpeaker,
        phoneCaller,
#endif
#ifdef ENABLE_MCC
        meetingSpeaker,
        meetingClient,
        calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
        m_commsMediaPlayer,
        commsSpeaker,
        sharedDataStream,
#endif
        equalizerRuntimeSetup,
        audioFactory,
        authDelegate,
        std::move(alertStorage),
        std::move(messageStorage),
        std::move(notificationsStorage),
        std::move(deviceSettingsStorage),
        std::move(bluetoothStorage),
        miscStorage,
        {userInterfaceManager},
        {userInterfaceManager},
        std::move(internetConnectionMonitor),
        m_capabilitiesDelegate,
        contextManager,
        transportFactory,
        avsGatewayManager,
        localeAssetsManager,
        enabledConnectionRules,
        /* systemTimezone*/ nullptr,
        firmwareVersion,
        true,
        nullptr,
        std::move(bluetoothDeviceManager),
        metricRecorder,
#ifdef ENABLE_LPM
        std::make_shared<avsCommon::utils::power::NoOpPowerResourceManager>(),
#else
        nullptr,
#endif
        diagnostics,
        std::make_shared<ExternalCapabilitiesBuilder>(deviceInfo),
        std::make_shared<alexaClientSDK::capabilityAgents::speakerManager::DefaultChannelVolumeFactory>(),
        true,
        std::make_shared<alexaClientSDK::acl::MessageRouterFactory>(),
        nullptr,
        capabilityAgents::aip::AudioProvider::null(),
        m_guiManager,
        APLVersion);
    if (!client) {
        ACSDK_CRITICAL(LX("Failed to create default SDK client!"));
        return false;
    }

    client->addSpeakerManagerObserver(userInterfaceManager);

    client->addNotificationsObserver(userInterfaceManager);

    client->addTemplateRuntimeObserver(m_guiManager);
    client->addAlexaPresentationObserver(m_guiManager);
    client->addAlexaDialogStateObserver(m_guiManager);
    client->addAudioPlayerObserver(m_guiManager);
    client->addAudioPlayerObserver(m_aplClientBridge);
    client->addCallStateObserver(m_guiManager);
    client->addFocusManagersObserver(m_guiManager);
    client->addAudioInputProcessorObserver(m_guiManager);
    m_guiManager->setClient(client);
    m_guiClient->setGUIManager(m_guiManager);

#ifdef KWD
    // This observer is notified any time a keyword is detected and notifies the DefaultClient to start recognizing.
    auto keywordObserver = std::make_shared<KeywordObserver>(client, wakeWordAudioProvider);

    m_keywordDetector = alexaClientSDK::kwd::KeywordDetectorProvider::create(
        sharedDataStream,
        compatibleAudioFormat,
        {keywordObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder);
    if (!m_keywordDetector) {
        ACSDK_CRITICAL(LX("Failed to create keyword detector!"));
    }
#endif

#ifdef ENABLE_CAPTIONS
    std::vector<std::shared_ptr<MediaPlayerInterface>> captionableMediaSources = m_audioMediaPlayerPool;
    captionableMediaSources.emplace_back(m_speakMediaPlayer);
    client->addCaptionPresenter(captionPresenter);
#ifndef UWP_BUILD
    client->setCaptionMediaPlayers(captionableMediaSources);
#endif
#endif

#ifdef ENABLE_ENDPOINT_CONTROLLERS
    // Default Endpoint
    if (!addControllersToDefaultEndpoint(client->getDefaultEndpointBuilder())) {
        ACSDK_CRITICAL(LX("Failed to add controllers to default endpoint!"));
        return false;
    }

    // Peripheral Endpoint
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> peripheralEndpointBuilder =
        client->createEndpointBuilder();
    if (!peripheralEndpointBuilder) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint Builder!"));
        return false;
    }

    peripheralEndpointBuilder->withDerivedEndpointId(PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID)
        .withDescription(PERIPHERAL_ENDPOINT_DESCRIPTION)
        .withFriendlyName(PERIPHERAL_ENDPOINT_FRIENDLYNAME)
        .withManufacturerName(PERIPHERAL_ENDPOINT_MANUFACTURER_NAME)
        .withAdditionalAttributes(
            PERIPHERAL_ENDPOINT_MANUFACTURER_NAME,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER)
        .withDisplayCategory(PERIPHERAL_ENDPOINT_DISPLAYCATEGORY);

    if (!addControllersToPeripheralEndpoint(peripheralEndpointBuilder)) {
        ACSDK_CRITICAL(LX("Failed to add controllers to peripheral endpoint!"));
        return false;
    }

    auto peripheralEndpoint = peripheralEndpointBuilder->build();
    if (!peripheralEndpoint) {
        ACSDK_CRITICAL(LX("Failed to create Peripheral Endpoint!"));
        return false;
    }

    client->registerEndpoint(std::move(peripheralEndpoint));
#endif

#ifdef ENABLE_REVOKE_AUTH
    // Creating the revoke authorization observer.
    auto revokeObserver =
        std::make_shared<alexaSmartScreenSDK::sampleApp::RevokeAuthorizationObserver>(client->getRegistrationManager());
    client->addRevokeAuthorizationObserver(revokeObserver);
#endif  // ENABLE_REVOKE_AUTH

    authDelegate->addAuthObserver(m_guiClient);
    client->addRegistrationObserver(m_guiClient);
    m_capabilitiesDelegate->addCapabilitiesObserver(m_guiClient);

    m_guiManager->setDoNotDisturbSettingObserver(m_guiClient);
    m_guiManager->configureSettingsNotifications();

    if (!m_guiClient->start()) {
        return false;
    }

    client->connect();

    return true;
}

std::shared_ptr<ApplicationMediaInterfaces> SampleApplication::createApplicationMediaPlayer(
    const std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>&
        httpContentFetcherFactory,
    bool enableEqualizer,
    const std::string& name,
    bool enableLiveMode) {
#ifdef GSTREAMER_MEDIA_PLAYER
    /*
     * For the SDK, the MediaPlayer happens to also provide volume control functionality.
     * Note the externalMusicProviderMediaPlayer is not added to the set of SpeakerInterfaces as there would be
     * more actions needed for these beyond setting the volume control on the MediaPlayer.
     */
    auto mediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create(
        httpContentFetcherFactory, enableEqualizer, name, enableLiveMode);
    if (!mediaPlayer) {
        return nullptr;
    }
    auto speaker = std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(mediaPlayer);
    auto equalizer =
        std::static_pointer_cast<alexaClientSDK::acsdkEqualizerInterfaces::EqualizerInterface>(mediaPlayer);
    auto requiresShutdown = std::static_pointer_cast<alexaClientSDK::avsCommon::utils::RequiresShutdown>(mediaPlayer);
    auto applicationMediaInterfaces =
        std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, equalizer, requiresShutdown);
#elif defined(ANDROID_MEDIA_PLAYER)
    // TODO - Add support of live mode to AndroidSLESMediaPlayer (ACSDK-2530).
    std::shared_ptr<mediaPlayer::android::AndroidSLESMediaPlayer> mediaPlayer =
        mediaPlayer::android::AndroidSLESMediaPlayer::create(
            httpContentFetcherFactory,
            m_openSlEngine,
            enableEqualizer,
            mediaPlayer::android::PlaybackConfiguration(),
            name);
    if (!mediaPlayer) {
        return nullptr;
    }
    auto speaker = mediaPlayer->getSpeaker();
    auto equalizer =
        std::static_pointer_cast<alexaClientSDK::acsdkEqualizerInterfaces::EqualizerInterface>(mediaPlayer);
    auto requiresShutdown = std::static_pointer_cast<alexaClientSDK::avsCommon::utils::RequiresShutdown>(mediaPlayer);
    auto applicationMediaInterfaces =
        std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, equalizer, requiresShutdown);
#elif defined(CUSTOM_MEDIA_PLAYER)
    // Custom media players must implement the createCustomMediaPlayer function
    auto applicationMediaInterfaces =
        createCustomMediaPlayer(httpContentFetcherFactory, enableEqualizer, name, enableLiveMode);
    if (!applicationMediaInterfaces) {
        return nullptr;
    }
#elif defined(UWP_BUILD)
    auto mediaPlayer = std::make_shared<alexaSmartScreenSDK::sssdkCommon::TestMediaPlayer>();
    auto nullEqualizer = std::make_shared<alexaSmartScreenSDK::sssdkCommon::NullEqualizer>();

    auto speaker = std::make_shared<alexaSmartScreenSDK::sssdkCommon::NullMediaSpeaker>();
    auto requiresShutdown = std::static_pointer_cast<alexaClientSDK::avsCommon::utils::RequiresShutdown>(mediaPlayer);

    return std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, nullEqualizer, requiresShutdown);
#endif

#ifndef UWP_BUILD
    if (applicationMediaInterfaces->requiresShutdown) {
        m_shutdownRequiredList.push_back(applicationMediaInterfaces->requiresShutdown);
    }
    return applicationMediaInterfaces;
#endif
}

#ifdef ENABLE_ENDPOINT_CONTROLLERS
bool SampleApplication::addControllersToDefaultEndpoint(
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> defaultEndpointBuilder) {
    if (!defaultEndpointBuilder) {
        ACSDK_CRITICAL(LX("addControllersToDefaultEndpointFailed").m("invalidDefaultEndpointBuilder"));
        return false;
    }

#ifdef TOGGLE_CONTROLLER
    auto toggleHandler =
        DefaultEndpointToggleControllerHandler::create(DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME);
    if (!toggleHandler) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint toggle controller handler!"));
        return false;
    }

    auto toggleControllerAttributes = buildToggleControllerAttributes(
        {{{FriendlyName::Type::TEXT, DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME}}});
    if (!toggleControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint toggle controller attributes!"));
        return false;
    }

    defaultEndpointBuilder->withToggleController(
        toggleHandler,
        DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME,
        toggleControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef RANGE_CONTROLLER
    auto rangeHandler = DefaultEndpointRangeControllerHandler::create(DEFAULT_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME);
    if (!rangeHandler) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint range controller handler!"));
        return false;
    }

    auto rangeControllerAttributes = buildRangeControllerAttributes(
        {{{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_SETTING_FANSPEED}}},
        {{DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MAXIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_HIGH}}},
         {DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MEDIUM}}},
         {DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MINIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_LOW}}}});

    if (!rangeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint range controller attributes!"));
        return false;
    }

    defaultEndpointBuilder->withRangeController(
        rangeHandler,
        DEFAULT_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME,
        rangeControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef MODE_CONTROLLER
    auto modeHandler = DefaultEndpointModeControllerHandler::create(DEFAULT_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME);
    if (!modeHandler) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint mode controller handler!"));
        return false;
    }

    auto modeControllerAttributes = buildModeControllerAttributes(
        {{{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_SETTING_MODE}}},
        {{DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_FAN_ONLY,
          {{FriendlyName::Type::TEXT,
            DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_FAN_ONLY_FRIENDLY_NAME}}},
         {DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_HEAT,
          {{FriendlyName::Type::TEXT, DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_HEAT_FRIENDLY_NAME}}},
         {DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_COOL,
          {{FriendlyName::Type::TEXT,
            DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_COOL_FRIENDLY_NAME}}}});

    if (!modeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint mode controller attributes!"));
        return false;
    }

    defaultEndpointBuilder->withModeController(
        modeHandler,
        DEFAULT_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME,
        modeControllerAttributes.value(),
        true,
        true,
        false);
#endif

    return true;
}

bool SampleApplication::addControllersToPeripheralEndpoint(
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> peripheralEndpointBuilder) {
    if (!peripheralEndpointBuilder) {
        ACSDK_CRITICAL(LX("addControllersToPeripheralEndpointFailed").m("invalidPeripheralEndpointBuilder"));
        return false;
    }

#ifdef POWER_CONTROLLER
    m_peripheralEndpointPowerHandler =
        PeripheralEndpointPowerControllerHandler::create(PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID);
    if (!m_peripheralEndpointPowerHandler) {
        ACSDK_CRITICAL(LX("Failed to create power controller handler!"));
        return false;
    }
    peripheralEndpointBuilder->withPowerController(m_peripheralEndpointPowerHandler, true, true);
#endif

#ifdef TOGGLE_CONTROLLER
    m_peripheralEndpointToggleHandler = PeripheralEndpointToggleControllerHandler::create(
        PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID, PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME);
    if (!m_peripheralEndpointToggleHandler) {
        ACSDK_CRITICAL(LX("Failed to create toggle controller handler!"));
        return false;
    }

    auto peripheralEndpointToggleControllerAttributes = buildToggleControllerAttributes(
        {{{FriendlyName::Type::TEXT, PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME}}});
    if (!peripheralEndpointToggleControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint toggle controller attributes!"));
        return false;
    }

    peripheralEndpointBuilder->withToggleController(
        m_peripheralEndpointToggleHandler,
        PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME,
        peripheralEndpointToggleControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef RANGE_CONTROLLER
    m_peripheralEndpointRangeHandler = PeripheralEndpointRangeControllerHandler::create(
        PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID, PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME);
    if (!m_peripheralEndpointRangeHandler) {
        ACSDK_CRITICAL(LX("Failed to create range controller handler!"));
        return false;
    }

    // Enables "raise" and "lower" utterances for the peripheral endpoint by mapping actions to directives.
    auto raiseActionMapping = capabilitySemantics::ActionsToDirectiveMapping();
    raiseActionMapping.addAction(SEMANTICS_ACTION_ID_RAISE);
    raiseActionMapping.setDirective(SETRANGE_DIRECTIVE_NAME, PERIPHERAL_ENDPOINT_RAISE_PAYLOAD);

    auto lowerActionMapping = capabilitySemantics::ActionsToDirectiveMapping();
    lowerActionMapping.addAction(SEMANTICS_ACTION_ID_LOWER);
    lowerActionMapping.setDirective(SETRANGE_DIRECTIVE_NAME, PERIPHERAL_ENDPOINT_LOWER_PAYLOAD);

    auto peripheralEndpointRangeSemantics = capabilitySemantics::CapabilitySemantics();
    peripheralEndpointRangeSemantics.addActionsToDirectiveMapping(raiseActionMapping);
    peripheralEndpointRangeSemantics.addActionsToDirectiveMapping(lowerActionMapping);
    if (!peripheralEndpointRangeSemantics.isValid()) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint semantic annotations!"));
        return false;
    }

    auto peripheralEndpointRangeControllerAttributes = buildRangeControllerAttributes(
        {{{FriendlyName::Type::TEXT, PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_FRIENDLY_NAME}}},
        {{PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MAXIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_HIGH}}},
         {PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MEDIUM}}},
         {PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MINIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_LOW}}}},
        utils::Optional<capabilitySemantics::CapabilitySemantics>(peripheralEndpointRangeSemantics));

    if (!peripheralEndpointRangeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint range controller attributes!"));
        return false;
    }

    peripheralEndpointBuilder->withRangeController(
        m_peripheralEndpointRangeHandler,
        PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME,
        peripheralEndpointRangeControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef MODE_CONTROLLER
    m_peripheralEndpointModeHandler = PeripheralEndpointModeControllerHandler::create(
        PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID, PERIPHERAL_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME);
    if (!m_peripheralEndpointModeHandler) {
        ACSDK_CRITICAL(LX("Failed to create mode controller handler!"));
        return false;
    }

    auto peripheralEndpointModeControllerAttributes = buildModeControllerAttributes(
        {{{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_SETTING_MODE},
          {FriendlyName::Type::TEXT, PERIPHERAL_ENDPOINT_MODE_CONTROLLER_FRIENDLY_NAME}}},
        {{PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_RED,
          {{FriendlyName::Type::TEXT, PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_RED}}},
         {PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_GREEN,
          {{FriendlyName::Type::TEXT, PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_GREEN}}},
         {PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_BLUE,
          {{FriendlyName::Type::TEXT, PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_BLUE}}}});

    if (!peripheralEndpointModeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint mode controller attributes!"));
        return false;
    }

    peripheralEndpointBuilder->withModeController(
        m_peripheralEndpointModeHandler,
        PERIPHERAL_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME,
        peripheralEndpointModeControllerAttributes.value(),
        true,
        true,
        false);
#endif

    return true;
}
#endif  // ENABLE_ENDPOINT_CONTROLLERS

std::string SampleApplication::decodeHexToAscii(const std::string hexString) {
    std::string asciiString(hexString.size() / 2, '\0');
    std::string byte(2, '\0');
    for (size_t i = 0; i < hexString.size() - 1; i += 2) {
        byte[0] = hexString[i];
        byte[1] = hexString[i + 1];
        asciiString[i / 2] = static_cast<char>(std::stoi(byte, nullptr, 16));
    }

    return asciiString;
}

}  // namespace sampleApp
}  // namespace alexaSmartScreenSDK
