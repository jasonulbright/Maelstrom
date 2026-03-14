package com.maelstrom.game;

import org.libsdl.app.SDLActivity;

/**
 * Maelstrom game activity. Extends SDLActivity which handles all SDL
 * initialization, JNI bridging, input, and lifecycle management.
 */
public class MaelstromActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        return new String[]{
            "SDL3",
            "main"
        };
    }
}
