# Maelstrom Android Build

## Prerequisites

1. **Android Studio** (latest stable) — or just the command-line SDK tools
2. **Android SDK** with:
   - SDK Platform 35 (Android 15)
   - NDK 28.2.13676358 (or update `ndkVersion` in `app/build.gradle`)
   - CMake (from SDK Manager → SDK Tools)
3. **Java 17+** (bundled with Android Studio)

## Building

### From Android Studio
1. Open the `android/` folder as an Android Studio project
2. Wait for Gradle sync to complete
3. Build → Make Project (or Run on a connected device/emulator)

### From Command Line
```bash
cd android
# On Linux/macOS:
./gradlew assembleDebug
# On Windows:
gradlew.bat assembleDebug
```

The APK will be at: `app/build/outputs/apk/debug/app-debug.apk`

## Project Structure

```
android/
├── build.gradle          # Root Gradle config
├── settings.gradle
├── app/
│   ├── build.gradle      # App config (SDK versions, ABI filters, CMake path)
│   ├── jni/
│   │   └── CMakeLists.txt   # NDK build — references game sources from repo root
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── assets/Data/     # Game assets (copy of repo Data/)
│       ├── java/
│       │   ├── com/maelstrom/game/MaelstromActivity.java
│       │   └── org/libsdl/app/   # SDL3 Java classes
│       └── res/              # Icons, strings, theme
```

## Notes

- The game renders at 640x480 with SDL logical presentation (letterboxed scaling)
- Forced landscape orientation via `android:screenOrientation="sensorLandscape"`
- Touch input is handled by the existing thumbstick UI element
- Steam integration is automatically excluded on Android
- Only `arm64-v8a` is built by default. Edit `abiFilters` in `app/build.gradle` to add other ABIs
- The `assets/Data/` folder is a copy of the repo's `Data/` directory. After modifying game assets, re-copy them

## Updating Assets

After changing files in the root `Data/` folder:
```bash
rm -rf app/src/main/assets/Data
cp -r ../Data app/src/main/assets/Data
```
