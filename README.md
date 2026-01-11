# SODA Player

**SODA Player** is a modern, open-source multimedia player for Linux focused on audio playback with support for local files, YouTube audio streaming, and podcasts.

## Features

- **Multi-source playback**: Play local files, YouTube audio, and podcasts from a unified interface
- **Customizable UI**: HTML/CSS-based skin system for complete interface customization
- **Plugin architecture**: Extend functionality with plugins for equalizers, visualizers, and more
- **Privacy-focused**: No telemetry, no cloud services, all data stored locally
- **Modern formats**: Native support for MP3, FLAC, OGG, OPUS, M4A, and WAV

## Building from Source

### Prerequisites

- CMake 3.16+
- C++20 compatible compiler (GCC 10+, Clang 12+)
- GTK4
- WebKitGTK 4.1
- yaml-cpp
- libcurl
- TagLib
- ALSA development libraries

On Ubuntu/Debian:
```bash
sudo apt install cmake g++ libgtk-4-dev libwebkitgtk-6.0-dev \
    libyaml-cpp-dev libcurl4-openssl-dev libtag1-dev libasound2-dev \
    libjsoncpp-dev
```

On Fedora:
```bash
sudo dnf install cmake gcc-c++ gtk4-devel webkitgtk6.0-devel \
    yaml-cpp-devel libcurl-devel taglib-devel alsa-lib-devel jsoncpp-devel
```

### Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Install

```bash
sudo make install
```

## Usage

```bash
# Start SODA Player
soda-player

# Play a specific file
soda-player ~/Music/song.mp3

# Start with a different skin
soda-player --skin default-light

# Run in headless mode (for CLI/daemon usage)
soda-player --headless
```

## Configuration

Configuration files are stored in `~/.config/soda-player/`:

- `config.yaml` - Main application settings
- `playlists/` - Your playlists in YAML format

Data is stored in `~/.local/share/soda-player/`:

- `music/` - Downloaded audio files
- `skins/` - Custom skins
- `plugins/` - Installed plugins

## Creating Skins

Skins are HTML/CSS/JS packages located in the skins directory. Each skin needs:

- `manifest.yaml` - Skin metadata
- `index.html` - Main HTML structure
- `style.css` - Styling
- `script.js` - Interactive functionality (optional)

The JavaScript API (`window.soda`) provides access to playback controls, library, and settings.

See `skins/default-dark/` for a complete example.

## Creating Plugins

Plugins are shared libraries that implement the `PluginInterface`. See `include/soda/plugin_interface.hpp` for the API.

Plugin types:
- **Audio processors**: Equalizers, effects, normalization
- **Visualizers**: Spectrum analyzers, waveforms
- **Sources**: Additional streaming services
- **Tools**: Lyrics, scrobbling, library management

## License

SODA Player is licensed under the GNU General Public License v3.0 or later.

## Contributing

Contributions are welcome! Please see our contribution guidelines before submitting pull requests.

## Disclaimer

YouTube audio extraction functionality is provided for personal use. Users are responsible for ensuring their use complies with applicable laws and YouTube's Terms of Service.
