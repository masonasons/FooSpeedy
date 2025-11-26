# foo_dsp_speedy - Speedy DSP for foobar2000

A foobar2000 DSP component that provides audio playback speed and pitch manipulation using Google's Speedy algorithm.

## Features

- **Playback Speed Control**: Adjust playback speed from 0.25x to 4.0x
- **Pitch Control**: Modify pitch from 0.5x to 2.0x independently of speed
- **Nonlinear Speedup**: Google's Speedy algorithm for natural-sounding speech speedup
  - Compresses vowels and unstressed syllables more than consonants
  - Maintains intelligibility at higher speeds

## Building

### Prerequisites

1. **Visual Studio 2022** (or 2019 with v143 toolset)
2. **foobar2000 SDK** (download from https://www.foobar2000.org/SDK)
3. **WTL (Windows Template Library)** - installed via NuGet

### Setup Instructions

1. Download the foobar2000 SDK from https://www.foobar2000.org/SDK
2. Extract the SDK to `lib/foobar2000_SDK/` so the structure looks like:
   ```
   lib/
     foobar2000_SDK/
       foobar2000/
       libPPUI/
       pfc/
   ```

3. Open `foo_dsp_speedy.sln` in Visual Studio

4. Install WTL NuGet package:
   - Right-click on solution → Manage NuGet Packages for Solution
   - Search for "wtl" and install to the following projects:
     - foobar2000_sdk_helpers
     - libPPUI

5. Build the solution (Release x64 recommended for modern foobar2000)

6. Copy the resulting `foo_dsp_speedy.dll` from `bin/Release/x64/` to your foobar2000 components folder

## Usage

1. In foobar2000, go to **File → Preferences → Playback → DSP Manager**
2. Add "Speedy (Speed/Pitch)" to the active DSP chain
3. Double-click to configure:
   - Adjust **Playback Speed** slider for faster/slower playback
   - Adjust **Pitch** slider to change pitch without affecting speed
   - Enable **Nonlinear speedup** for speech-optimized speed changes

## Libraries Used

- **Google Speedy** - Nonlinear speech speedup algorithm (Apache 2.0)
- **Sonic** by Bill Cox - Core audio time-stretching (Apache 2.0)
- **KISS FFT** - Fast Fourier Transform library (BSD)

## License

Apache License 2.0

## Credits

- Google's Speedy algorithm: https://github.com/google/speedy
- Bill Cox's Sonic library: https://github.com/waywardgeek/sonic
- KISS FFT: https://github.com/mborgerding/kissfft
- foobar2000 SDK: https://www.foobar2000.org/SDK
