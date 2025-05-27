#include <Bounce2.h>

#include <functional>

class ButtonController {
   public:
    using Callback = std::function<void()>;

    ButtonController(int pinBlack, int pinRed);

    void update();

    // Event-Handler-Setter
    void onButtonBlackPressed(Callback callback);
    void onButtonRedPressed(Callback callback);

   private:
    // Pins und Bounce-Objekte für die Buttons
    int pinBlack;
    int pinRed;

    void resetFlags();

    Bounce buttonBlack;
    Bounce buttonRed;

    // Callback-Funktionen
    Callback blackPressedCallback;
    Callback redPressedCallback;

   protected:
    void init();
};