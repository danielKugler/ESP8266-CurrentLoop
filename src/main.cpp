#include "ButtonController.h"
#include "CPortal.h"
#include "LEDController.h"
#include "LittleFSManager.h"
#include "Menu.h"
#include "NoiascaCurrentLoop.h"

// Current Loop Sensor Definitionen START
#define STEP_UP_PIN D5          // Pin der den Step-Up über die Transistoren schaltet
const byte sensorPin = A0;      // pin for the sensor
const uint16_t resistor = 150;  // pull down resistor in Ohm
const byte vref = 48;           // VREF in Volt*10  (Uno 16Mhz 50, ProMini 8Mhz 3V3, ) - be carefull, an UNO powered by usb will NOT HAVE 5V on VREF
// Hinweis: ein Arduino Uno der nur an USB hängt, hat selten eine VREF von exakt 5 Volt.
// Wenn also keine stabilen Werte zustande kommen, dann mal die Spannung an VREF messen
// und evtl. nur 48 oder 47 eintragen.
const uint16_t maxDisplayValue = 8;  // max value of sensor at 20mA

// Variable für den Sensorwert
static unsigned int sensorLevel = 0;
static unsigned int sensorAdc = 0;
static unsigned long measureInterval = 6;  // 1 Second
bool changingMeasureAdc = false;

static unsigned int measureTimestamp = millis();

CurrentLoopSensor pressureSensor(sensorPin, resistor, vref, maxDisplayValue);
// Current Loop Sensor Definitionen END

// LED Definitionen START
#define LED_PIN 2
#define NUM_LEDS 8
#define LED_BRIGHTNESS 50
// LED Definitionen END

// BUTTON Definitionen START
const int PIN_BUTTON_BLACK = D6;
const int PIN_BUTTON_RED = D7;
// BUTTON Definitionen END

// Erzeuge eine Instanz des LittleFSManagers mit debug option
LittleFSManager store(false);

// Erzeuge eine Instanz des LEDControllers
LEDController ledController(LED_PIN, NUM_LEDS, LED_BRIGHTNESS);

// Erzeuge eine Instanz des ButtonControllers
ButtonController buttons(PIN_BUTTON_BLACK, PIN_BUTTON_RED);

// Erstellen einer Instanz der Menu-Klasse
Menu menu(7);

// Erstellen einer Instanz der CaptivePortal-Klasse
CPortal portal;

/**
 * @brief Restart the ESP.
 *
 * This function waits for 1 second and then restarts the ESP using
 * ESP.restart(). It is used to reboot the ESP after a reset was
 * requested.
 */
void restart() {
    delay(1000);
    ESP.restart();
}

/**
 * @brief Resets the ESP to its factory settings.
 *
 * This function formats the LittleFS storage, erases the ESP configuration,
 * resets the Captive Portal and finally restarts the ESP.
 */
void onReset() {
    store.format();
    ESP.eraseConfig();
    portal.reset();
    restart();
}

/**
 * @brief Handles failure by triggering the LED failure animation.
 *
 * This function is called when a failure occurs, and it initiates
 * an animation on the LED strip to visually indicate a failure state
 * to the user.
 */
void handleFailure() {
    ledController.failureAnimation();
}

/**
 * @brief Handle black button press.
 *
 * This function is called when the black button is pressed. It
 * advances the menu to the next step.
 */
void handleBlackButtonPress() {
    // Serial.println("Main -> Schwarzer Button wurde gedrückt.");
    menu.nextStep();
}

/**
 * @brief Handle red button press.
 *
 * This function is called when the red button is pressed. It
 * confirms the current menu selection or activates the selected
 * menu item's function.
 */
void handleRedButtonPress() {
    // Serial.println("Main -> Roter Button wurde gedrückt.");
    menu.accept();
}

/**
 * @brief Handle changed measurement interval.
 *
 * This function is called when the measurement interval is changed
 * via the captive portal. It saves the new interval to the EEPROM
 * and updates the menu with the new interval.
 *
 * @param interval The new measurement interval.
 */
void handleIntervalChanged(unsigned int interval) {
    digitalWrite(STEP_UP_PIN, LOW);
    measureInterval = interval;
    store.save("interval", measureInterval);
    menu.setInterval(measureInterval);
}

/**
 * @brief Handle changed ADC value.
 *
 * This function is called when the ADC value is changed via the captive
 * portal. It saves the new value to the EEPROM and updates the sensor
 * with the new value.
 *
 * @param which The type of value to set. Either "min" or "max".
 * @param value The new value.
 */
void handleAdcChanged(String which, int value) {
    if (which == "min") {
        store.save("minAdcValue", value);
        pressureSensor.setMinAdcValue(value);
    } else if (which == "max") {
        store.save("maxAdcValue", value);
        pressureSensor.setMaxAdcValue(value);
    }
}

/**
 * @brief Callback function for the menu apply button.
 *
 * This function is called when the apply button is pressed in the menu.
 * It updates the LED strip with the current menu step and keeps the menu
 * active.
 */
void handleMenuApplyCallback() {
    ledController.menuIndicator(menu.currentStep());
    menu.keepAlive();
}

/**
 * @brief Handle LED direction change.
 *
 * This function is called when the LED direction needs to be changed.
 * It updates the LED strip's orientation, saves the new state to persistent
 * storage, and applies an animation to reflect the change.
 *
 * @param upsideDown A boolean indicating the new LED direction state.
 */
void handleLedDirectionChanged(boolean upsideDown) {
    Serial.println("Led direction changed to: " + String(upsideDown));
    ledController.setUpsideDown(upsideDown);
    store.save("upsideDown", upsideDown);
    ledController.applyAnimation();
}

/**
 * @brief Handle menu next step.
 *
 * This function is called when the menu next step button is pressed. It
 * updates the LED strip with the current menu step and keeps the menu
 * active.
 */
void handleMenuNextStep() {
    ledController.menuIndicator(menu.currentStep());
}

/**
 * @brief Handles menu selection.
 *
 * This function is called when a menu item is selected. It checks if the
 * first or second menu is active and exits the sub menu, updates the LED
 * strip with the current menu step, and keeps the menu active. If the
 * first menu is active, it also saves the current brightness to persistent
 * storage. If the second menu is active, it saves the current interval to
 * persistent storage. If neither menu is active, it just updates the LED
 * strip with the current menu step.
 */
void handleMenuSelected() {
    if (menu.isFirstMenuActive() == true) {
        menu.exitSubMenu();
        ledController.applyAnimation(handleMenuNextStep);
        store.save("brightness", menu.currentBrightness());
    } else if (menu.isSecondMenuActive() == true) {
        menu.exitSubMenu();
        ledController.applyAnimation(handleMenuNextStep);
        handleIntervalChanged(menu.currentInterval());
    } else {
        ledController.menuActiveAnimation(menu.currentStep());
    }
}

/**
 * @brief Handle success state.
 *
 * This function is called when a success state is reached. It applies an
 * animation to the LED strip to visually indicate the success state to the
 * user.
 */
void handleSuccess() {
    ledController.successAnimation();
}

/**
 * @brief Handle menu apply.
 *
 * This function is called when a menu item is selected. It checks which menu
 * item was selected and applies the corresponding action. If the selected
 * menu item is the first or second menu, it saves the current brightness and
 * interval to persistent storage, respectively. If the selected menu item is
 * the third or fourth menu, it sets the minimum and maximum sensor value,
 * respectively. If the selected menu item is the fifth menu, it toggles the
 * LED strip upside down. If the selected menu item is the sixth menu, it
 * restarts the machine. If the selected menu item is the seventh menu, it
 * exits the menu. In all cases, it applies an animation to the LED strip to
 * visually indicate the success state to the user.
 */
void handleMenuApply() {
    // Serial.println("Main -> Menu wurde akzeptiert | step: " + String(menu.currentStep()));
    if (menu.currentStep() == 3) {
        // SET MINIMUM SENSOR VALUE
        changingMeasureAdc = true;
        digitalWrite(STEP_UP_PIN, HIGH);
        pressureSensor.getValue();
        sensorAdc = pressureSensor.getAdc();
        store.save("minAdcValue", sensorAdc);
        pressureSensor.setMinAdcValue(sensorAdc);
        ledController.applyAnimation(handleMenuApplyCallback);
        delay(1000);
        changingMeasureAdc = false;
        digitalWrite(STEP_UP_PIN, LOW);

    } else if (menu.currentStep() == 4) {
        // SET MAXIMUM SENSOR VALUE
        changingMeasureAdc = true;
        digitalWrite(STEP_UP_PIN, HIGH);
        pressureSensor.getValue();
        sensorAdc = pressureSensor.getAdc();
        store.save("maxAdcValue", sensorAdc);
        pressureSensor.setMaxAdcValue(sensorAdc);
        ledController.applyAnimation(handleMenuApplyCallback);
        delay(1000);
        changingMeasureAdc = false;
        digitalWrite(STEP_UP_PIN, LOW);
    } else if (menu.currentStep() == 5) {
        // UPSIDEDOWN MENU
        bool upsideDown = ledController.isUpsideDown();
        ledController.setUpsideDown(!upsideDown);
        store.save("upsideDown", !upsideDown);
        ledController.applyAnimation(handleMenuApplyCallback);
    } else if (menu.currentStep() == 6) {
        // RESET MACHINE
        ledController.shutdownAnimation(onReset);
        onReset();
    } else if (menu.currentStep() == 7) {
        // RESTART MACHINE
        ledController.applyAnimation(restart);
    } else {
        // EVERYTHING ELSE
        ledController.applyAnimation(handleMenuApplyCallback);
    }
}

/**
 * @brief Handles menu exit.
 *
 * This function is called when the menu exit button is pressed. It clears the
 * LED strip and exits the menu.
 */
void handleMenuExit() {
    ledController.clear();
}

/**
 * @brief Enter the brightness menu.
 *
 * This function is called when the brightness menu is entered. It displays the
 * current brightness on the LED strip.
 */
void enterBrightnessMenu() {
    ledController.menuValueSelection(menu.currentBrightness());
}

/**
 * @brief Enter the interval menu.
 *
 * This function is called when the interval menu is entered. It displays the
 * current interval on the LED strip.
 */
void enterIntervalMenu() {
    ledController.menuValueSelection(menu.currentInterval());
}

/**
 * @brief Updates the LED brightness.
 *
 * This function sets the LED strip's brightness to the current value from the menu
 * and updates the LED display to reflect the current brightness level.
 */
void updateBrightness() {
    ledController.setBrightness(menu.currentBrightness());
    ledController.menuValueSelection(menu.currentBrightness());
}

/**
 * @brief Updates the LED strip's interval display.
 *
 * This function sets the LED strip's display to reflect the current interval
 * selected in the menu.
 */
void updateInterval() {
    ledController.menuValueSelection(menu.currentInterval());
}

/**
 * @brief Converts a menu-selected interval into milliseconds.
 *
 * This function translates an interval selected from the menu into
 * its corresponding value in milliseconds. The intervals represent
 * different durations from always active (1 second) to 12 hours.
 *
 * @param interval The interval value from the menu, ranging from 1 to 6.
 *                 - 1: 4 hours (43,200 seconds)
 *                 - 2: 1 hour (28,800 seconds)
 *                 - 3: 30 minutes (1,800 seconds)
 *                 - 4: 5 minutes (300 seconds)
 *                 - 5: 30 seconds
 *                 - 6: Always active (1 second)
 *
 * @return The interval duration in milliseconds. Defaults to 1 second
 *         (1,000 milliseconds) if an invalid interval is provided.
 */
unsigned long timedInterval(unsigned int interval) {
    // 6 = (1) 		Immer aktiv ( 1 Sekunde )
    // 5 = (30)		30 Sekunden
    // 4 = (300)	5 Minute
    // 3 = (1.800)	30 Minuten
    // 2 = (3.600)	1 Stunde
    // 1 = (14.400)	4 Stunden
    switch (interval) {
        case 1:
            return 14400 * 1000;
        case 2:
            return 3600 * 1000;
        case 3:
            return 1800 * 1000;
        case 4:
            return 300 * 1000;
        case 5:
            return 30 * 1000;
        default:
            return 1000;
    }
}

/**
 * @brief Arduino setup function.
 *
 * This function is called once when the Arduino boots up. It sets up the
 * serial console, initializes the LED strip, loads saved settings from EEPROM,
 * sets up the pressure sensor, and starts the Captive Portal.
 */
void setup() {
    //Serial.setDebugOutput(true);
    Serial.begin(115200);

    store.begin();

    static unsigned int savedBrightness = store.read("brightness", 5);
    measureInterval = store.read("interval", 6);

    // ------------------- LED STRIP -------------------
    static bool upsideDown = store.read("upsideDown", 1);
    ledController.setUpsideDown(upsideDown);
    ledController.setBrightness(savedBrightness);
    ledController.startAnimation();

    // ------------------- SENSOR -------------------
    pinMode(STEP_UP_PIN, OUTPUT);
    static unsigned int minAdcValue = store.read("minAdcValue", 0);
    static unsigned int maxAdcValue = store.read("maxAdcValue", 0);
    if (minAdcValue != 0) {
        pressureSensor.setMinAdcValue(minAdcValue);
    }
    if (maxAdcValue != 0) {
        pressureSensor.setMaxAdcValue(maxAdcValue);
    }
    pressureSensor.begin();
    // Die Library hat auch einen eigenen Check, der die erfassten Parameter auf Plausibilität überprüft und beispielsweise vor falschen Widerstandswerten warnt. Üblicherweise braucht man diesen Check nur beim ersten Sketch und kann im laufenden Betrieb auskommentiert werden:
    // pressureSensor.check();  // remove this line if check shows no error, will save about 320 bytes program memory (flash)

    // ------------------- Captive Portal -------------------
    portal.update(0, 0, pressureSensor.getMinAdcValue(), pressureSensor.getMaxAdcValue(), measureInterval, 0, upsideDown);
    portal.onIntervalChanged(handleIntervalChanged);
    portal.onAdcChanged(handleAdcChanged);
    portal.onLedDirectionChanged(handleLedDirectionChanged);
    portal.begin();

    // ------------------- BUTTONS -------------------
    buttons.onButtonBlackPressed(handleBlackButtonPress);
    buttons.onButtonRedPressed(handleRedButtonPress);

    // ------------------- MENU -------------------
    menu.setBrightness(savedBrightness);
    menu.setInterval(measureInterval);

    menu.onNextStep(handleMenuNextStep);
    menu.onMenuExit(handleMenuExit);
    menu.onSelect(handleMenuSelected);
    menu.onApply(handleMenuApply);
    menu.onMenuFirstEnter(enterBrightnessMenu);
    menu.onMenuFirstChange(updateBrightness);
    menu.onMenuSecondEnter(enterIntervalMenu);
    menu.onMenuSecondChange(updateInterval);
}

/**
 * @brief Reading the current loop sensor at a specified interval.
 *
 * It:
 * Checks if the interval has passed since the last measurement.
 * If so, it enables the step-up transistor and measures the sensor value.
 * If the measurement is successful, it updates the sensor level, ADC value, and timestamp.
 * If the menu is not active, it updates the LED display based on the sensor level.
 */
void checkSensor(unsigned long interval) {
    unsigned long stepUpDelay = 1000;
    static unsigned long lastTimeMeasure = 0;
    unsigned long currentTimeMeasure = millis();

    if (changingMeasureAdc == false && currentTimeMeasure - lastTimeMeasure >= interval) {
        digitalWrite(STEP_UP_PIN, HIGH);  // Schalte den Stepup über den Transistoren ein
        if (interval == 1000 || (currentTimeMeasure - lastTimeMeasure >= interval + stepUpDelay)) {
            sensorLevel = pressureSensor.getValue();
            sensorAdc = pressureSensor.getAdc();
            measureTimestamp = currentTimeMeasure;
            lastTimeMeasure = currentTimeMeasure;
            // Serial.println("Time in minutes: " + String(currentTimeMeasure / 60000) + " ");
            // Serial.println("Sensor: " + String(sensorLevel) + " ADC: " + String(sensorAdc));
            if (interval > 1000) {
                digitalWrite(STEP_UP_PIN, LOW);  // Schalte den Stepup über den Transistoren aus
            }
            if (menu.isMenuActive() == false) {
                if (sensorLevel == 0) {
                    ledController.clear();
                    ledController.blinkRed();
                } else {
                    ledController.updateLEDs(sensorLevel);
                }
            }
        }
    }
}

/**
 * The main loop of the application.
 *
 * Calls {@link checkSensor} with the current measure interval,
 * then updates the captive portal with the current sensor values,
 * and finally updates the led controller, buttons and menu.
 */
void loop() {
    checkSensor(timedInterval(measureInterval));
    boolean upsideDown = ledController.isUpsideDown();
    portal.update(sensorLevel, sensorAdc, pressureSensor.getMinAdcValue(), pressureSensor.getMaxAdcValue(), measureInterval, measureTimestamp, upsideDown);
    ledController.update();
    buttons.update();
    menu.update();
}
