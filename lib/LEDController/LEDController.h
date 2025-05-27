#include <Adafruit_NeoPixel.h>

class LEDController {
   public:
    // Typdefinition für Callback-Funktionen
    using Callback = std::function<void()>;

    // Konstruktor und Initialisierung
    LEDController(uint8_t pin, uint16_t numLEDs, int brightness);
    void setBrightness(int brightness);
    void startAnimation();
    void applyAnimation(Callback callback = nullptr);
    void successAnimation(Callback callback = nullptr);
    void failureAnimation();
    void update();
    void stopAnimation();
	void shutdownAnimation(Callback callback = nullptr);
	void menuIndicator(int step);			// indicating the current menu step
	void menuValueSelection(int value);	// indicating the current brightness
	void menuActiveAnimation(int step);		// indicating the current menu step
    void clear();
    void blinkRed();
    void updateLEDs(int level);

    void setUpsideDown(bool u);
    bool isUpsideDown();

   private:
    Adafruit_NeoPixel strip;
    uint16_t numLEDs;
    uint32_t colorApply = strip.Color(185, 50, 255);
    uint32_t colorSuccess = strip.Color(0, 255, 0);
    uint32_t colorMenuIndicator = strip.Color(255, 255, 0);
	uint32_t colorMenuStep = strip.Color(0, 140, 255);
    uint32_t color;
    uint32_t lastUpdateTime;
    uint32_t currentTime;
    uint32_t animationStartTime;
    int brightness = 255;
    bool animationActive;
    bool upsideDown = true;
    enum AnimationState { IDLE,
                          FADE_UP,
                          FADE_DOWN,
                          BLINK_FIRST,
						  MENU_ACTIVE,
						  BLINK_RED_2500 };
    int state = 0;
    int currentLED = 0;  // Verfolgt die aktuelle LED in der Sequenz
	int menuStep = 1;
	bool subMenuActive = false;
    uint32_t animationColor = 0;
    void updateAnimation();
    void initAnimation(uint32_t colorValue, Callback callback = nullptr);
    void fadeUp();
    void fadeDown();
    void blinkFirstLED();
    void blink(int startLED, int endLED, uint32_t time, uint32_t timeTotal = 0);
    void blinkMenu();

   	uint16_t mapIndex(uint16_t idx);
    void setPixel(uint16_t idx, uint32_t c);

	Callback animationCallback;
};