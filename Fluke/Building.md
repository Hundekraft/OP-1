# Build Fluke

The complete plugin source is in `Source/esc.fluke`, with the pinned SDK headers and their provenance in `Source/sdk`, `SDK-SOURCE.json` and `SDK-LICENSE`.

Use the Opus plugin kit and a Blackfin toolchain compatible with that pinned SDK. Set `OP1_BFIN_TOOLCHAIN` to the compiler installation directory, then run:

```
python3 plugin_tool.py build Source/esc.fluke --out fluke.opmod
python3 plugin_tool.py show fluke.opmod
python3 plugin_tool.py svg Source/esc.fluke/screen.svg
```

Host checks use Clang with ASan/UBSan. From the Fluke directory:

```
clang -std=c11 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -I Source/sdk Source/tests/test_fluke.c -o /tmp/fluke-test
FLUKE_TEST_OUTPUT=/tmp /tmp/fluke-test
clang -std=c11 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -I Source/sdk Source/tests/test_save_stutter.c -o /tmp/fluke-save-test
FLUKE_TEST_OUTPUT=/tmp /tmp/fluke-save-test
clang -std=c11 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined Source/tests/test_variation.c -o /tmp/fluke-variation
/tmp/fluke-variation
```

The historical test filename `test_save_stutter.c` now covers save/recall and synchronized Split jumps. Stutter was removed. Clean builds and host checks do not establish physical OP-1 compatibility or CPU cost.
