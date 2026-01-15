#!/bin/bash

echo "🧠 Initializing Robot Brain..."

# 1. Install Python Deps
echo "📦 Installing Python Dependencies..."
pip install -r requirements.txt

# 2. Download Vosk Model (Listening)
if [ ! -d "vosk-model" ]; then
    echo "👂 Downloading Vosk Model (Speech-to-Text)..."
    wget https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip
    unzip vosk-model-small-en-us-0.15.zip
    mv vosk-model-small-en-us-0.15 vosk-model
    rm vosk-model-small-en-us-0.15.zip
    echo "✅ Vosk Model Installed"
else
    echo "✅ Vosk Model already present"
fi

# 3. Download Piper (Speaking)
if [ ! -f "piper/piper" ]; then
    echo "🗣️ Downloading Piper (Text-to-Speech)..."
    
    ARCH=$(uname -m)
    if [ "$ARCH" = "aarch64" ]; then
        echo "   -> Detected ARM64 (Raspberry Pi 64-bit)"
        URL="https://github.com/rhasspy/piper/releases/download/2023.11.14-2/piper_linux_aarch64.tar.gz"
        FILE="piper_linux_aarch64.tar.gz"
    elif [ "$ARCH" = "armv7l" ]; then
        echo "   -> Detected ARMv7 (Raspberry Pi 32-bit)"
        URL="https://github.com/rhasspy/piper/releases/download/2023.11.14-2/piper_linux_armv7l.tar.gz"
        FILE="piper_linux_armv7l.tar.gz"
    else
        echo "   -> Detected x86_64 (PC)"
        URL="https://github.com/rhasspy/piper/releases/download/2023.11.14-2/piper_linux_x86_64.tar.gz"
        FILE="piper_linux_x86_64.tar.gz"
    fi

    wget -O $FILE $URL
    tar -xvf $FILE
    rm $FILE
    echo "✅ Piper Installed"
else
    echo "✅ Piper already present (Delete 'piper' folder to reinstall)"
fi

# 4. Download Voice
mkdir -p voices
if [ ! -f "voices/ryan-low.onnx" ]; then
    echo "🗣️ Downloading Voice (Ryan)..."
    wget -O voices/ryan-low.onnx https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/ryan/low/en_US-ryan-low.onnx
    wget -O voices/ryan-low.onnx.json https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/ryan/low/en_US-ryan-low.onnx.json
    echo "✅ Voice Installed"
else
    echo "✅ Voice already present"
fi

echo "✨ Brain Setup Complete!"
echo "⚠️  Ensure you have Ollama installed and have run: 'ollama pull gemma3:1b'"
