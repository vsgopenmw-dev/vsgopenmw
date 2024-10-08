#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1

# things that are installed by default, we need to purge
brew uninstall --ignore-dependencies python@3.8 || true
brew uninstall --ignore-dependencies python@3.9 || true
brew uninstall --ignore-dependencies qt@6 || true
brew uninstall --ignore-dependencies jpeg || true
brew uninstall --ignore-dependencies node || true
brew uninstall --ignore-dependencies php || true
brew uninstall --ignore-dependencies selenium-server || true

brew tap --repair
brew update --quiet

# Some of these tools can come from places other than brew, so check before installing
brew reinstall xquartz fontconfig freetype harfbuzz brotli

# Fix: can't open file: @loader_path/libbrotlicommon.1.dylib (No such file or directory)
BREW_LIB_PATH="$(brew --prefix)/lib"
install_name_tool -change "@loader_path/libbrotlicommon.1.dylib" "${BREW_LIB_PATH}/libbrotlicommon.1.dylib" ${BREW_LIB_PATH}/libbrotlidec.1.dylib
install_name_tool -change "@loader_path/libbrotlicommon.1.dylib" "${BREW_LIB_PATH}/libbrotlicommon.1.dylib" ${BREW_LIB_PATH}/libbrotlienc.1.dylib

command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt@5
export PATH="/usr/local/opt/qt@5/bin:$PATH"


# Install deps
brew install icu4c yaml-cpp sqlite
brew install vulkan-headers vulkan-loader vulkan-validationlayers
# vulkan-tools vulkan-extensionlayer

ccache --version
cmake --version
qmake --version

if [[ "${MACOS_AMD64}" ]]; then
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20231220.zip -o ~/openmw-deps.zip
else
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20231022_arm64.zip -o ~/openmw-deps.zip
fi

unzip -o ~/openmw-deps.zip -d /tmp > /dev/null

