#!/bin/bash

# Navigate to the android directory
cd "$(dirname "$0")/.."

echo "Building debug APK..."
./gradlew assembleDebug

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Installing APK..."
./gradlew installDebug

if [ $? -ne 0 ]; then
    echo "Installation failed!"
    exit 1
fi

echo "Clearing logcat buffer..."
adb logcat -c

echo "Starting app..."
adb shell am start -n com.redttg.matrix/com.redttg.matrix.MainActivity

echo "Showing logs (press Ctrl+C to stop)..."
adb logcat -v color --pid=$(adb shell pidof -s com.redttg.matrix)
