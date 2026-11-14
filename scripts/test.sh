#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
clang -Wall -Wextra -Werror Tests/EngineTests.c -framework ApplicationServices -o build/tests/engine
build/tests/engine
clang -Wall -Wextra -Werror -c Sources/Core/GestureEngine.c -o build/tests/engine.o
clang -Wall -Wextra -Werror -fobjc-arc Tests/SettingsTests.m build/tests/engine.o -framework Foundation -framework ApplicationServices -o build/tests/settings
build/tests/settings
