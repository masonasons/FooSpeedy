/*
 * foo_dsp_speedy - foobar2000 DSP component using Google's Speedy algorithm
 *
 * This component provides audio playback speed manipulation using Google's
 * Speedy library, which is a nonlinear speech speedup algorithm based on
 * the Mach1 algorithm.
 *
 * Copyright 2024
 * Licensed under the Apache License, Version 2.0
 */

#include "pch.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

// Include Speedy/Sonic headers
// Define KISS_FFT before including to use kiss_fft instead of FFTW
#define KISS_FFT 1

extern "C" {
#include "sonic2.h"
// Note: sonic.h is already included by sonic2.h with SONIC_INTERNAL defined
// This means sonicSetPitch becomes sonicIntSetPitch, etc.
// Only the functions undef'd in sonic2.h use the regular names
}

// Component GUID - unique identifier for this DSP
// {8E4A9F2C-3B5D-4E7A-9C1F-6D8B2A4E5F3C}
static const GUID g_dsp_speedy_guid =
{ 0x8e4a9f2c, 0x3b5d, 0x4e7a, { 0x9c, 0x1f, 0x6d, 0x8b, 0x2a, 0x4e, 0x5f, 0x3c } };

// Configuration defaults (prefixed to avoid Windows SDK conflicts)
static const float kDefaultSpeed = 1.0f;
static const float kDefaultPitch = 1.0f;
static const float kDefaultRate = 1.0f;
static const float kDefaultVolume = 1.0f;
static const bool kDefaultNonlinear = false;
static const float kDefaultNonlinearFactor = 1.0f;

// Configuration structure
struct dsp_speedy_config {
    float speed;
    float pitch;
    float rate;
    float volume;
    bool nonlinear_enabled;
    float nonlinear_factor;

    dsp_speedy_config() :
        speed(kDefaultSpeed),
        pitch(kDefaultPitch),
        rate(kDefaultRate),
        volume(kDefaultVolume),
        nonlinear_enabled(kDefaultNonlinear),
        nonlinear_factor(kDefaultNonlinearFactor)
    {}

    bool is_default() const {
        return speed == kDefaultSpeed &&
               pitch == kDefaultPitch &&
               rate == kDefaultRate &&
               volume == kDefaultVolume &&
               nonlinear_enabled == kDefaultNonlinear;
    }

    void reset() {
        *this = dsp_speedy_config();
    }
};

// Forward declarations
static void make_preset(const dsp_speedy_config& config, dsp_preset& out);
static void parse_preset(const dsp_preset& preset, dsp_speedy_config& config);

// DSP implementation class
class dsp_speedy : public dsp_impl_base {
public:
    dsp_speedy(const dsp_preset& preset) {
        parse_preset(preset, m_config);
        m_stream = nullptr;
        m_sample_rate = 0;
        m_channels = 0;
        m_channel_config = 0;
    }

    ~dsp_speedy() {
        cleanup_stream();
    }

    static GUID g_get_guid() {
        return g_dsp_speedy_guid;
    }

    static void g_get_name(pfc::string_base& out) {
        out = "Speedy (Speed/Pitch)";
    }

    static bool g_get_default_preset(dsp_preset& out) {
        dsp_speedy_config config;
        make_preset(config, out);
        return true;
    }

    static void g_show_config_popup(const dsp_preset& preset, HWND parent, dsp_preset_edit_callback& callback);

    static bool g_have_config_popup() {
        return true;
    }

    bool on_chunk(audio_chunk* chunk, abort_callback& abort) override {
        if (m_config.is_default()) {
            return true; // Pass through unchanged
        }

        const t_size sample_count = chunk->get_sample_count();
        const unsigned sample_rate = chunk->get_srate();
        const unsigned channels = chunk->get_channels();
        const unsigned channel_config = chunk->get_channel_config();

        // Check if format changed
        if (sample_rate != m_sample_rate || channels != m_channels || channel_config != m_channel_config) {
            cleanup_stream();
            if (!init_stream(sample_rate, channels)) {
                return true; // Pass through on error
            }
            m_sample_rate = sample_rate;
            m_channels = channels;
            m_channel_config = channel_config;
        }

        if (!m_stream) {
            return true;
        }

        // Get input samples
        const audio_sample* input = chunk->get_data();

        // Convert float samples to short for Sonic (with clamping)
        m_input_buffer.resize(sample_count * channels);
        for (t_size i = 0; i < sample_count * channels; i++) {
            float sample = static_cast<float>(input[i]) * 32767.0f;
            if (sample > 32767.0f) sample = 32767.0f;
            if (sample < -32768.0f) sample = -32768.0f;
            m_input_buffer[i] = static_cast<short>(sample);
        }

        // Write to Sonic stream
        if (!sonicWriteShortToStream(m_stream, m_input_buffer.data(), static_cast<int>(sample_count))) {
            return true; // Pass through on error
        }

        // Read all available processed samples
        int max_samples = static_cast<int>(sample_count * 4); // Allow for slowdown
        m_output_buffer.resize(max_samples * channels);

        int total_read = 0;
        int samples_read;
        while ((samples_read = sonicReadShortFromStream(m_stream,
                m_output_buffer.data() + total_read * channels,
                max_samples - total_read)) > 0) {
            total_read += samples_read;
            if (total_read >= max_samples) break;
        }

        if (total_read > 0) {
            // Convert short output back to audio_sample for foobar2000
            m_audio_output.resize(total_read * channels);
            for (int i = 0; i < total_read * static_cast<int>(channels); i++) {
                m_audio_output[i] = static_cast<audio_sample>(m_output_buffer[i]) / 32767.0;
            }
            chunk->set_data(m_audio_output.data(), total_read, channels, sample_rate, channel_config);
        } else {
            // No output available yet - output silence
            m_audio_output.resize(sample_count * channels);
            std::fill(m_audio_output.begin(), m_audio_output.end(), static_cast<audio_sample>(0));
            chunk->set_data(m_audio_output.data(), sample_count, channels, sample_rate, channel_config);
        }

        return true;
    }

    void on_endofplayback(abort_callback& abort) override {
        flush_remaining();
    }

    void on_endoftrack(abort_callback& abort) override {
        // Could flush here if needed, but usually better to keep stream continuous
    }

    void flush() override {
        cleanup_stream();
        m_sample_rate = 0;
        m_channels = 0;
        m_channel_config = 0;
    }

    double get_latency() override {
        // Return approximate latency in seconds
        if (m_sample_rate > 0 && m_stream) {
            // Base Sonic latency
            double latency = 0.02; // ~20ms typical latency

            // Speedy nonlinear mode adds significant lookahead latency
            // kTemporalHysteresisFuture = 12 frames at 100Hz = 120ms
            if (m_config.nonlinear_enabled) {
                latency += 0.12; // ~120ms for Speedy lookahead
            }

            return latency;
        }
        return 0.0;
    }

    bool need_track_change_mark() override {
        return false;
    }

private:
    dsp_speedy_config m_config;
    sonicStream m_stream;
    unsigned m_sample_rate;
    unsigned m_channels;
    unsigned m_channel_config;

    std::vector<short> m_input_buffer;
    std::vector<short> m_output_buffer;
    std::vector<audio_sample> m_audio_output;

    bool init_stream(unsigned sample_rate, unsigned channels) {
        m_stream = sonicCreateStream(sample_rate, channels);
        if (!m_stream) {
            return false;
        }

        // Apply settings
        // sonicSetSpeed and sonicSetRate are wrapped by sonic2.h (call internal sonic)
        // sonicSetPitch and sonicSetVolume are renamed to Int versions by SONIC_INTERNAL
        sonicSetSpeed(m_stream, m_config.speed);
        sonicIntSetPitch(m_stream, m_config.pitch);
        sonicSetRate(m_stream, m_config.rate);
        sonicIntSetVolume(m_stream, m_config.volume);

        // Enable nonlinear speedup if requested
        if (m_config.nonlinear_enabled) {
            sonicEnableNonlinearSpeedup(m_stream, m_config.nonlinear_factor);
        }

        return true;
    }

    void cleanup_stream() {
        if (m_stream) {
            sonicDestroyStream(m_stream);
            m_stream = nullptr;
        }
    }

    void flush_remaining() {
        if (m_stream) {
            sonicFlushStream(m_stream);
            // Read any remaining samples
            m_output_buffer.resize(4096 * m_channels);
            int samples_read;
            do {
                samples_read = sonicReadShortFromStream(m_stream, m_output_buffer.data(), 4096);
            } while (samples_read > 0);
        }
    }
};

static void parse_preset(const dsp_preset& preset, dsp_speedy_config& config) {
    if (preset.get_owner() != g_dsp_speedy_guid) {
        config.reset();
        return;
    }

    try {
        const t_uint8* data = static_cast<const t_uint8*>(preset.get_data());
        t_size size = preset.get_data_size();

        if (size >= sizeof(float) * 5 + sizeof(bool)) {
            const float* floats = reinterpret_cast<const float*>(data);
            config.speed = floats[0];
            config.pitch = floats[1];
            config.rate = floats[2];
            config.volume = floats[3];
            config.nonlinear_factor = floats[4];
            config.nonlinear_enabled = *reinterpret_cast<const bool*>(data + sizeof(float) * 5);
        } else {
            config.reset();
        }
    } catch (...) {
        config.reset();
    }
}

static void make_preset(const dsp_speedy_config& config, dsp_preset& out) {
    out.set_owner(g_dsp_speedy_guid);

    // Simple binary format: 5 floats + 1 bool
    std::vector<char> data(sizeof(float) * 5 + sizeof(bool));
    float* floats = reinterpret_cast<float*>(data.data());
    floats[0] = config.speed;
    floats[1] = config.pitch;
    floats[2] = config.rate;
    floats[3] = config.volume;
    floats[4] = config.nonlinear_factor;
    *reinterpret_cast<bool*>(data.data() + sizeof(float) * 5) = config.nonlinear_enabled;

    out.set_data(data.data(), data.size());
}

// Dialog data stored for the dialog procedure
struct DialogData {
    dsp_speedy_config config;
    dsp_preset_edit_callback* callback;
};

static void UpdateDialogLabels(HWND hDlg, const dsp_speedy_config& config) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2fx", config.speed);
    SetDlgItemTextA(hDlg, IDC_SPEED_VALUE, buf);

    snprintf(buf, sizeof(buf), "%.2fx", config.pitch);
    SetDlgItemTextA(hDlg, IDC_PITCH_VALUE, buf);
}

static void UpdatePresetFromDialog(HWND hDlg, DialogData* data) {
    dsp_preset_impl preset;
    make_preset(data->config, preset);
    data->callback->on_preset_changed(preset);
}

static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    DialogData* data = reinterpret_cast<DialogData*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));

    switch (message) {
    case WM_INITDIALOG:
        {
            data = reinterpret_cast<DialogData*>(lParam);
            SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));

            // Initialize speed slider (25% to 400%)
            HWND hSpeedSlider = GetDlgItem(hDlg, IDC_SLIDER_SPEED);
            SendMessage(hSpeedSlider, TBM_SETRANGE, TRUE, MAKELPARAM(25, 400));
            SendMessage(hSpeedSlider, TBM_SETPOS, TRUE, static_cast<int>(data->config.speed * 100));

            // Initialize pitch slider (50% to 200%)
            HWND hPitchSlider = GetDlgItem(hDlg, IDC_SLIDER_PITCH);
            SendMessage(hPitchSlider, TBM_SETRANGE, TRUE, MAKELPARAM(50, 200));
            SendMessage(hPitchSlider, TBM_SETPOS, TRUE, static_cast<int>(data->config.pitch * 100));

            // Initialize nonlinear checkbox
            CheckDlgButton(hDlg, IDC_NONLINEAR, data->config.nonlinear_enabled ? BST_CHECKED : BST_UNCHECKED);

            UpdateDialogLabels(hDlg, data->config);
            return TRUE;
        }

    case WM_HSCROLL:
        {
            if (data) {
                HWND hSpeedSlider = GetDlgItem(hDlg, IDC_SLIDER_SPEED);
                HWND hPitchSlider = GetDlgItem(hDlg, IDC_SLIDER_PITCH);

                data->config.speed = SendMessage(hSpeedSlider, TBM_GETPOS, 0, 0) / 100.0f;
                data->config.pitch = SendMessage(hPitchSlider, TBM_GETPOS, 0, 0) / 100.0f;

                UpdateDialogLabels(hDlg, data->config);
                UpdatePresetFromDialog(hDlg, data);
            }
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_NONLINEAR:
            if (data && HIWORD(wParam) == BN_CLICKED) {
                data->config.nonlinear_enabled = (IsDlgButtonChecked(hDlg, IDC_NONLINEAR) == BST_CHECKED);
                UpdatePresetFromDialog(hDlg, data);
            }
            return TRUE;

        case IDC_RESET:
            if (data) {
                data->config.reset();

                HWND hSpeedSlider = GetDlgItem(hDlg, IDC_SLIDER_SPEED);
                HWND hPitchSlider = GetDlgItem(hDlg, IDC_SLIDER_PITCH);

                SendMessage(hSpeedSlider, TBM_SETPOS, TRUE, 100);
                SendMessage(hPitchSlider, TBM_SETPOS, TRUE, 100);
                CheckDlgButton(hDlg, IDC_NONLINEAR, BST_UNCHECKED);

                UpdateDialogLabels(hDlg, data->config);
                UpdatePresetFromDialog(hDlg, data);
            }
            return TRUE;

        case IDOK:
            EndDialog(hDlg, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void dsp_speedy::g_show_config_popup(const dsp_preset& preset, HWND parent, dsp_preset_edit_callback& callback) {
    DialogData data;
    parse_preset(preset, data.config);
    data.callback = &callback;

    DialogBoxParam(
        core_api::get_my_instance(),
        MAKEINTRESOURCE(IDD_DSP_SPEEDY),
        parent,
        DialogProc,
        reinterpret_cast<LPARAM>(&data)
    );
}

// Register the DSP
static dsp_factory_t<dsp_speedy> g_dsp_speedy_factory;

// Declare component version information
DECLARE_COMPONENT_VERSION(
    "Speedy DSP",
    "1.0.0",
    "Audio speed and pitch manipulation using Google's Speedy algorithm.\n"
    "Based on the Mach1 nonlinear speech speedup algorithm.\n\n"
    "Speedy: Copyright 2022 Google LLC (Apache 2.0)\n"
    "Sonic: Copyright 2010 Bill Cox (Apache 2.0)"
);

// Validate that we're building against compatible SDK
VALIDATE_COMPONENT_FILENAME("foo_dsp_speedy.dll");
