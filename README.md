1.0.5 - 07 Feb 2026 - https://www.virustotal.com/gui/file/d85f342cb54db2fd8f7286158c8c5fd26cc56bc81738040082f7cfe87692d23d?nocache=1
      - Reduced AV false positives (19 → 2 detections)
      - Enabled security features: Buffer Security, SDL
      - Added application manifest for Windows compatibility
      - Included debug information for AV verification
      - Enhanced version metadata with GitHub link

1.0.4 - 06 Feb 2026 - https://www.virustotal.com/gui/file/6f2382935b218920cffc103d86dc438425ccfd3235d6ab4420c9cc5f67e41551/detection
      - ⚠️ Higher AV detections due to aggressive optimizations

1.0.3 - 15 Jan 2026 - https://www.virustotal.com/gui/file/15f2795e01dc8574786b0a3305e8249a50a95665d1001bf305d616bd4b50b1b1/detection

# OSD Lock Indicator

<div align="center">

**A beautiful, ultra-lightweight Windows utility that displays an elegant on-screen notification when Caps Lock or Num Lock is toggled.**

![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange)
![Size](https://img.shields.io/badge/size-~230%20KB-green)
![License](https://img.shields.io/badge/license-MIT-brightgreen)

[Features](#-features) • [Installation](#-installation) • [Customization](#%EF%B8%8F-customization) • [Building](#-building-from-source)

</div>

---

## 🌟 Why This Exists

Ever accidentally typed in ALL CAPS because you didn't notice Caps Lock was on? Or struggled to see if Num Lock is enabled on your keyboard? This lightweight utility solves that with a beautiful, non-intrusive on-screen display.

Unlike bloated alternatives that consume 15-25 MB of RAM, **OSD Lock Indicator** uses just **~1.6 MB** while delivering a polished, professional experience.

---

## ✨ Features

### Visual Excellence
- 🎨 **Beautiful Design** - Smooth rounded corners with anti-aliased rendering
- 🌈 **Color-Coded Status** - Green for ON, Red for OFF
- ✨ **Smooth Animations** - Elegant fade-in and fade-out transitions with easing
- 🔍 **Perfect Clarity** - Semi-transparent background with 100% opaque text

### Performance
- ⚡ **Ultra-Lightweight** - ~209 KB executable size
- 🚀 **Minimal Memory** - Uses only ~1.6 MB of RAM
- 💨 **Instant Startup** - Launches in milliseconds
- 🎯 **Zero Dependencies** - No .NET framework or runtime required

### Functionality
- 🖥️ **Multi-Monitor Support** - Automatically displays on the active screen
- 🎮 **Game-Friendly** - Click-through window that never steals focus
- 🔝 **Always Visible** - Stays on top of all applications
- 🔄 **Optional Auto-Start** - Choose whether to launch with Windows on first run
- 🎯 **Text Stabilization** - No wiggling when toggling between ON/OFF

### Compatibility
- ✅ **Windows 10/11** - Fully supported
- ✅ **High DPI Displays** - Works on any resolution and scaling
- ✅ **Borderless Windowed Games** - Works seamlessly
- ✅ **Multiple Keyboards** - Detects any keyboard input

---

## 📸 Screenshots

### Caps Lock - ON
![CapsLock ON](screenshots/capslock-on.png)

### Num Lock - OFF
![NumLock OFF](screenshots/numlock-off.png)

---

## 🚀 Installation

### Quick Start (Recommended)

1. **Download** the latest release from the [Releases](../../releases) page
2. **Extract** `OsdLockIndicator.exe` to any folder
3. **Run** the executable
4. **Choose** whether to start automatically with Windows when prompted
5. That's it! The indicator is now running.

### First Run Setup

On first launch, you'll be asked:

> "Would you like this program to start automatically with Windows?"

- Click **Yes** to add to Windows startup
- Click **No** to run manually when needed

You can change this preference anytime using the `/install` command.

---

## 🗑️ Uninstall

### Complete Removal (Recommended)

Simply run the uninstall command - it will automatically:
1. Terminate any running instances
2. Remove itself from Windows startup
3. Clear all settings

**Steps:**

1. **Navigate to the folder** where `OsdLockIndicator.exe` is located
2. **Open Terminal:** Right-click on empty space → Select **"Open in Terminal"**
3. **Run the uninstall command:**
   ```powershell
   .\OsdLockIndicator.exe /uninstall
   ```
4. **Delete the file** after the confirmation message appears

### Alternative: Manual Removal

If you've already deleted the file without running the uninstall command:

1. Press `Win + R`, type `regedit`, press Enter
2. Navigate to: `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`
3. Find and delete the `OsdLockIndicator` entry
4. Also delete: `HKEY_CURRENT_USER\SOFTWARE\OsdLockIndicator` (if exists)
5. Open Task Manager (`Ctrl+Shift+Esc`) and end any `OsdLockIndicator.exe` processes

---

## 💻 Command Line Options

| Command | Description |
|---------|-------------|
| `OsdLockIndicator.exe` | Normal launch |
| `OsdLockIndicator.exe /install` | Change startup preference |
| `OsdLockIndicator.exe /uninstall` | Complete removal |

**Note:** `/install`, `--install`, and `-install` all work (same for uninstall).

---

## ⚙️ Customization

You can customize the appearance by modifying the constants at the top of the source code and rebuilding:

```cpp
// =============================================================================
// WINDOW SIZE & SHAPE
// =============================================================================

constexpr int OSD_WIDTH       = 175;    // Width of the indicator (pixels)
constexpr int OSD_HEIGHT      = 60;     // Height of the indicator (pixels)
constexpr int CORNER_RADIUS   = 20;     // Roundness of corners (0 = square)

// =============================================================================
// POSITION ON SCREEN
// =============================================================================

constexpr int DISTANCE_FROM_BOTTOM = 75;    // How far from the bottom of the screen (pixels)

// =============================================================================
// COLORS - Format: (Red, Green, Blue) - Values 0-255
// =============================================================================

// Background
constexpr int BG_ALPHA        = 80;     // Background transparency (0=invisible, 255=solid)
constexpr int BG_RED          = 0;      // Background red component
constexpr int BG_GREEN        = 0;      // Background green component  
constexpr int BG_BLUE         = 0;      // Background blue component

// "ON" Status Color (default: soft green)
constexpr int ON_RED          = 76;
constexpr int ON_GREEN        = 217;
constexpr int ON_BLUE         = 100;

// "OFF" Status Color (default: soft coral-red)
constexpr int OFF_RED         = 255;
constexpr int OFF_GREEN       = 95;
constexpr int OFF_BLUE        = 87;

// =============================================================================
// ANIMATION TIMING
// =============================================================================

constexpr int FADE_SPEED      = 25;     // Fade speed (higher = faster, 1-50 recommended)
constexpr int DISPLAY_TIME    = 1500;   // How long to show before fading out (milliseconds)
constexpr bool EASE_ANIMATION = true;   // true = smooth easing, false = linear fade
```

### Customization Examples

**More Transparent Background:**
```cpp
constexpr int BG_ALPHA = 50;  // Very transparent
```

**Larger Display:**
```cpp
constexpr int OSD_WIDTH = 200;
constexpr int OSD_HEIGHT = 80;
```

**Longer Display Time:**
```cpp
constexpr int DISPLAY_TIME = 2500;  // 2.5 seconds
```

**Higher on Screen:**
```cpp
constexpr int DISTANCE_FROM_BOTTOM = 150;  // 150px from bottom
```

**Linear Animation (no easing):**
```cpp
constexpr bool EASE_ANIMATION = false;
```

---

## 🔧 Building from Source

### Prerequisites

- **Windows 10/11**
- **Visual Studio 2022** (or later) with C++ Desktop Development workload
- **Windows SDK** (included with Visual Studio)

### Build Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/DopeMSR/OsdLockIndicator.git
   cd OsdLockIndicator
   ```

2. **Open in Visual Studio:**
   - Open `OsdLockIndicator.slnx`

3. **Configure for Release:**
   - Set configuration to **Release**
   - Set platform to **x64**

4. **Build:**
   - Press `Ctrl+Shift+B` or select **Build → Build Solution**
   - Find the executable in `x64/Release/OsdLockIndicator.exe`

### Build Optimization Settings (Already Configured)

The project is pre-configured with optimal settings:

- **Optimization:** MaxSpeed (/O2)
- **Runtime Library:** Multi-threaded (/MT) - no runtime dependencies
- **Incremental Linking:** Disabled
- **Debug Info:** Enabled in Release

---

## 🎮 Gaming Notes

This overlay works seamlessly with most games running in **Borderless Windowed** or **Windowed** mode.

### Exclusive Fullscreen Games

Some older games use exclusive fullscreen mode, which may hide the overlay. Solutions:

1. Switch the game to **Borderless Windowed** mode (recommended)
2. Use a borderless gaming tool to convert fullscreen to borderless
3. Alt+Tab out to see the notification

Most modern games support borderless windowed mode natively.

---

## 🔍 Technical Details

### Architecture
- **Language:** C++20
- **UI Framework:** Win32 API
- **Graphics:** GDI+ with hardware acceleration
- **Rendering:** Per-pixel alpha blending via `UpdateLayeredWindow`

### Window Properties
- **Style Flags:** `WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT`
- **Click-Through:** Yes - won't interfere with clicking on applications beneath it
- **Focus Stealing:** No - won't minimize or interrupt your active window
- **Always on Top:** Yes - visible over all other windows

### Keyboard Hook
- **Type:** Low-level keyboard hook (`WH_KEYBOARD_LL`)
- **Scope:** Global - detects all keyboard input
- **Keys Monitored:** Caps Lock (`VK_CAPITAL`), Num Lock (`VK_NUMLOCK`)
- **State Detection:** Uses `GetKeyState` to accurately read lock state

### Performance Benchmarks

| Metric | Value |
|--------|-------|
| Executable Size | ~209 KB |
| RAM Usage (Idle) | ~1.6 MB |
| RAM Usage (Active) | ~1.7 MB |
| CPU Usage (Idle) | <0.1% |
| CPU Usage (Animating) | ~0.3% |
| Startup Time | ~15 ms |

---

## 📊 Comparison to Alternatives

| Feature | OSD Lock Indicator | Typical Alternatives |
|---------|-------------------|---------------------|
| **File Size** | ~209 KB | 2-15 MB |
| **RAM Usage** | ~1.6 MB | 8-25 MB |
| **Dependencies** | None | .NET, Java, or other runtimes |
| **Visual Quality** | Anti-aliased, smooth | Varies |
| **Startup Time** | ~15 ms | 200-500 ms |
| **Multi-Monitor** | Yes | Sometimes |
| **Game Compatible** | Yes | Sometimes |
| **Auto-Start** | Optional (user choice) | Often forced |

---

## 🤔 FAQ

### Q: Does this work with laptop keyboards?
**A:** Yes! It works with any keyboard - built-in laptop keyboards, USB keyboards, Bluetooth keyboards, etc.

### Q: Will this slow down my computer?
**A:** No. It uses only ~1.6 MB of RAM and barely any CPU. It's more efficient than most system tray icons.

### Q: Can I use this on multiple monitors?
**A:** Yes! The indicator automatically appears on whichever monitor your mouse cursor is on.

### Q: Does this work with games?
**A:** Yes, for games running in Borderless Windowed or Windowed mode. Exclusive fullscreen may hide the overlay.

### Q: How do I change the auto-start setting?
**A:** Run `OsdLockIndicator.exe /install` to change your preference.

### Q: Is this safe? My antivirus flagged it.
**A:** Yes, it's completely safe. Some antivirus programs flag any software that uses keyboard hooks as potentially suspicious. This is a false positive - the source code is available for review. You can submit it to your AV vendor as a false positive.

### Q: Can I customize the colors?
**A:** Yes! Edit the color constants at the top of the source code and rebuild. All settings are clearly organized in the "USER SETTINGS" section.

### Q: Why doesn't it show Scroll Lock?
**A:** Scroll Lock is rarely used on modern systems. The code can be easily modified to include it if needed.

---

## 🛡️ Privacy & Security

- ✅ **No telemetry** - Doesn't connect to the internet
- ✅ **No data collection** - Doesn't log or store any information
- ✅ **No admin required** - Runs with standard user privileges
- ✅ **Open source** - Full source code available for review
- ✅ **Minimal permissions** - Only uses keyboard hook and window creation

**What it does:**
- Monitors Caps Lock and Num Lock key states
- Displays an on-screen notification
- Optionally adds itself to Windows startup registry

**What it doesn't do:**
- Capture or log keystrokes
- Send data over the network
- Modify system files
- Require elevated permissions

---

## 🐛 Troubleshooting

### The indicator doesn't appear

1. **Check if it's running:**
   - Open Task Manager (`Ctrl+Shift+Esc`)
   - Look for `OSD Lock Indicator` in the Processes tab

2. **Check display settings:**
   - The indicator appears 15 pixels from the bottom of your screen
   - Move your mouse to the desired monitor before pressing Caps/Num Lock

### Multiple instances are running

- Only one instance should run at a time due to mutex protection
- If multiple instances appear in Task Manager, run `/uninstall` to clean up

### The indicator appears on the wrong monitor

- The indicator follows your mouse cursor
- Move your mouse to the desired monitor before pressing Caps/Num Lock

### Antivirus is blocking it

- Add an exception for `OsdLockIndicator.exe` in your antivirus software
- This is a false positive - keyboard hooks are flagged by some antivirus programs
- Consider submitting to your AV vendor as a false positive

---

## 📝 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

**What this means:**
- ✅ Free to use commercially
- ✅ Free to modify and distribute
- ✅ No warranty provided
- ⚠️ Must include license and copyright notice in copies

---

<div align="center">

**Made with ❤️ and C++**

*Developed with assistance from Claude (Anthropic)*

[⬆ Back to Top](#osd-lock-indicator)

</div>
