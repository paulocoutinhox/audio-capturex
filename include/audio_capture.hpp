#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <cubeb/cubeb.h>
#include <thread>
#include <vector>

namespace AudioCaptureX
{

/**
 * @brief Callback function type for audio data
 * @param audioData Vector containing audio samples (interleaved)
 * @param frameCount Number of audio frames
 * @param sampleRate Sample rate in Hz
 * @param channelCount Number of audio channels
 */
using AudioDataCallback = std::function<void(const std::vector<float> &audioData,
                                             int frameCount,
                                             int sampleRate,
                                             int channelCount)>;

/**
 * @brief Audio capture class for cross-platform audio input
 */
class AudioCapture
{
public:
    /**
     * @brief Constructor
     * @param callback Function to call when audio data is available
     */
    explicit AudioCapture(AudioDataCallback callback = nullptr);

    /**
     * @brief Destructor - automatically stops capture if running
     */
    ~AudioCapture();

    // Disable copy constructor and assignment operator
    AudioCapture(const AudioCapture &) = delete;
    AudioCapture &operator=(const AudioCapture &) = delete;

    // Move constructor and assignment operator
    AudioCapture(AudioCapture &&other) noexcept;
    AudioCapture &operator=(AudioCapture &&other) noexcept;

    /**
     * @brief Start audio capture in background thread
     * @param deviceIndex Optional device index (-1 for default device)
     * @return true if capture started successfully, false otherwise
     */
    bool startCapture(int deviceIndex = -1);

    /**
     * @brief Stop audio capture
     * @return true if capture stopped successfully, false otherwise
     */
    bool stopCapture();

    /**
     * @brief Check if capture is currently running
     * @return true if capture is active, false otherwise
     */
    bool isCapturing() const noexcept;

    /**
     * @brief Set the audio data callback
     * @param callback Function to call when audio data is available
     */
    void setCallback(AudioDataCallback callback);

    /**
     * @brief Get current sample rate
     * @return Sample rate in Hz, or 0 if not capturing
     */
    int getSampleRate() const noexcept;

    /**
     * @brief Get current channel count
     * @return Number of channels, or 0 if not capturing
     */
    int getChannelCount() const noexcept;

    /**
     * @brief Get list of available input devices
     * @return Vector of device names
     */
    std::vector<std::string> getAvailableInputDevices() const;

    /**
     * @brief Set specific input device by index
     * @param deviceIndex Index of the device (use getAvailableInputDevices to get list)
     * @return true if device was set successfully, false otherwise
     */
    bool setInputDevice(int deviceIndex);

    /**
     * @brief Get current input device name
     * @return Name of current input device, or empty string if none
     */
    std::string getCurrentInputDevice() const;

    /**
     * @brief Set output file for recording
     * @param filename Output file path
     */
    void setOutputFile(const std::string &filename);

    /**
     * @brief Save recorded audio as WAV file
     * @return true if saved successfully, false otherwise
     */
    bool saveRecordedAudio() const;

private:
    // Internal callback for cubeb
    static long dataCallback(cubeb_stream *stream, void *user_ptr, const void *input_buffer, void *output_buffer, long nframes);
    static void stateCallback(cubeb_stream *stream, void *user_ptr, cubeb_state state);

    // Instance method called by static callback
    void onAudioData(const std::vector<float> &audioData, int frameCount);

    // Initialize cubeb
    bool initializeCubeb();

    // Cleanup resources
    void cleanup();

    // Background thread function
    void captureThread();

    // Member variables
    cubeb *context;
    cubeb_stream *stream;
    cubeb_devid inputDeviceId;

    AudioDataCallback callback;
    std::atomic<bool> capturing;
    std::atomic<bool> shouldStop;

    std::thread captureThreadHandle;
    mutable std::mutex mutex;
    std::condition_variable cv;

    std::atomic<int> sampleRate;
    std::atomic<int> channelCount;
    std::string currentDeviceName;

    int inputDeviceIndex;
    bool initialized;

    // Audio recording
    std::vector<char> recordedAudio;
    std::string outputFile;
};

} // namespace AudioCaptureX
