#include "Menu.h"

Menu::Menu(int numSteps) : numSteps(numSteps) {}

void Menu::keepAlive() {
    resetTimer();
}
int Menu::nextStep() {
    if (active == true) {
        keepAlive();
        if (selectedStep == 1) {
            return nextBrightness();
        }
        if (selectedStep == 2) {
            return nextInterval();
        }
        if (step > 0 && selectedStep == step) {
            selectedStep = 0;
        } else {
            step = step + 1;
            if (step > numSteps) {
                step = 1;
            }
        }
        if (nextStepCallback) {
            nextStepCallback();
        }
    }
    return step;
}
void Menu::setBrightness(int value) {
    brightness = value;
}
void Menu::setInterval(int value) {
    interval = value;
}
int Menu::nextBrightness() {
    keepAlive();
    brightness = brightness + 1;
    if (brightness > numSteps - 1) {
        brightness = 1;
    }
    if (firstMenuChangeCallback) {
        firstMenuChangeCallback();
    }
    return brightness;
}
int Menu::nextInterval() {
    keepAlive();
    interval = interval + 1;
    if (interval > numSteps - 1) {
        interval = 1;
    }
    if (secondMenuChangeCallback) {
        secondMenuChangeCallback();
    }
    return interval;
}
int Menu::accept() {
    if (!active) {
        active = true;
        return nextStep();
    } else {
        resetTimer();
        if (step != 1 && step != 2 && selectedStep != 0) {
			if (applyCallback) {
				applyCallback();
			}
            selectedStep = 0;
        } else {
            if (step == 1 && selectedStep != step) {
                selectedStep = 1;
                if (firstMenuEnterCallback) {
                    firstMenuEnterCallback();
                }
            } else if (step == 2 && selectedStep != step) {
                selectedStep = 2;
                if (secondMenuEnterCallback) {
                    secondMenuEnterCallback();
                }
            } else {
				selectedStep = step;
                if (selectCallback) {
                    selectCallback();
                }
            }
        }
        return step;
    }
}
int Menu::currentStep() {
    return step;
}
unsigned int Menu::currentBrightness() {
    return brightness;
}
unsigned int Menu::currentInterval() {
    return interval;
}
bool Menu::isFirstMenuActive() {
	return selectedStep == 1;
}
bool Menu::isSecondMenuActive() {
	return selectedStep == 2;
}
bool Menu::isMenuActive() {
    return active;
}
void Menu::exitSubMenu() {
    keepAlive();
    selectedStep = 0;
}
void Menu::exit() {
    step = 0;
    selectedStep = 0;
    active = false;
    if (exitMenuCallback) {
        exitMenuCallback();
    }
}
void Menu::resetTimer() {
    lastUpdateTime = millis();  // Zeit des Resets
}
void Menu::checkTimer() {
    if (!active) {
        return;  // Timer läuft nicht
    }

    currentTime = millis();
    unsigned long wait = selectedStep == 0 ? keepMenuOpenDelay : keepMenuSelectedOpenDelay;
    if (currentTime - lastUpdateTime >= wait) {
        if (selectedStep == 0) {
            exit();
        } else {
            resetTimer();
            selectedStep = 0;
        }
    }
}
void Menu::onSelect(Callback callback) {
    selectCallback = callback;
}
void Menu::onApply(Callback callback) {
    applyCallback = callback;
}
void Menu::onNextStep(Callback callback) {
    nextStepCallback = callback;
}
void Menu::onMenuExit(Callback callback) {
    exitMenuCallback = callback;
}
void Menu::onMenuFirstEnter(Callback callback) {
    firstMenuEnterCallback = callback;
}
void Menu::onMenuFirstChange(Callback callback) {
    firstMenuChangeCallback = callback;
}
void Menu::onMenuSecondEnter(Callback callback) {
    secondMenuEnterCallback = callback;
}
void Menu::onMenuSecondChange(Callback callback) {
    secondMenuChangeCallback = callback;
}
void Menu::update() {
    checkTimer();
}