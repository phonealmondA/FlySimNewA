#!/bin/bash

echo "===================================="
echo "  KatieFlySimRust Launcher"
echo "===================================="
echo ""

# Check if Rust is installed
if ! command -v cargo &> /dev/null; then
    echo "ERROR: Cargo not found!"
    echo ""
    echo "Please install Rust from: https://rustup.rs/"
    echo ""
    echo "Quick install:"
    echo "  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
    echo ""
    exit 1
fi

echo "Rust/Cargo found!"
echo ""

# Check OS and provide SFML installation hints
echo "NOTE: This game requires SFML libraries to be installed."
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "On Ubuntu/Debian, install with:"
    echo "  sudo apt-get install libsfml-dev libcsfml-dev"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "On macOS, install with Homebrew:"
    echo "  brew install sfml csfml"
fi
echo ""

# Navigate to Rust project directory
cd KatieFlySimRust || exit 1

echo "Building and running KatieFlySimRust..."
echo "This may take a few minutes on first run..."
echo ""

# Build and run in release mode for better performance
cargo run --release

# Check if cargo run succeeded
if [ $? -ne 0 ]; then
    echo ""
    echo "===================================="
    echo "  Build/Run Failed!"
    echo "===================================="
    echo ""
    echo "Common issues:"
    echo "1. SFML not installed - See installation commands above"
    echo "2. Missing build tools - Install with:"
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "   sudo apt-get install build-essential"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "   xcode-select --install"
    fi
    echo ""
    exit 1
fi

echo ""
echo "Game closed successfully!"
