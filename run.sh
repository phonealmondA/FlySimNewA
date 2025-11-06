#!/bin/bash

echo "===================================="
echo "  KatieFlySimRust Launcher"
echo "  Pure Rust - Zero Dependencies!"
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
    echo "Please check the error messages above."
    echo ""
    exit 1
fi

echo ""
echo "Game closed successfully!"
