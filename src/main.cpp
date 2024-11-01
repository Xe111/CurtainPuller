/*    TODO
1. reconstruct the code into class style(abstarct the functions to a class)
2. implement related contents with curtain_state
3. change delay to other way
4. configable wifi ? 
*/

#pragma region Preparation

#include <Arduino.h>
#include <functional>

#define BLINKER_WIFI
#include <Blinker.h>

char auth[] = "b06b42bba5c3";
char ssid[] = "WIFI_C912";
char pswd[] = "17705003103";

#pragma endregion Preparation

#pragma region CP dec
class CurtainPuller
{
private:
    enum PIN
    {
        PWMA = A17,
        AIN2 = A16,
        AIN1 = A15,
        STBY = A14
    };

    int time_pullup; // sec
    double pulldown_coef;
    char motion_state;    // h for halt,u for pullup,d for pulldn
    double curtain_state; // 1 for fully closed, 0 for fully opened
    bool use_led;
    struct Blk
    {
        BlinkerButton btn_up;
        BlinkerButton btn_dn;
        BlinkerButton btn_dbg;

        Blk() : btn_up("btn_up"), btn_dn("btn_dn"), btn_dbg("btn_dbg") {}
        ~Blk() = default;

    } blk;

    static CurtainPuller *instance;

    void _btn_up(const String &state);
    void _btn_dn(const String &state);
    void _btn_dbg(const String &state);
    static void _btn_up_wrapper(const String& state);
    static void _btn_dn_wrapper(const String& state);
    static void _btn_dbg_wrapper(const String& state);

    inline void halt();
    inline void pullup();
    inline void pulldn();

    void _delay(bool is_pullingup, double percentage = 1.0);

public:
    CurtainPuller() : blk(), time_pullup(90), pulldown_coef(0.9), motion_state('h'), curtain_state(1.0), use_led(true)
    {
        instance = this;
    }
    ~CurtainPuller() = default;

    void init();
    void attach();
};
using CP = CurtainPuller;

CP cp;

#pragma region Setup&Loop

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  BLINKER_DEBUG.stream(Serial);
  BLINKER_DEBUG.debugAll();

  cp.init();
  Blinker.begin(auth, ssid, pswd);
  cp.attach();
}

void loop()
{
  // put your main code here, to run repeatedly:
  Blinker.run();
}

#pragma endregion Setup&Loop

#pragma region CP def

CP* CP::instance = nullptr;

void CP::init()
{
    pinMode(PWMA, OUTPUT);
    pinMode(STBY, OUTPUT);
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(PWMA, HIGH);
    digitalWrite(STBY, HIGH);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    digitalWrite(LED_BUILTIN, LOW);
}
void CP::attach()
{
    blk.btn_up.attach(_btn_up_wrapper);
    blk.btn_dn.attach(_btn_dn_wrapper);
    blk.btn_dbg.attach(_btn_dbg_wrapper);
}
void CP::halt()
{
    if (motion_state == 'h')
        return;
    motion_state = 'h';
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    if (use_led)
        digitalWrite(LED_BUILTIN, LOW);
    BLINKER_LOG("halt");
}
void CP::pullup()
{
    if (motion_state != 'h')
    {
        BLINKER_LOG("the motor is working!!!");
        return;
    }
    motion_state = 'u';

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    if (use_led)
        digitalWrite(LED_BUILTIN, HIGH);
    BLINKER_LOG("pullup");
}
void CP::pulldn()
{
    if (motion_state != 'h')
    {
        BLINKER_LOG("the motor is working!!!");
        return;
    }
    motion_state = 'd';

    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    if (use_led)
        digitalWrite(LED_BUILTIN, HIGH);
    BLINKER_LOG("pulldn");
}
void CP::_delay(bool is_pullingup, double percentage)
{
    int time_ = time_pullup * 1000 * percentage;
    if (!is_pullingup)
        time_ *= pulldown_coef;
    delay(time_);
    curtain_state += -(2 * is_pullingup - 1) * percentage;
}

void CP::_btn_up(const String &state)
{
    if (state == "tap")
    {
        pullup();
        _delay(true);
        halt();
    }
    else if (state == "press")
    {
        pullup();
    }
    else if (state == "pressup")
    {
        halt();
    }
    else
    {
        BLINKER_LOG("no such state!");
    }
}

void CP::_btn_dn(const String &state)
{
    if (state == "tap")
    {
        pulldn();
        _delay(false);
        halt();
    }
    else if (state == "press")
    {
        pulldn();
    }
    else if (state == "pressup")
    {
        halt();
    }
    else
    {
        BLINKER_LOG("no such state!");
    }
}

void CP::_btn_dbg(const String &state)
{

    BLINKER_LOG("get button state: ", state);
    BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
#if false
  String msg = "";
  if (digitalPinIsValid(PWMA))
  {
    msg += "\nPWMA is valid\n";
  }
  else
  {
    msg += "PWMA is not valid\n";
  }
  if (digitalPinIsValid(STBY))
  {
    msg += "STBY is valid\n";
  }
  else
  {
    msg += "STBY is not valid\n";
  }
  if (digitalPinIsValid(AIN1))
  {
    msg += "AIN1 is valid\n";
  }
  else
  {
    msg += "AIN1 is not valid\n";
  }
  if (digitalPinIsValid(AIN2))
  {
    msg += "AIN2 is valid\n";
  }
  else
  {
    msg += "AIN2 is not valid\n";
  }
  if (digitalPinCanOutput(PWMA))
  {
    msg += "PWMA can output\n";
  }
  else
  {
    msg += "PWMA can not output\n";
  }
  if (digitalPinCanOutput(STBY))
  {
    msg += "STBY can output\n";
  }
  else
  {
    msg += "STBY can not output\n";
  }
  if (digitalPinCanOutput(AIN1))
  {
    msg += "AIN1 can output\n";
  }
  else
  {
    msg += "AIN1 can not output\n";
  }
  if (digitalPinCanOutput(AIN2))
  {
    msg += "AIN2 can output\n";
  }
  else
  {
    msg += "AIN2 can not output\n";
  }

    BLINKER_LOG("DEBUG_INFO:", msg);
#endif
    BLINKER_LOG("DEBUG_INFO:this function is still preparing...");
    BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
}

void CP::_btn_up_wrapper(const String &state)
{
    if (instance)
        instance->_btn_up(state);
}
void CP::_btn_dn_wrapper(const String &state)
{
    if (instance)
        instance->_btn_dn(state);
}
void CP::_btn_dbg_wrapper(const String &state)
{
    if (instance)
        instance->_btn_dbg(state);
}
#pragma endregion CP