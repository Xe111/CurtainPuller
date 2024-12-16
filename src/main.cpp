#define BLINKER_WIFI
#include <Blinker.h>

#pragma region 数据准备区

    char auth[] = "b06b42bba5c3";       // NodeMCU-32S
    //char auth[] = "d25e869afcac";       // ESP32-DevKitC

    #define wifi_choice 2

    #if (wifi_choice == 1)
    char ssid[] = "WIFI_C912";
    char pswd[] = "17705003103"; // 配置wifi
    #endif

    #if (wifi_choice == 2)
    char ssid[] = "Xe123";
    char pswd[] = "91215225825"; // 配置wifi
    #endif

    #if (wifi_choice == 3)
    char ssid[] = "Note12";
    char pswd[] = "12345678";
    #endif

    #if (wifi_choice == 4)
    char ssid[] = "Win11";
    char pswd[] = "12345678";
    #endif

#pragma endregion 数据准备区


#pragma region 全局变量区

    enum PIN { PWMA = A17, AIN2 = A16, AIN1 = A15, STBY = A14 };// 引脚对应关系
    bool use_led = true;             // 是否启用LED灯
    char motion_state = '0';         // 窗帘运动状态: '0':停机; '-':正在上拉; '+':正在下拉;
    double curtain_state = 50;       // 窗帘位置状态:  0 :全开; 100:全关
    double target_state = 50;        // 窗帘目标状态
    constexpr double pullup_time = 10;
    constexpr double pulldn_coef = 1.2;

#pragma endregion 全局变量区


#pragma region 按钮注册与函数声明区

    BlinkerSlider sld_sta("ran_sta"); // 当前状态滑动条
    BlinkerSlider sld_tg("ran_tg");   // 目标状态滑动条
    BlinkerButton btn_led("btn_led"); // LED开关按钮
    BlinkerButton btn_up("btn_up");   // 上拉按钮
    BlinkerButton btn_dn("btn_dn");   // 下拉按钮
    BlinkerButton btn_dbg("btn_dbg"); // 调试按钮
    BlinkerButton btn_rst("btn_rst"); // 复位按钮

    void _btn_up(const String &state);  // 按钮拉起窗帘的回调函数
    void _btn_dn(const String &state);  // 按钮拉下窗帘的回调函数
    void _sld_tg(int32_t data);         // 目标状态滑动条响应函数
    
    void _btn_dbg(const String &state); // 按钮调试的回调函数
    void _btn_rst(const String &state); // 按钮复位的回调函数
    void _btn_led(const String &state); // 按钮复位的回调函数

    void halt();                     // 停止窗帘
    void pullup();                   // 拉起窗帘
    void pulldn();                   // 拉下窗帘

    void init();                      // 初始化
    void rtData();                    // 实时数据上传函数, 每秒执行一次
    void DataStorage();               // 历史数据存储函数, 每分钟执行一次
    void attach_all()                 // 绑定所有回调函数
    {
        btn_up.attach(_btn_up);          // 绑定按钮回调函数
        btn_dn.attach(_btn_dn);
        btn_dbg.attach(_btn_dbg);
        btn_rst.attach(_btn_rst);
        btn_led.attach(_btn_led);
        sld_tg.attach(_sld_tg);
        Blinker.attachDataStorage(DataStorage);   // 绑定历史数据存储函数
        Blinker.attachRTData(rtData);              // 绑定实时数据函数
    }
#pragma endregion 按钮注册与函数声明区


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

void init()  // 初始化
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

    if (use_led)
        btn_led.print("on");
    else
        btn_led.print("off");
    halt();
}

void halt()   // 停止窗帘
{
    motion_state = '0';
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    digitalWrite(LED_BUILTIN, LOW);
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

void _btn_up(const String &state)  // 按钮拉起窗帘的回调函数
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
void _btn_dn(const String &state)  // 按钮拉下窗帘的回调函数
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
void _sld_tg(int32_t data)         // 目标状态滑动条响应函数
{
    target_state = data;
    if( target_state > curtain_state){
        pulldn();
    }
    else if( target_state < curtain_state){
        pullup();
    }
    else{
        halt();
    }
}

void _btn_led(const String &state) // LED灯按钮的回调函数
{
    if (state == "on") {
        use_led = true;
        btn_led.print("on");
    } else {
        use_led = false;
        btn_led.print("off");
    }
}
void _btn_dbg(const String &state) // 按钮调试的回调函数
{
    BLINKER_LOG("=========== Debug =============");
    BLINKER_LOG("curtain_state: ", curtain_state);
    BLINKER_LOG("target_state: ", target_state);
    BLINKER_LOG("motion_state: ", motion_state);
    BLINKER_LOG("=========== Debug =============");
}
void _btn_rst(const String &state) // 按钮复位的回调函数
{
    motion_state = '0';
    curtain_state = 50;
    target_state = 50;
    halt();
}

void DataStorage()  // 历史数据函数, 每分钟执行一次
{
    BLINKER_LOG("DataStorage: ", curtain_state);
    Blinker.dataStorage("ran_sta", curtain_state);
}
void rtData()       // 实时数据函数, 每秒执行一次
{
    constexpr double pullup_speed = 100 / pullup_time;
    constexpr double pulldn_speed = pullup_speed * pulldn_coef;


    BLINKER_LOG("=========== rtData =============");
    BLINKER_LOG("curtain_state: ", curtain_state);
    BLINKER_LOG("target_state: ", target_state);
    BLINKER_LOG("motion_state: ", motion_state);

    Blinker.sendRtData("ran_sta", curtain_state);
    Blinker.printRtData();

    if( motion_state == '+'){
        curtain_state += pulldn_speed;
        if( curtain_state >= target_state){
            curtain_state = target_state;
            halt();
        }
    }
    if( motion_state == '-'){
        curtain_state -= pullup_speed;
        if( curtain_state <= target_state){
            curtain_state = target_state;
            halt();
        }
    }
}

#pragma endregion 函数定义区