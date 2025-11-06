#!/bin/bash

echo "===================================="
echo "  SFML Installation Helper"
echo "===================================="
echo ""

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected Linux"
    echo ""

    # Check if running Ubuntu/Debian
    if command -v apt-get &> /dev/null; then
        echo "Installing SFML via apt-get..."
        sudo apt-get update
        sudo apt-get install -y libsfml-dev libcsfml-dev

        if [ $? -eq 0 ]; then
            echo ""
            echo "✓ SFML installed successfully!"
        else
            echo ""
            echo "✗ Installation failed. Try manually:"
            echo "  sudo apt-get install libsfml-dev libcsfml-dev"
        fi

    # Check if running Fedora/RHEL
    elif command -v dnf &> /dev/null; then
        echo "Installing SFML via dnf..."
        sudo dnf install -y SFML-devel CSFML-devel

        if [ $? -eq 0 ]; then
            echo ""
            echo "✓ SFML installed successfully!"
        else
            echo ""
            echo "✗ Installation failed. Try manually:"
            echo "  sudo dnf install SFML-devel CSFML-devel"
        fi

    # Check if running Arch
    elif command -v pacman &> /dev/null; then
        echo "Installing SFML via pacman..."
        sudo pacman -S --noconfirm sfml csfml

        if [ $? -eq 0 ]; then
            echo ""
            echo "✓ SFML installed successfully!"
        else
            echo ""
            echo "✗ Installation failed. Try manually:"
            echo "  sudo pacman -S sfml csfml"
        fi
    else
        echo "Unknown package manager."
        echo "Please install SFML manually from: https://www.sfml-dev.org/"
    fi

elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS"
    echo ""

    # Check if Homebrew is installed
    if command -v brew &> /dev/null; then
        echo "Installing SFML via Homebrew..."
        brew install sfml csfml

        if [ $? -eq 0 ]; then
            echo ""
            echo "✓ SFML installed successfully!"
        else
            echo ""
            echo "✗ Installation failed. Try manually:"
            echo "  brew install sfml csfml"
        fi
    else
        echo "Homebrew not found. Install it first:"
        echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        echo ""
        echo "Then run: brew install sfml csfml"
    fi

else
    echo "Unsupported OS: $OSTYPE"
    echo "Please install SFML manually from: https://www.sfml-dev.org/"
fi

echo ""
echo "===================================="
echo "After SFML is installed, run:"
echo "  ./run.sh    (Linux/macOS)"
echo "  run.cmd     (Windows)"
echo "===================================="
