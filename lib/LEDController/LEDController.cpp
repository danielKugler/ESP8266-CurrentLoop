// LEDController.cpp
#include "LEDController.h"

LEDController::LEDController(uint8_t pin, uint16_t numLEDs, int brightness)
    : strip(numLEDs, pin, NEO_GRB + NEO_KHZ800), numLEDs(numLEDs), lastUpdateTime(0), animationActive(false), state(IDLE), currentLED(0) {
    strip.setBrightness(brightness);
    strip.begin();
    strip.show();  // Initialisiert alle LEDs als ausgeschaltet
}

/**
 * Animationen für:
 * - Nicht Verbunden
 * - Erfolgreich Verbunden
 * - Speichern erfolgreich
 * - Reset des anderen ESP
 * - Eigener Reset
 */

/**
 * Das unterste LED leuchtet in der Menü-Farbe
 * Dazwischen wird in einer anderen FArbe der aktuelle schritt angezeigt
 */

/**
 * @brief Calculates the logarithmic brightness value for the LED strip.
 *
 * This function takes a linear brightness value and maps it to a logarithmic
 * scale to provide a more perceptually linear brightness change on the LED strip.
 * The function uses predefined mappings for brightness levels from 1 to 6.
 *
 * @param brightness An integer representing the linear brightness level (1 to 6).
 * @return An integer representing the logarithmic brightness value, used by the LED strip.
 */
int calculateLogBrightness(int brightness) {
    switch (brightness) {
        case 1:
            return 1;
        case 2:
            return 5;
        case 3:
            return 30;
        case 4:
            return 50;
        case 5:
            return 150;
        case 6:
            return 255;
        default:
            return 1;
    };
}

/**
 * @brief Sets the brightness of the LED strip.
 *
 * This function updates the internal brightness value and adjusts the
 * LED strip's brightness using a logarithmic scale for better visual
 * representation.
 *
 * @param value The new brightness level to set.
 */
void LEDController::setBrightness(int value) {
    brightness = value;
    strip.setBrightness(calculateLogBrightness(brightness));
}

/**
 * @brief Displays the LED menu indicator.
 *
 * This function clears the LED strip and lights up the first LED with the
 * menu indicator color. It then lights up additional LEDs based on the
 * specified step value, using the menu step color. Finally, it updates
 * the LED strip to show the changes.
 *
 * @param step The menu step value to be represented on the LED strip.
 */
void LEDController::menuIndicator(int step) {
    clear();
    setPixel(0, colorMenuIndicator);
    for (int i = 1; i <= step; i++) {
        setPixel(i, colorMenuStep);
    }
    strip.show();
}

/**
 * @brief Displays the LED menu value selection.
 *
 * This function clears the LED strip and lights up the first two LEDs with the
 * menu indicator color. It then lights up additional LEDs based on the
 * specified brightness value, using the menu step color. Finally, it updates
 * the LED strip to show the changes.
 *
 * @param brightness The brightness level to be represented on the LED strip.
 */

void LEDController::menuValueSelection(int brightness) {
    clear();
    for (int i = 0; i < 2; i++) {
        setPixel(i, colorMenuIndicator);
    }
    for (int i = 1; i <= brightness; i++) {
        setPixel(1 + i, colorMenuStep);
    }
    strip.show();
}

/**
 * @brief Activates the menu animation on the LED strip.
 *
 * Sets the current menu step and clears the LEDs.
 * Updates the LED strip to indicate the active menu
 * state, initializing the animation with the specified
 * step. The animation state is set to active, and the
 * LEDs are prepared to visually represent the current
 * menu selection.
 *
 * @param step The current step in the menu to be indicated
 * on the LED strip.
 */
void LEDController::menuActiveAnimation(int step) {
    menuStep = step;
    for (int i = 1; i <= numLEDs; i++) {
        setPixel(i, 0);
    }
    strip.show();
    state = MENU_ACTIVE;
    color = colorMenuStep;
    lastUpdateTime = millis();
    animationActive = true;
}

/**
 * @brief Initiates a shutdown animation on the LED strip.
 *
 * This function sets all LEDs to red, indicating a shutdown sequence.
 * It then begins a fade-up animation from the current state. The animation
 * runs in a loop until it is stopped. An optional callback can be provided,
 * which will be invoked when the animation completes.
 *
 * @param callback An optional callback function that is called after the animation
 * ends. If a callback is provided, it is stored and invoked upon completion.
 */
void LEDController::shutdownAnimation(Callback callback) {
    if (callback) {
        animationCallback = callback;
    }
    uint32_t red = strip.Color(255, 0, 0);
    currentLED = numLEDs - 1;
    for (int i = 0; i < numLEDs; i++) {
        setPixel(i, red);
    }
    strip.show();
    state = FADE_UP;
    initAnimation(red, std::bind(&LEDController::shutdownAnimation, this, callback));
}

/**
 * @brief Initiates an apply animation on the LED strip.
 *
 * This function starts an animation that visually indicates an apply action
 * by fading the LEDs up from black to a predefined color and back down.
 * The animation is executed in a loop until it is stopped. An optional callback
 * can be provided, which will be invoked when the animation completes.
 *
 * @param callback An optional callback function that is called after the animation
 * ends. If a callback is provided, it is stored and invoked upon completion.
 */
void LEDController::applyAnimation(Callback callback) {
    if (callback) {
        animationCallback = callback;
    }
    state = FADE_UP;
    initAnimation(colorApply, std::bind(&LEDController::applyAnimation, this, callback));
}

/**
 * @brief Starts a success animation on the LED strip.
 *
 * This function is called when a success state is reached, and it initiates
 * an animation on the LED strip to visually indicate the success state to the
 * user. The animation is a fade from black to green and back to black.
 *
 * @param callback An optional callback function that is called when the animation
 * is finished.
 */
void LEDController::successAnimation(Callback callback) {
    if (callback) {
        animationCallback = callback;
    }
    state = FADE_UP;
    initAnimation(colorSuccess, std::bind(&LEDController::applyAnimation, this, callback));
}

/**
 * @brief Starts the default animation loop.
 *
 * This function starts the default animation loop by calling initAnimation
 * with the default color and a callback to itself. This creates an infinite
 * animation loop that fades the LED strip up and down.
 */
void LEDController::startAnimation() {
    state = FADE_UP;
    initAnimation(strip.Color(185, 50, 255), std::bind(&LEDController::startAnimation, this));
}

/**
 * @brief Animates a red blinking animation to visually indicate a failure state.
 *
 * This function is called when a failure occurs, and it initiates
 * an animation on the LED strip to visually indicate a failure state
 * to the user.
 */
void LEDController::failureAnimation() {
    state = FADE_UP;
    initAnimation(strip.Color(255, 0, 0), std::bind(&LEDController::failureAnimation, this));
}

/**
 * @brief Initiates a red blinking animation for the first LED.
 *
 * This function sets the first LED to red and begins a blinking animation.
 * It updates the animation state to BLINK_FIRST and sets the active color
 * to red, activating the animation sequence.
 */

void LEDController::blinkRed() {
    setPixel(0, color);  // LED an
    state = BLINK_FIRST;
    color = strip.Color(255, 0, 0);
    animationActive = true;
}

/**
 * @brief Updates the LED animation state.
 *
 * This function is responsible for updating the current animation
 * of the LED strip by calling the animation update routine. It
 * ensures that the LED animations are continuously refreshed
 * based on the current state.
 */
void LEDController::updateAnimation() {
    if (!animationActive) return;

    currentTime = millis();

    switch (state) {
        case FADE_UP:
            fadeUp();
            break;
        case FADE_DOWN:
            fadeDown();
            break;
        case BLINK_FIRST:
            blinkFirstLED();
            break;
        case MENU_ACTIVE:
            blinkMenu();
            break;
        case BLINK_RED_2500:
            blink(0, numLEDs, 100, 2500);
            break;
        case IDLE:
            // Nichts tun, während IDLE
            break;
    }
}

/**
 * @brief Flashes the LEDs between `startLED` and `endLED` with the duration `time` and the total duration `timeTotal`.
 *
 * `time` specifies the duration for which the LEDs are to be on or off.
 * `timeTotal` specifies the total duration after which the animation is completed.
 * If `timeTotal` is greater than 0, the animation is ended after `timeTotal` milliseconds have elapsed and the `animationCallback` callback is called.
 *
 * Calling this function starts the animation and the LEDs flash until the animation ends.
 */
void LEDController::blink(int startLED, int endLED, uint32_t time, uint32_t timeTotal) {
    static bool blinkOn = true;  // LED-Zustand verfolgen

    if (blinkOn && (currentTime - lastUpdateTime >= time)) {
        // Wenn die LED an ist und die Zeit für "an" abgelaufen ist
        for (int i = startLED; i <= endLED; i++) {
            setPixel(i, color);
        }
        strip.show();
        lastUpdateTime = currentTime;  // Zeitstempel aktualisieren
        blinkOn = false;               // Zustand wechseln
    } else if (!blinkOn && (currentTime - lastUpdateTime >= time)) {
        // Wenn die LED aus ist und die Zeit für "aus" abgelaufen ist
        for (int i = startLED; i <= endLED; i++) {
            setPixel(i, 0);
        }
        strip.show();
        lastUpdateTime = currentTime;  // Zeitstempel aktualisieren
        blinkOn = true;                // Zustand wechseln
    }
    if (timeTotal > 0) {
        if (currentTime - animationStartTime >= timeTotal) {
            stopAnimation();
            if (animationCallback) {
                animationCallback();
            }
        }
    }
}

/**
 * @brief Blinks the LEDs representing the active menu.
 *
 * This function triggers a blinking animation for the LEDs
 * from the second LED up to the current menu step, providing
 * a visual indicator for the active menu state.
 */
void LEDController::blinkMenu() {
    blink(1, menuStep, 500);
}

/**
 * Fades in the LEDs in the strip by turning them on one by one.
 *
 * This function is part of the animation loop and is called repeatedly
 * by the LEDController. It checks if it is time to turn on the next LED
 * and if so, turns it on. The function also keeps track of the current
 * LED and checks if all LEDs have been turned on. If so, it switches to
 * the FADE_DOWN state.
 */
void LEDController::fadeUp() {
    // Überprüft, ob es Zeit ist, die nächste LED einzuschalten
    if (currentTime - lastUpdateTime >= 100) {
        setPixel(currentLED, color);  // Schaltet die aktuelle LED ein
        strip.show();
        currentLED++;

        lastUpdateTime = currentTime;  // Setzt den Timer für den nächsten Schritt zurück

        // Wenn alle LEDs eingeschaltet sind, wechselt zu TURN_OFF
        if (currentLED >= numLEDs) {
            state = FADE_DOWN;
            currentLED = numLEDs - 1;   // Beginnt von der letzten LED zum Ausschalten
            lastUpdateTime = millis();  // Setzt den Timer für das Ausschalten zurück
        }
    }
}

/**
 * Fades out the LEDs in the strip by turning them off one by one.
 *
 * This function is part of the animation loop and is called repeatedly
 * by the LEDController. It checks if it is time to turn off the next LED
 * and if so, turns it off. The function also keeps track of the current
 * LED and checks if all LEDs have been turned off. If so, it stops the
 * animation and calls the callback function if provided.
 */
void LEDController::fadeDown() {
    // Überprüft, ob es Zeit ist, die nächste LED auszuschalten
    if (currentTime - lastUpdateTime >= 100) {
        setPixel(currentLED, 0);  // Schaltet die aktuelle LED aus
        strip.show();
        currentLED--;

        lastUpdateTime = currentTime;  // Setzt den Timer für den nächsten Schritt zurück

        // Wenn alle LEDs aus sind, wird die Animation beendet
        if (currentLED < 0) {
            stopAnimation();
            if (animationCallback) {
                animationCallback();
            }
        }
    }
}

/**
 * @brief Initializes an animation with the given color and callback.
 *
 * This function starts a new animation with the given color and an optional callback.
 * If the animation is already active, it stops the current animation and calls the
 * given callback before starting the new animation.
 *
 * @param colorValue The color value to use for the animation. This is an RGB
 * value in the format 0x00RRGGBB.
 * @param callback A callback function to be called when the animation is started.
 * If no callback is given, the function does nothing.
 */
void LEDController::initAnimation(uint32_t colorValue, Callback callback) {
    if (!animationActive) {
        color = colorValue;
        animationActive = true;
        currentLED = 0;             // Setzt den Startpunkt auf die erste LED
        lastUpdateTime = millis();  // Setzt den Startzeitpunkt für die Animation
    } else {
        stopAnimation();
        if (callback) callback();
    }
}

/**
 * @brief Stops the current animation and clears the LED strip.
 *
 * This function immediately stops the current animation and resets the
 * animation state to IDLE. It also clears the LED strip and turns off all LEDs.
 */
void LEDController::stopAnimation() {
    animationActive = false;
    state = IDLE;
    strip.clear();
    strip.show();  // Stellt sicher, dass alle LEDs ausgeschaltet sind
}

/**
 * @brief Clears the LED strip.
 *
 * This function deactivates any active animations and sets all LEDs to black, effectively clearing the strip.
 */
void LEDController::clear() {
    stopAnimation();
    for (int i = 0; i <= numLEDs; i++) {
        setPixel(i, 0);
    }
    strip.show();
}

/**
 * @brief Updates the LED strip based on the specified level.
 *
 * This function deactivates any active animations and clears the LED strip.
 * It sets the color of the LEDs up to the specified level, with different
 * colors indicating different levels. The colors are defined as follows:
 * - Level 0: Magenta
 * - Level 1: Red
 * - Level 2 and 3: Orange
 * - Level 4 and above: Green
 *
 * @param level The level up to which the LEDs should be lit, influencing the color.
 */
void LEDController::updateLEDs(int level) {
    animationActive = false;
    strip.clear();
    int color = strip.Color(0, 255, 0);

    if (level == 0) {
        color = strip.Color(255, 0, 255);
    } else if (level < 2) {
        color = strip.Color(255, 0, 0);
    } else if (level < 4) {
        color = strip.Color(255, 165, 0);
    }

    for (int i = 0; i <= level; i++) {
        setPixel(i, color);
    }

    strip.show();
}

/**
 * @brief Toggles the first LED on and off at specified intervals.
 *
 * This function manages the blinking of the first LED in the strip. When
 * the LED is on, it remains lit for 1 second. When off, it stays off for
 * 3 seconds. The toggle is controlled by the elapsed time since the last
 * update. The function uses a static variable to keep track of the LED state
 * between calls.
 */
void LEDController::blinkFirstLED() {
    static bool ledOn = true;  // LED-Zustand verfolgen

    if (ledOn && (currentTime - lastUpdateTime >= 1000)) {
        // Wenn die LED an ist und die Zeit für "an" abgelaufen ist
        setPixel(0, 0);  // LED aus
        strip.show();
        lastUpdateTime = currentTime;  // Zeitstempel aktualisieren
        ledOn = false;                 // Zustand wechseln
    } else if (!ledOn && (currentTime - lastUpdateTime >= 3000)) {
        // Wenn die LED aus ist und die Zeit für "aus" abgelaufen ist
        setPixel(0, color);  // LED an
        strip.show();
        lastUpdateTime = currentTime;  // Zeitstempel aktualisieren
        ledOn = true;                  // Zustand wechseln
    }
}

/**
 * @brief Sets the LED strip upside-down state.
 *
 * This function configures the LED strip to be either upside-down or not
 * based on the provided boolean value.
 *
 * @param value A boolean indicating the desired upside-down state.
 */
void LEDController::setUpsideDown(bool value) {
    upsideDown = value;
}

/**
 * @brief Maps an LED index to the actual index on the strip.
 *
 * Depending on the current upside-down state of the strip, this
 * function either returns the index as-is or calculates the mirrored
 * index on the strip.
 *
 * @param idx The index to be mapped.
 * @return The mapped index.
 */
uint16_t LEDController::mapIndex(uint16_t idx) {
    return upsideDown ? (numLEDs - 1 - idx) : idx;
}

/**
 * @brief Sets the color of an LED on the strip.
 *
 * This function sets the color of the LED at the given index
 * to the specified color. The index is not adjusted based on
 * the current upside-down state of the strip.
 *
 * @param idx The index of the LED to set.
 * @param c   The color to set the LED to.
 */
void LEDController::setPixel(uint16_t idx, uint32_t c) {
    strip.setPixelColor(mapIndex(idx), c);
}

/**
 * @brief Gets the current LED strip upside-down state.
 *
 * This function returns a boolean indicating whether the LED strip is currently
 * configured to be upside-down or not.
 *
 * @return A boolean indicating the current upside-down state.
 */
bool LEDController::isUpsideDown() {
    return upsideDown;
}

/**
 * @brief Updates the LED animation state.
 *
 * This function is responsible for updating the current animation
 * of the LED strip by calling the animation update routine. It
 * ensures that the LED animations are continuously refreshed
 * based on the current state.
 */
void LEDController::update() {
    updateAnimation();
}