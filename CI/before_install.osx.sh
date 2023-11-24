#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1

brew uninstall --ignore-dependencies python@3.8 || true
brew uninstall --ignore-dependencies python@3.9 || true
brew uninstall --ignore-dependencies qt@6 || true
brew uninstall --ignore-dependencies jpeg || true

brew tap --repair
brew update --quiet

# Some of these tools can come from places other than brew, so check before installing
brew reinstall xquartz fontconfig

command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt@5

# Install deps
brew install icu4c yaml-cpp sqlite
brew install vulkan-headers vulkan-loader vulkan-tools vulkan-extensionlayer vulkan-validationlayers
export PATH="/usr/local/opt/qt@5/bin:$PATH"  # needed to use qmake in none default path as qt now points to qt6

ccache --version
cmake --version
qmake --version

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20221113.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null

