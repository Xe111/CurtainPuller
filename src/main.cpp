#define BLINKER_WIFI
#include <Blinker.h>
#include <Preferences.h>
#include "device_config.h"

#pragma region 全局变量区

enum PIN
{
    PWMA = A17,
    AIN2 = A16,
    AIN1 = A15,
    STBY = A14
}; // 引脚对应关系

constexpr double pullup_time = 71;            // 上拉时间
constexpr double pulldn_coef = 1.00;          // 下拉时间系数
constexpr double default_curtain_state = 100; // 默认窗帘状态

bool use_led = true;                          // 是否启用LED灯
char motion_state = '0';                      // 窗帘运动状态: '0':停机; '-':正在上拉; '+':正在下拉;
double curtain_state = default_curtain_state; // 窗帘位置状态:  0 :全开; 100:全关
double target_state = default_curtain_state;  // 窗帘目标状态

Preferences preferences; // 断电状态保存用到的库

BlinkerSlider sld_sta("ran_sta"); // 当前状态滑动条
BlinkerSlider sld_tg("ran_tg");   // 目标状态滑动条
BlinkerButton btn_led("btn_led"); // LED开关按钮
BlinkerButton btn_up("btn_up");   // 上拉按钮
BlinkerButton btn_dn("btn_dn");   // 下拉按钮
BlinkerButton btn_dbg("btn_dbg"); // 调试按钮
BlinkerButton btn_rst("btn_rst"); // 复位按钮

#pragma endregion 全局变量区

#pragma region 函数声明区

void _btn_up(const String &state); // 按钮拉起窗帘的回调函数
void _btn_dn(const String &state); // 按钮拉下窗帘的回调函数
void _sld_tg(int32_t data);        // 目标状态滑动条响应函数

void _btn_dbg(const String &state); // 按钮调试的回调函数
void _btn_rst(const String &state); // 按钮复位的回调函数
void _btn_led(const String &state); // 按钮复位的回调函数

void halt();   // 停止窗帘
void pullup(); // 拉起窗帘
void pulldn(); // 拉下窗帘

    void init();                      // 初始化
    void savePreferences();           // 保存数据到EEPROM
    void loadPreferences();           // 从EEPROM加载数据
    void rtData();                    // 实时数据上传函数, 每秒执行一次
    void DataStorage();               // 历史数据存储函数, 每分钟执行一次

void show_motion_state(); // 显示窗帘运动状态

void attach_all() // 绑定所有回调函数
{
    btn_up.attach(_btn_up); // 绑定按钮回调函数
    btn_dn.attach(_btn_dn);
    btn_dbg.attach(_btn_dbg);
    btn_rst.attach(_btn_rst);
    btn_led.attach(_btn_led);
    sld_tg.attach(_sld_tg);
    Blinker.attachDataStorage(DataStorage); // 绑定历史数据存储函数
    Blinker.attachRTData(rtData);           // 绑定实时数据函数
}

#pragma endregion 函数声明区

#pragma region 主程序区

void setup()
{
    Serial.begin(115200);            // 初始化串口
    BLINKER_DEBUG.stream(Serial);    // 串口输出调试信息
    Blinker.begin(auth, ssid, pswd); // 初始化blinker
    attach_all();                    // 绑定所有回调函数
    init();                          // 初始化
}

void loop()
{
    Blinker.run();
}

#pragma endregion 主程序区

#pragma region 函数定义区

void init() // 初始化
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

    // 加载上次保存的状态
    loadPreferences();
    // 正确显示初始状态
    if (use_led) btn_led.print("on");
    else         btn_led.print("off");
    sld_tg.print(target_state);

}
void loadPreferences() // 加载数据到EEPROM
{
    preferences.begin("my_preferences", false);
    use_led = preferences.getBool("use_led", true); 
    motion_state = preferences.getChar("motion_state", '0'); 
    curtain_state = preferences.getDouble("curtain_state", 50.0); 
    target_state = preferences.getDouble("target_state", 50.0); 
    
    BLINKER_LOG("=========== 加载上次状态 =============");
    BLINKER_LOG("use_led: ", use_led);
    BLINKER_LOG("curtain_state: ", curtain_state);
    BLINKER_LOG("target_state: ", target_state);
    BLINKER_LOG("motion_state: ", motion_state);
    BLINKER_LOG("=========== 加载上次状态 =============");
}
void savePreferences() // 保存数据到EEPROM
{
    preferences.putBool("use_led", use_led);
    preferences.putChar("motion_state", motion_state);
    preferences.putDouble("curtain_state", curtain_state);
    preferences.putDouble("target_state", target_state);
  
}

void halt() // 停止窗帘
{
    motion_state = '0';
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    target_state = curtain_state;
    sld_tg.print(curtain_state);
}
void pullup() // 上拉窗帘
{
    motion_state = '-';
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    if (use_led)
        digitalWrite(LED_BUILTIN, HIGH);
}
void pulldn() // 下拉窗帘
{
    motion_state = '+';
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    if (use_led)
        digitalWrite(LED_BUILTIN, HIGH);
}

void _btn_up(const String &state) // 按钮拉起窗帘的回调函数
{
    if (state == "tap" && motion_state == '0' || state == "press")
    {
        target_state = 0;
        sld_tg.print(target_state);
        pullup();
    }
    else
    {
        halt();
    }
}
void _btn_dn(const String &state) // 按钮拉下窗帘的回调函数
{
    if (state == "tap" && motion_state == '0' || state == "press")
    {
        target_state = 100;
        sld_tg.print(target_state);
        pulldn();
    }
    else
    {
        halt();
    }
}
void _sld_tg(int32_t data) // 目标状态滑动条响应函数
{
    target_state = data;
    if (target_state > curtain_state)
    {
        pulldn();
    }
    else if (target_state < curtain_state)
    {
        pullup();
    }
    else
    {
        halt();
    }
}

void _btn_led(const String &state) // LED灯按钮的回调函数
{
    if (state == "on")
    {
        use_led = true;
        btn_led.print("on");
        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
        use_led = false;
        btn_led.print("off");
        digitalWrite(LED_BUILTIN, LOW);
    }
}
void _btn_dbg(const String &state) // 按钮调试的回调函数
{
    BLINKER_LOG("=========== Debug =============");
    BLINKER_LOG("use_led: ", use_led);
    BLINKER_LOG("curtain_state: ", curtain_state);
    BLINKER_LOG("target_state: ", target_state);
    BLINKER_LOG("motion_state: ", motion_state);
    BLINKER_LOG("=========== Debug =============");

    Blinker.print("curtain_state", curtain_state);
    Blinker.print("target_state", target_state);
    Blinker.print("motion_state", motion_state);
}
void _btn_rst(const String &state) // 按钮复位的回调函数
{
    motion_state = '0';
    curtain_state = default_curtain_state;
    target_state = default_curtain_state;
    halt();
}

void DataStorage() // 历史数据函数, 每分钟执行一次
{
    BLINKER_LOG("DataStorage: ", curtain_state);
    Blinker.dataStorage("ran_sta", curtain_state);
}
void rtData() // 实时数据函数, 每秒执行一次
{
    constexpr static double pullup_speed = 100 / pullup_time;
    constexpr static double pulldn_speed = pullup_speed * pulldn_coef;

    Blinker.sendRtData("ran_sta", curtain_state);
    Blinker.printRtData();
    savePreferences(); // 保存数据到EEPROM, 方便重启后恢复状态
    // 根据当前状态执行相应动作
    if (motion_state == '+')
    {
        curtain_state += pulldn_speed;
        pulldn();
        if (curtain_state >= target_state)
        {
            curtain_state = target_state;
            halt();
        }
    }
    if (motion_state == '-')
    {
        curtain_state -= pullup_speed;
        pullup();
        if (curtain_state <= target_state)
        {
            curtain_state = target_state;
            halt();
        }
    }
    if (motion_state == '0')
    {
        digitalWrite(LED_BUILTIN, LOW);
    }

    show_motion_state();
}

void show_motion_state()
{
    const static String blinker_grey("#959595"); // blinker灰色
    const static String blinker_blue("#066EEF"); // blinker蓝色
    
    static char last_motion_state = '0';
    
    constexpr static int resend_times = 3;
    static int resend_count = 0;

    if(motion_state != last_motion_state)
    {
        last_motion_state = motion_state;
        if (motion_state == '-')
        {
            btn_up.color(blinker_blue);
            btn_dn.color(blinker_grey);
        }
        else if (motion_state == '+')
        {
            btn_up.color(blinker_grey);
            btn_dn.color(blinker_blue);
        }
        else if (motion_state == '0')
        {
            btn_up.color(blinker_grey);
            btn_dn.color(blinker_grey);
        }
        
        resend_count = resend_times;
    }

    if(resend_count > 0)
    {
        resend_count--;
        btn_up.print();
        btn_dn.print();
    }
}

#pragma endregion 函数定义区