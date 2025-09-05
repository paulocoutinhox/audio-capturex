#include "audio_capture.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace AudioCaptureX
{

AudioCapture::AudioCapture(AudioDataCallback callback)
    : context(nullptr)
    , stream(nullptr)
    , inputDeviceId(nullptr)
    , callback(std::move(callback))
    , capturing(false)
    , shouldStop(false)
    , sampleRate(0)
    , channelCount(0)
    , inputDeviceIndex(-1)
    , initialized(false)
    , outputFile("captured-audio.wav")
{
    if (!initializeCubeb())
    {
        std::cerr << "Failed to initialize audio system" << std::endl;
    }
}

AudioCapture::~AudioCapture()
{
    stopCapture();
    cleanup();
}

AudioCapture::AudioCapture(AudioCapture &&other) noexcept
    : context(other.context)
    , stream(other.stream)
    , inputDeviceId(other.inputDeviceId)
    , callback(std::move(other.callback))
    , capturing(other.capturing.load())
    , shouldStop(other.shouldStop.load())
    , captureThreadHandle(std::move(other.captureThreadHandle))
    , sampleRate(other.sampleRate.load())
    , channelCount(other.channelCount.load())
    , currentDeviceName(std::move(other.currentDeviceName))
    , inputDeviceIndex(other.inputDeviceIndex)
    , initialized(other.initialized)
    , recordedAudio(std::move(other.recordedAudio))
    , outputFile(std::move(other.outputFile))
{
    other.context = nullptr;
    other.stream = nullptr;
    other.inputDeviceId = nullptr;
    other.initialized = false;
}

AudioCapture &AudioCapture::operator=(AudioCapture &&other) noexcept
{
    if (this != &other)
    {
        stopCapture();
        cleanup();

        context = other.context;
        stream = other.stream;
        inputDeviceId = other.inputDeviceId;
        callback = std::move(other.callback);
        capturing = other.capturing.load();
        shouldStop = other.shouldStop.load();
        captureThreadHandle = std::move(other.captureThreadHandle);
        sampleRate = other.sampleRate.load();
        channelCount = other.channelCount.load();
        currentDeviceName = std::move(other.currentDeviceName);
        inputDeviceIndex = other.inputDeviceIndex;
        initialized = other.initialized;
        recordedAudio = std::move(other.recordedAudio);
        outputFile = std::move(other.outputFile);

        other.context = nullptr;
        other.stream = nullptr;
        other.inputDeviceId = nullptr;
        other.initialized = false;
    }
    return *this;
}

bool AudioCapture::initializeCubeb()
{
    int r = cubeb_init(&context, "AudioCaptureX", nullptr);
    if (r != CUBEB_OK)
    {
        std::cerr << "Error initializing cubeb: " << r << std::endl;
        return false;
    }

    // Get default input device
    cubeb_device_collection collection;
    r = cubeb_enumerate_devices(context, CUBEB_DEVICE_TYPE_INPUT, &collection);
    if (r != CUBEB_OK)
    {
        std::cerr << "Error enumerating devices: " << r << std::endl;
        cubeb_destroy(context);
        context = nullptr;
        return false;
    }

    if (collection.count == 0)
    {
        std::cerr << "No input devices found" << std::endl;
        cubeb_device_collection_destroy(context, &collection);
        cubeb_destroy(context);
        context = nullptr;
        return false;
    }

    // Use first available device as default
    inputDeviceId = collection.device[0].devid;
    currentDeviceName = collection.device[0].friendly_name ? collection.device[0].friendly_name : "Unknown Device";
    inputDeviceIndex = 0;

    cubeb_device_collection_destroy(context, &collection);
    initialized = true;

    return true;
}

void AudioCapture::cleanup()
{
    if (stream)
    {
        cubeb_stream_stop(stream);
        cubeb_stream_destroy(stream);
        stream = nullptr;
    }

    if (context)
    {
        cubeb_destroy(context);
        context = nullptr;
    }

    inputDeviceId = nullptr;
    initialized = false;
}

bool AudioCapture::startCapture(int deviceIndex)
{
    if (!initialized)
    {
        std::cerr << "Audio system not initialized" << std::endl;
        return false;
    }

    if (capturing.load())
    {
        std::cerr << "Capture already running" << std::endl;
        return false;
    }

    // Set device if specified
    if (deviceIndex >= 0)
    {
        if (!setInputDevice(deviceIndex))
        {
            std::cerr << "Failed to set input device: " << deviceIndex << std::endl;
            return false;
        }
    }

    if (!inputDeviceId)
    {
        std::cerr << "No input device available" << std::endl;
        return false;
    }

    // Set up stream parameters
    cubeb_stream_params input_params;
    input_params.format = CUBEB_SAMPLE_FLOAT32LE;
    input_params.rate = 48000; // Default sample rate
    input_params.channels = 2; // Default stereo
    input_params.layout = CUBEB_LAYOUT_UNDEFINED;
    input_params.prefs = CUBEB_STREAM_PREF_NONE;

    uint32_t latency_frames = 4096; // Default latency

    int r = cubeb_stream_init(context, &stream, "AudioCaptureX Input",
                             inputDeviceId, &input_params, nullptr, nullptr,
                             latency_frames, dataCallback, stateCallback, this);

    if (r != CUBEB_OK)
    {
        std::cerr << "Error creating stream: " << r << std::endl;
        return false;
    }

    // Get actual stream parameters
    cubeb_stream_get_latency(stream, &latency_frames);
    sampleRate = input_params.rate;
    channelCount = input_params.channels;

    // Clear previous recording
    recordedAudio.clear();

    // Start the stream
    r = cubeb_stream_start(stream);
    if (r != CUBEB_OK)
    {
        std::cerr << "Error starting stream: " << r << std::endl;
        cubeb_stream_destroy(stream);
        stream = nullptr;
        return false;
    }

    shouldStop = false;
    capturing = true;

    std::cout << "Audio capture started on device: " << currentDeviceName << std::endl;
    std::cout << "Sample rate: " << sampleRate << " Hz, Channels: " << channelCount << std::endl;

    return true;
}

bool AudioCapture::stopCapture()
{
    if (!capturing.load())
    {
        return true; // Already stopped
    }

    std::cout << "Stopping audio capture..." << std::endl;

    // Set stop flag first
    shouldStop = true;

    // Stop the stream
    if (stream)
    {
        int r = cubeb_stream_stop(stream);
        if (r != CUBEB_OK)
        {
            std::cerr << "Error stopping stream: " << r << std::endl;
        }
    }

    // Set capturing to false after everything is stopped
    capturing = false;

    std::cout << "Audio capture stopped" << std::endl;
    return true;
}

bool AudioCapture::isCapturing() const noexcept
{
    return capturing.load();
}

void AudioCapture::setCallback(AudioDataCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex);
    this->callback = std::move(callback);
}

int AudioCapture::getSampleRate() const noexcept
{
    return sampleRate.load();
}

int AudioCapture::getChannelCount() const noexcept
{
    return channelCount.load();
}

std::vector<std::string> AudioCapture::getAvailableInputDevices() const
{
    std::vector<std::string> devices;

    if (!initialized || !context)
    {
        return devices;
    }

    cubeb_device_collection collection;
    int r = cubeb_enumerate_devices(context, CUBEB_DEVICE_TYPE_INPUT, &collection);
    if (r != CUBEB_OK)
    {
        std::cerr << "Error enumerating devices: " << r << std::endl;
        return devices;
    }

    for (size_t i = 0; i < collection.count; ++i)
    {
                const char* name = collection.device[i].friendly_name ?
                          collection.device[i].friendly_name :
                          collection.device[i].device_id;
        if (name)
        {
            devices.emplace_back(name);
        }
    }

    cubeb_device_collection_destroy(context, &collection);
    return devices;
}

bool AudioCapture::setInputDevice(int deviceIndex)
{
    if (!initialized || !context)
    {
        std::cerr << "Audio system not initialized" << std::endl;
        return false;
    }

    if (capturing.load())
    {
        std::cerr << "Cannot change device while capturing" << std::endl;
        return false;
    }

    cubeb_device_collection collection;
    int r = cubeb_enumerate_devices(context, CUBEB_DEVICE_TYPE_INPUT, &collection);
    if (r != CUBEB_OK)
    {
        std::cerr << "Error enumerating devices: " << r << std::endl;
        return false;
    }

    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(collection.count))
    {
        std::cerr << "Invalid device index: " << deviceIndex << std::endl;
        cubeb_device_collection_destroy(context, &collection);
        return false;
    }

    // Set new device
    inputDeviceId = collection.device[deviceIndex].devid;
    inputDeviceIndex = deviceIndex;
    currentDeviceName = collection.device[deviceIndex].friendly_name ?
                       collection.device[deviceIndex].friendly_name :
                       collection.device[deviceIndex].device_id;

    cubeb_device_collection_destroy(context, &collection);

    std::cout << "Input device set to: " << currentDeviceName << std::endl;
    return true;
}

std::string AudioCapture::getCurrentInputDevice() const
{
    return currentDeviceName;
}

long AudioCapture::dataCallback(cubeb_stream *stream, void *user_ptr, const void *input_buffer, void *output_buffer, long nframes)
{
    AudioCapture *capture = static_cast<AudioCapture *>(user_ptr);
    if (!capture || !input_buffer)
    {
        return 0;
    }

    // Convert input buffer to float vector
    const float *input_samples = static_cast<const float *>(input_buffer);
    int sample_count = nframes * capture->channelCount.load();

    std::vector<float> audio_data(input_samples, input_samples + sample_count);

    // Store for recording
    capture->recordedAudio.insert(capture->recordedAudio.end(),
                                 reinterpret_cast<const char*>(input_samples),
                                 reinterpret_cast<const char*>(input_samples) + sample_count * sizeof(float));

    // Call user callback
    capture->onAudioData(audio_data, nframes);

    return nframes;
}

void AudioCapture::stateCallback(cubeb_stream *stream, void *user_ptr, cubeb_state state)
{
    AudioCapture *capture = static_cast<AudioCapture *>(user_ptr);
    if (!capture)
    {
        return;
    }

    switch (state)
    {
        case CUBEB_STATE_STARTED:
            std::cout << "Stream started" << std::endl;
            break;
        case CUBEB_STATE_STOPPED:
            std::cout << "Stream stopped" << std::endl;
            capture->capturing = false;
            break;
        case CUBEB_STATE_DRAINED:
            std::cout << "Stream drained" << std::endl;
            break;
        case CUBEB_STATE_ERROR:
            std::cerr << "Stream error" << std::endl;
            capture->capturing = false;
            break;
        default:
            break;
    }
}

void AudioCapture::onAudioData(const std::vector<float> &audioData, int frameCount)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (callback)
    {
        callback(audioData, frameCount, sampleRate.load(), channelCount.load());
    }
}


void AudioCapture::setOutputFile(const std::string &filename)
{
    outputFile = filename;
}

bool AudioCapture::saveRecordedAudio() const
{
    if (recordedAudio.empty())
    {
        std::cerr << "No audio data to save" << std::endl;
        return false;
    }

    // Create WAV filename
    std::string wavFile = outputFile;
    if (wavFile.substr(wavFile.find_last_of(".") + 1) != "wav")
    {
        wavFile = wavFile.substr(0, wavFile.find_last_of(".")) + ".wav";
    }

    // Convert float32 to 16-bit PCM
    int numChannels = channelCount.load();
    int sampleRate = this->sampleRate.load();
    int numSamples = recordedAudio.size() / sizeof(float);

    std::vector<int16_t> pcmData;
    pcmData.reserve(numSamples);

    const float *floatData = reinterpret_cast<const float *>(recordedAudio.data());
    for (int i = 0; i < numSamples; ++i)
    {
        // Clamp to [-1.0, 1.0] and convert to 16-bit PCM
        float sample = std::max(-1.0f, std::min(1.0f, floatData[i]));
        int16_t pcmSample = static_cast<int16_t>(sample * 32767.0f);
        pcmData.push_back(pcmSample);
    }

    // Use drwav to write WAV file
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = numChannels;
    format.sampleRate = sampleRate;
    format.bitsPerSample = 16;

    drwav wav;
    if (!drwav_init_file_write(&wav, wavFile.c_str(), &format, NULL))
    {
        std::cerr << "Failed to initialize WAV file: " << wavFile << std::endl;
        return false;
    }

    // Write PCM data
    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, pcmData.size() / numChannels, pcmData.data());
    drwav_uninit(&wav);

    if (framesWritten == 0)
    {
        std::cerr << "Failed to write audio data" << std::endl;
        return false;
    }

    std::cout << "WAV audio saved to: " << wavFile << std::endl;
    std::cout << "Recorded " << framesWritten << " frames (" << pcmData.size() << " samples)" << std::endl;
    std::cout << "Format: " << numChannels << " channels, "
              << sampleRate << " Hz, 16 bits PCM" << std::endl;

    return true;
}

} // namespace AudioCaptureX

