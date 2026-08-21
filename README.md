# Search on Active Display (Windhawk Mod)

Opens the Windows Search pane (`Win+S` or taskbar icon) on the monitor where your mouse cursor is currently located, or on a specific monitor of your choice.

Native Windows 11 hardcodes the `Win+S` shortcut to always open the search pane on the primary display, regardless of where you are actively working. This mod fixes that behavior, making multi-monitor workflows much more seamless.

## Features
- **Follows the Cursor:** Press `Win+S` and the search pane appears instantly on the monitor you are currently using.
- **Fixed Monitor Override:** Optionally pin the search pane to a specific secondary monitor (e.g., Monitor 2).
- **Native Taskbar Anchoring:** Precisely calculates taskbar offsets and leverages native `Shcore.dll` DPI scaling to ensure the search pane looks identical to its native appearance, no matter the monitor's resolution or scale factor.
- **Highly Optimized:** Engineered with performance in mind. DPI queries are cached globally and cursor polls are deferred for tiny UI elements, ensuring zero overhead on the Windows layout engine.

## Compatibility
- Windows 11 (22H2 and 24H2)

## Installation
This mod requires [Windhawk](https://windhawk.net/), the customization platform for Windows.

1. Download and install [Windhawk](https://windhawk.net/).
2. Open Windhawk and go to "Explore".
3. Search for **Search on Active Display** and click "Install".

*(Alternatively, you can clone this repository, go to Windhawk -> "Create a new mod", paste the contents of `search-active-display.wh.cpp`, and compile it).*

## Configuration
You can configure the mod via the "Settings" tab in Windhawk:
- **Monitor:** Set to `0` (default) to follow the mouse cursor. Set to `1, 2, 3...` to force the search pane to always open on a specific monitor.
- **Monitor interface name:** Useful if your monitor IDs shift often. You can input a partial hardware interface string here to permanently lock the search pane to a specific physical display.

## How it works (Technical Details)
The mod hooks into `SearchHost.exe` (the UWP app responsible for rendering the modern search pane). It intercepts `MonitorFromWindow` and `MonitorFromRect` layout queries to trick `SearchHost` into recognizing the target monitor. 

Because UWP background apps do not natively reposition themselves across monitors on hotkey triggers, the mod actively calculates the relative coordinates, dynamically scales the window width/height based on the target monitor's DPI, and repositions the CoreWindow natively using `SetWindowPos`.

## License
MIT License. See the mod source code for details.
