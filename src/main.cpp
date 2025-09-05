/**
 * AudioCaptureX Sample Application
 * Simple terminal-based audio capture control
 */

#include "include/audio_capture.hpp"
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <signal.h>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#endif

using namespace AudioCaptureX;

// Global state
std::atomic<bool> running{true};
std::unique_ptr<AudioCapture> currentCapture = nullptr;

// Signal handler
void signalHandler(int signal)
{
    std::cout << "\nShutting down..." << std::endl;
    running = false;

    if (currentCapture)
    {
        currentCapture->stopCapture();
    }
}

#ifdef _WIN32
BOOL WINAPI consoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
    {
        std::cout << "\nShutting down..." << std::endl;
        running = false;

        if (currentCapture)
        {
            currentCapture->stopCapture();
        }

        return TRUE;
    }

    return FALSE;
}
#endif

// Audio callback with level monitoring
void audioCallback(const std::vector<float> &audioData, int frameCount, int sampleRate, int channelCount)
{
    static int callCount = 0;
    callCount++;

    // Calculate levels
    float peak = 0.0f;
    float sum = 0.0f;
    for (float sample : audioData)
    {
        float absSample = std::abs(sample);

        if (absSample > peak)
        {
            peak = absSample;
        }

        sum += sample * sample;
    }

    float rms = std::sqrt(sum / audioData.size());

    // Log every 500 callbacks (reduced frequency)
    if (callCount % 500 == 0)
    {
        std::cout << "Audio #" << callCount << " - Peak: " << peak << ", RMS: " << rms << std::endl;
    }
}

void startCapture()
{
    if (currentCapture && currentCapture->isCapturing())
    {
        std::cout << "Already capturing. Stop first." << std::endl;
        return;
    }

    // Show available devices
    auto tempCapture = std::make_unique<AudioCapture>(audioCallback);
    auto devices = tempCapture->getAvailableInputDevices();

    if (devices.empty())
    {
        std::cout << "No input devices available" << std::endl;
        return;
    }

    std::cout << "Available input devices:" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i)
    {
        std::cout << "  " << i << ": " << devices[i] << std::endl;
    }

    std::cout << "Enter device number (or press Enter for default): ";
    std::string input;
    std::getline(std::cin, input);

    int deviceIndex = -1; // Default device
    if (!input.empty())
    {
        try
        {
            deviceIndex = std::stoi(input);
            if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size()))
            {
                std::cout << "Invalid device number, using default device" << std::endl;
                deviceIndex = -1;
            }
        }
        catch (const std::exception &)
        {
            std::cout << "Invalid input, using default device" << std::endl;
            deviceIndex = -1;
        }
    }

    std::cout << "Starting audio capture..." << std::endl;

    currentCapture = std::make_unique<AudioCapture>(audioCallback);
    currentCapture->setOutputFile("captured-audio.wav");

    if (currentCapture->startCapture(deviceIndex))
    {
        std::cout << "Audio capture started. Type 'stop' to stop and save." << std::endl;
        if (deviceIndex >= 0)
        {
            std::cout << "Using device: " << devices[deviceIndex] << std::endl;
        }
        else
        {
            std::cout << "Using default device: " << currentCapture->getCurrentInputDevice() << std::endl;
        }
    }
    else
    {
        std::cout << "Failed to start audio capture" << std::endl;
        currentCapture = nullptr;
    }
}

void stopCapture()
{
    if (!currentCapture || !currentCapture->isCapturing())
    {
        std::cout << "No capture running" << std::endl;
        return;
    }

    // Stop the capture
    currentCapture->stopCapture();

    // Save recorded audio as WAV file
    if (currentCapture->saveRecordedAudio())
    {
        std::cout << "WAV audio saved successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to save WAV audio file" << std::endl;
    }

    // Clean up
    currentCapture.reset();
    std::cout << "Capture stopped" << std::endl;
}

void listDevices()
{
    AudioCapture capture(nullptr);
    auto devices = capture.getAvailableInputDevices();

    std::cout << "Available audio devices:" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i)
    {
        std::cout << "[" << i << "] " << devices[i] << std::endl;
    }
}

void showStatus()
{
    if (currentCapture && currentCapture->isCapturing())
    {
        std::cout << "Status: Capturing audio" << std::endl;
        std::cout << "Device: " << currentCapture->getCurrentInputDevice() << std::endl;
        std::cout << "Sample Rate: " << currentCapture->getSampleRate() << " Hz" << std::endl;
        std::cout << "Channels: " << currentCapture->getChannelCount() << std::endl;
    }
    else
    {
        std::cout << "Status: Not capturing" << std::endl;
    }
}

void showHelp()
{
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  start    - Start audio capture" << std::endl;
    std::cout << "  stop     - Stop capture and save to file" << std::endl;
    std::cout << "  devices  - List available audio devices" << std::endl;
    std::cout << "  status   - Show current status" << std::endl;
    std::cout << "  help     - Show this help" << std::endl;
    std::cout << "  quit     - Exit program" << std::endl;
}

int main()
{
    // Setup signal handlers
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
#endif

    std::cout << "AudioCaptureX Sample Application" << std::endl;
    std::cout << "Type 'help' for commands or 'quit' to exit" << std::endl;

    std::string command;

    while (running.load())
    {
        std::cout << "\n> ";
        std::getline(std::cin, command);

        if (command == "start")
        {
            startCapture();
        }
        else if (command == "stop")
        {
            stopCapture();
        }
        else if (command == "devices")
        {
            listDevices();
        }
        else if (command == "status")
        {
            showStatus();
        }
        else if (command == "help")
        {
            showHelp();
        }
        else if (command == "quit" || command == "exit")
        {
            break;
        }
        else if (!command.empty())
        {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }

    // Cleanup
    if (currentCapture)
    {
        currentCapture->stopCapture();
        currentCapture.reset();
    }

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
