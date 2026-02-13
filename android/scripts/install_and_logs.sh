#!/bin/bash

# Navigate to the android directory
cd "$(dirname "$0")/.."

echo "Running CMake to regenerate shader headers for Android..."
cd ..
rm -rf android/app/.cxx
cmake -DANDROID_BUILD=ON -B android/app/.cxx/Debug -S .

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

cd android

echo "Building and installing APK..."
./gradlew installDebug

if [ $? -ne 0 ]; then
    echo "Installation failed!"
    exit 1
fi

echo "Killing existing app instance to force clean relaunch..."
adb shell am force-stop com.redttg.matrix
sleep 1

echo "Clearing logcat buffer..."
adb logcat -c

echo "Starting wallpaper service..."
adb shell am start -n com.redttg.matrix/.MainActivity

echo "Showing logs (press Ctrl+C to stop)..."
sleep 1
adb logcat -v color --pid=$(adb shell pidof -s com.redttg.matrix)
