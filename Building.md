# Building from source

Use the [Opus plugin kit](https://github.com/LizardMan245/opus) at commit `81968daa84f865886526de9bf7cc86725ae493a1`, its documented Blackfin toolchain and SDK setup. Read that kit’s AGENTS.md and docs before building. The kit is not bundled here.

Run `python3 /path/to/opus/plugin_tool.py build /path/to/Delay-Trio/Bubbler/Source/plugin`, or replace that final directory with `Delayed/Source/plugin` or `Pitch-Delay/Source/plugin`. Set `OP1_BFIN_TOOLCHAIN` to your toolchain installation as described by the kit. The kit writes its build output into its own build directory: copy each output before building the next device.

Bubbler’s native test runner is `Bubbler/Source/test.py`; the pair’s shared runner is `Tests/test.py`. Both accept the kit path. These desktop fixtures validate control/API behavior and do not emulate the native display or establish hardware CPU performance. Device testing remains necessary after code changes.

Generated lookup headers and embedded SVGs are included with the source, together with their available generation scripts. The published binaries are unchanged from the approved local release artifacts; SHA256SUMS records their exact identity.
