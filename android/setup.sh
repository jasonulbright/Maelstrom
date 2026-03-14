#!/bin/bash
# Copies SDL Java sources and game assets into the Android project.
# Run this after cloning or after updating SDL/Data.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Copying SDL Java sources..."
mkdir -p "$SCRIPT_DIR/app/src/main/java/org/libsdl/app"
cp "$REPO_ROOT/external/SDL/android-project/app/src/main/java/org/libsdl/app/"*.java \
   "$SCRIPT_DIR/app/src/main/java/org/libsdl/app/"

echo "Copying game assets..."
rm -rf "$SCRIPT_DIR/app/src/main/assets/Data"
mkdir -p "$SCRIPT_DIR/app/src/main/assets"
cp -r "$REPO_ROOT/Data" "$SCRIPT_DIR/app/src/main/assets/Data"

echo "Copying launcher icons from SDL template..."
for dir in mipmap-hdpi mipmap-mdpi mipmap-xhdpi mipmap-xxhdpi mipmap-xxxhdpi; do
    mkdir -p "$SCRIPT_DIR/app/src/main/res/$dir"
    cp "$REPO_ROOT/external/SDL/android-project/app/src/main/res/$dir/ic_launcher.png" \
       "$SCRIPT_DIR/app/src/main/res/$dir/" 2>/dev/null
done

echo "Done. Open android/ in Android Studio or run: ./gradlew assembleDebug"
