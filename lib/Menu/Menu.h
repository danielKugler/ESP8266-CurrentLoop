#include <Arduino.h>
#include <functional>

/**
 * Das Menü ist im Anzeige-Controller und Steuert eigenschaften des Sensors.
 * 
 * * Schwarzer Knopf:	Option
 * * Roter Knopf:		Accept
 * 
 * Drücken von Rot/Accept steigt in das Menü ein. LED 0 & 7 leuchten.
 * Nachdem es der erste Register im Menü ist, leuchtet LED 1 auch.
 * 
 * Schwarz/Option wechselt das nächste Register. Es gibt max. 6 Register (nur 8 LED, zwei belegt für Menü Indikation).
 * Rot/Accept aktiviert einen Register, oder dessen Funktion.
 * 
 * In einem aktivierten Register löst Rot die Aktion aus, wärend Schwarz den Register wieder verlässt.
 * 
 * Wird innerhalb von 10 Sekunden nichtmehr im Menü navigiert, wird dieses beendet.
 * 
 * 1. Register: Erneuter Verbindungsversuch zum Sensor (Falls nötig)
 * 2. Register: Reset des Sensors
 * 3. Register: Einstellung von Sensor-MIN
 * 4. Register: Einstellung von Sensor-MAX
 */


class Menu {
   public:
    using Callback = std::function<void()>;

    Menu(int numSteps);

    void update();

    int nextBrightness();     		// returns step
    int nextInterval();     		// returns step
	void setBrightness(int value);
	void setInterval(int value);

    int nextStep();     		// returns step
    int accept();    			// returns step
    int currentStep();  		// returns step
    unsigned int currentBrightness();  		// returns brightness
    unsigned int currentInterval();  		// returns brightness
    bool isFirstMenuActive();
    bool isSecondMenuActive();
	bool isMenuActive();

	void exit();
    void exitSubMenu();

	void keepAlive();

	// Callback functions
	void onSelect(Callback callback);
	void onApply(Callback callback);
	void onNextStep(Callback callback);
	void onMenuExit(Callback callback);

	void onMenuFirstEnter(Callback callback);
	void onMenuFirstChange(Callback callback);
	void onMenuSecondEnter(Callback callback);
	void onMenuSecondChange(Callback callback);

   private:
    unsigned int step = 0;				// Menu not active 
    unsigned int selectedStep = 0;		// Selected step to apply
    unsigned int numSteps = 0;			// Number of menu steps. Max. 6
    unsigned long keepMenuOpenDelay = 8000;
    unsigned long keepMenuSelectedOpenDelay = 20000;
	bool active = false;

	uint32_t lastUpdateTime;
	uint32_t currentTime;

	void resetTimer();
	void checkTimer();
	void waitingForInputOrCancelMenu();

	Callback selectCallback;
	Callback applyCallback;
	Callback nextStepCallback;
	Callback exitMenuCallback;

	unsigned int brightness = 1;			// Current brightness
	unsigned int interval = 1;			// Current interval
	Callback firstMenuEnterCallback;
	Callback firstMenuChangeCallback;
	Callback secondMenuEnterCallback;
	Callback secondMenuChangeCallback;

};