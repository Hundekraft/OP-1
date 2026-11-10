# Build and verification

Release date: 2026-11-10. Bubbler 1.5.0, Delayed 1.0.0, Pitch Delay 1.0.0.

This release changes version metadata from the native second-slot loader-fix builds (Bubbler 1.2.1, Delayed/Pitch Delay 0.4.1); device code and artwork are unchanged. Each plugin includes its own freestanding memset implementation. Import inspection confirms that no C-library memset or memcpy is requested from Opus.

Use the Opus plugin kit pinned in SDK-BASELINE.json (commit e47b95fce500c470447eae1ca31a638c67b6dea4, core baseline 2.33), with a Blackfin compiler selected through OP1_BFIN_TOOLCHAIN. The native page interface requires core 2.31 or later.

Build each Source/DEVICE/plugin with `python3 /path/to/plugin-kit/plugin_tool.py build Source/DEVICE/plugin`. Inspect each output using `show`, and validate its screen.svg using `svg`.

Run `python3 Source/test.py /path/to/plugin-kit` for the desktop tests. They use clang with AddressSanitizer and UndefinedBehaviorSanitizer and model native instances and page callbacks. They cover independent same-type controls and audio/history, page changes, encoder pushes, drawing boundaries, reselection defaults, dry master and Bubbler pitch-mode packing. Delayed also passed Smooth LFO transition checks, 1.536 million samples of Smooth Off reference equivalence, and 2.56 million stress samples.

Desktop tests do not emulate the complete firmware, its second-slot manager or the hardware display/audio timing. Exact-version OP-1 load, sound recall, OFF overlay, CPU/memory, sound and coexistence results are not recorded. No device-tested manifest claim or measured CPU figure has been invented.

The attached illustrated PDF is redistributed byte-for-byte. Source notices, original SoundHack terms and GPL licence are included.
