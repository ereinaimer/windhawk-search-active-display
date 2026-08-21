# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-08-21
### Changed
- Improved hooking stability by introducing a reentrancy guard to prevent potential stack overflow edge cases.
- Completely rewrote monitor identification logic to ensure full MIT license compliance.
- Fixed an issue where native DPI scaling could fail silently on some systems by explicitly linking `Shcore.dll`.

## [1.0.0] - 2026-08-21
### Added
- Initial release of the Search on Active Display mod.
- Core functionality to spawn the Windows Search (`Win+S`) pane on the monitor containing the active mouse cursor.
- Ability to lock the search pane to a specific monitor ID or hardware interface name.
- Native DPI scaling integration using `Shcore.dll` to properly size the search pane on mixed-resolution multi-monitor setups.
- Dynamic taskbar anchoring calculation to ensure the search pane flawlessly matches native positioning on any monitor.
