#include "ButtonController.h"

ButtonController::ButtonController(int pinBlack, int pinRed)
    : pinBlack(pinBlack), pinRed(pinRed) { init(); }

void ButtonController::init() {
    pinMode(pinBlack, INPUT_PULLUP);
    pinMode(pinRed, INPUT_PULLUP);

    // Initialize Bounce-Objects
    buttonBlack.attach(pinBlack);
    buttonRed.attach(pinRed);
    buttonBlack.interval(50);  // Debounce-Zeit in ms
    buttonRed.interval(50);
}

void ButtonController::update() {
    // Update Bounce-Objects
    buttonBlack.update();
    buttonRed.update();

    bool blackPressed = buttonBlack.read() == LOW;
    bool redPressed = buttonRed.read() == LOW;

    // Trigger individual events only if the combined event is not running
    if (buttonBlack.fell() && !redPressed) {
        if (blackPressedCallback) {
            blackPressedCallback();
        }
    }

    if (buttonRed.fell() && !blackPressed) {
        if (redPressedCallback) {
            redPressedCallback();
        }
    }
}

void ButtonController::onButtonBlackPressed(Callback callback) {
    blackPressedCallback = callback;
}

void ButtonController::onButtonRedPressed(Callback callback) {
    redPressedCallback = callback;
}
