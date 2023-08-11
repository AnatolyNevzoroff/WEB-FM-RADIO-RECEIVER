//////////////////// ДВУХДИАПАЗОННЫЙ РАДИОПРИЁМНИК (Two-band radio receiver) ////////////////////
////////////////////////////////////// ВЕРСИЯ (Version) 1.8 /////////////////////////////////////
///////////////////// СКЕТЧ АНАТОЛИЯ НЕВЗОРОВА / Code by Anatoly Nevzoroff //////////////////////
////////////////////////////// https://github.com/AnatolyNevzoroff //////////////////////////////

//ЕСЛИ ССЫЛКА НА БИБЛИОТЕКУ ОТСУТСТВУЕТ, ЗНАЧИТ ОНА ВХОДИТ В СТАНДАРТНЫЙ НАБОР Arduino IDE
//ВСЕ БИБЛИОТЕКИ ПО ССЫЛКАМ ВХОДЯТ В АРХИВ С ДАННЫМ ФАЙЛОМ В ВЕРСИЯХ НА МОМЕНТ КОМПИЛЯЦИИ
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Audio.h>         // https://github.com/schreibfaul1/ESP32-audioI2S.git
#include "Rotary.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <BD37534FV.h>// https://github.com/liman324/BD37534FV/archive/master.zip 
#include <SI4735.h>// https://pu2clr.github.io/SI4735/




//БОЛЬШИЕ ЦИФРЫ ДЛЯ mill
#define p10000 10000L //КОНСТАНТА ДЛЯ МАНИПУЛЯЦИЙ С ЧАСТОТОЙ
#define p5000 5000L //ЗАДЕРЖКА ДЛЯ ПОКАЗА НАЗВАНИЯ WEB СТАНЦИИ
#define p3000 3000L //ПАУЗА ДО НАЧАЛА ПРОКРУТКИ ТЕКСТА
#define p1000 1000L //ИНТЕРВАЛ 1000 mS (1 СЕКУНДА), ТИП ДАННЫХ "L" (long)
#define p500 500L //ВРЕМЯ ОБНОВЛЕНИЯ ЭКРАНА 0,5 СЕК.
#define p400 400L //ИНТЕРВАЛ ДЛЯ СМЕЩЕНИЯ СИМВОЛОВ "БЕГУЩЕЙ СТРОКИ"
#define p200 200L //ЗАДЕРЖКА ДЛЯ ГАРАНТИРОВАННОГО ПРЕКЛЮЧЕНИЯ ДИАПАЗОНОВ
#define POLLING_RDS 100 //ЗАДЕРЖКА ДЛЯ ЗАПРОСА ДАННЫХ "RDS"
#define maxFM 10800 //МАКСИМАЛЬНАЯ ЧАСТОТА
#define CH_MAX 10 //МАКСИМАЛЬНОЕ КОЛЛИЧЕСТВО WEB СТАНЦИЙ
#define text_Q "Q-FACTOR "

//ЦАП ПО I2S
#define I2S_DOUT 25 //D25 ВЫВОД  DIN (DATA connection)
#define I2S_LRC  26 //D26 ВЫВОД  LCK (Left Right Clock)
#define I2S_BCLK 27 //D27 ВЫВОД  BCK (Bit clock)

//ЭНКОДЕР (Enconder PINs)
#define ENCODER_PIN_A 18 //ВЫВОД (DT ИЛИ S1) 
#define ENCODER_PIN_B 19 //ВЫВОД (CLK ИЛИ S2) 
#define SW_key 5         //КНОПКА СУБМЕНЮ (ENCODER SW) 

//КНОПКИ
#define POWER_key 13  //D13 КНОПКА ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF"
#define WEB_FM_key 12 //D12 КНОПКА ПЕРЕКЛЮЧЕНИЯ WEB РАДИО / FM РАДИО
#define MENU_key 14   //D14 КНОПКА МЕНЮ ПАРАМЕТРОВ

//СВЕТОДИОДЫ-ИНДИКАТОРЫ
#define POWER_led 33  //D33 ИНДИКАТОР ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF" (красный)
#define WEB_led 32    //D32 ИНДИКАТОР ВКЛЮЧЕНИЯ WEB РАДИО (белый)
#define FM_led 23     //D23 ИНДИКАТОР ВКЛЮЧЕНИЯ FM РАДИО (жёлтый)

//ШИНА I2C
//#define SDA_pin 21
//#define SCL_pin 22

#define EEPROM_SIZE 512
#define FM_BAND_TYPE 0
#define RESET_PIN 17 //ДЛЯ ЗАПУСКА SI4735 ПОДКЛЮЧАЕМ К 15 ВЫВОДУ РАДИОПРИЁМНИКА

 #define LOG_ENABLE //ВЫВОД ДАННЫХ В СЕРИЙНЫЙ ПОРТ ДЛЯ ОТКЛЮЧЕНИЯ УДАЛИТЬ

/////////////////////////////////// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ //////////////////////////////////////

//ТАЙМЕРЫ ДЛЯ ФУНКЦИИ mill  unsigned long от 0 до 4294967295
uint32_t mill,timer_AUTORET,timer_BTN,timer_MENU,timer_WATCH;
uint32_t timer_ARTIST,timer_LINE,timer_CON,timer_RDS;


//const char *ssid="TP-Link 901";//ИМЯ (SSID) WI-FI ТОЧКИ
//const char *ssid="ZyXEL NWA1123";//ИМЯ (SSID) WI-FI ТОЧКИ
//const char *password="tolikn23";//ПАРОЛЬ WI-FI ТОЧКИ
const char *ntpServer="1.asia.pool.ntp.org";//NTP СЕРВЕР (МОЖНО europe.pool.ntp.org или ДРУГОЙ)
const long gmtOffset_sec=10800;//СДВИГ В СЕКУНДАХ ОТНОСИТЕЛЬНО ГРИНВИЧА 60*60*3 (+3 Москва)
const int daylightOffset_sec=0;//ПЕРЕХОД НА ЛЕТНЕЕ ВРЕМЯ (ДЛЯ РОССИИ НЕ НАДО = 0, ИНАЧЕ = 3600)
char *stationName;
char bufferStatioName[10];//МАССИВ ДЛЯ ПОСИМВОЛЬНОГО ВЫВОДА ИНФРМАЦИИ RDS
struct tm timeinfo;//СТРУКТУРА ДЛЯ БИБЛИОТЕКИ ВРЕМЕНИ
volatile int encoderCount=0;//ПЕРЕМЕННАЯ ДЛЯ ЭНКОДЕРА


//String ssid="TP-Link 901";//ИМЯ (SSID) WI-FI ТОЧКИ 
//String password="tolikn23";//ПАРОЛЬ WI-FI ТОЧКИ 
String channelNAME="Connection...";//ДО ПОЛУЧЕНИЯ ДАННЫХ ПЕЧАТАЕМ "Соединение..."
String emptyBLANC="                ";//ПУСТАЯ СТРОКА 16 СИМВОЛОВ
String bitrate;//БИТРЕЙТ ВЕЩАНИЯ СТАНЦИИ
String artistTITLE;//ИПОЛНИТЕЛЬ И НАЗВАНИЕ КОМПОЗИЦИИ (ЕСЛИ ЕСТЬ)
String OLDartistTITLE;//ВРЕМЕННОЕ ХРАНЕНИЕ ИМЁН АРТИСТОВ
String infoLINE;//ДЛЯ ФУНКЦИИ ПЕЧАТИ НА ЭКРАНЕ
//String RDS_station_NAME;//ИМЯ FM РАДИОСТАНЦИИ


//int от -32768 до 32767
int16_t startL,stopL;//ПЕРЕМЕННЫЕ ДЛЯ БЕГУЩЕЙ СТРОКИ
int16_t sizeL=16;//КОЛИЧЕСТВО СИМВОЛОВ В СТРОКЕ ИНФОРМАЦИИ (ДЛЯ LCD2004 - 20)
int16_t channel[21],freqFM,OLDfreqFM,tabminfreqFM[3]={6400,8700},minfreqFM=1;


//char от -128 до 127
int8_t chanWEB,chanWEB_MOD,OLDchanWEB,volWEB,volFM,ssidIdx,passwordIdx;//
int8_t smenu0,smenu1,smenu2,smenu3,smenu4,smenu5,smenu6,smenu7,smenu8,smenu9,smenu10,smenu11;
int8_t trig0,trig1,trig2,trig3,trig4,trig5,trig6,trig7,trig8,trig9,trig10,trig11;
int8_t input,gain,gain1,gain2,gain3,gain3_MOD,treb_c,mid_c,bas_c,treb_q,mid_q,bas_q;
int8_t rf,lf,rt,lt,volume,volume_MOD,bass,middle,treb,Loud_f,Loud,fmenu=1;
int8_t bandwidth,Emphasis,softMute,agcIdx,agcNdx;


//byte от 0 до 255
uint8_t FL_runtext,FL_channel,LCD_led,LCD_led_MOD,pilot;
uint8_t chanFM,chanFM_MOD,band=2,INPUT_band=2,BD_menu,R_menu,R_menu_MOD,wait,factor;//
uint8_t triangle,arrow,antenna,pointer,multiline,signvsnoise,logostereo;//СИМВОЛЫ
uint8_t rssi,mixing,signoise,disableAgc,multipath,Internet;//


//boolean от false до true 
bool power,FL_btn,FL_connect,FL_start_POWER=true,FL_start_MENU,FL_com1,FL_com2,RdsInfo,Mute_bd;
bool POWER_btn,POWER_btn_status=HIGH,WEB_FM_btn,WEB_FM_btn_status=HIGH,FAST_menu,FL_delay_MENU;
bool MENU_btn,MENU_btn_status=HIGH,SW_btn,SW_btn_status=HIGH,FL_delay_WEB_FM,FL_delay_SW;
bool animat,BDmenu1_save,BDmenu2_save,BDmenu6_save,BDmenu7_save,BDmenu10_save,BDmenu11_save;
bool showRDS=1,showINFO=0;


const char *ssidLIST[]{"ZyXEL NWA1123","TP-Link 901",};//ИМЕНА WI-FI ТОЧЕК ДОСТУПА  
const char *passwordLIST[]{"tolikn23","tolikn23",};//ПАРОЛИ К ТОЧКАМ ДОСТУПА


//МАССИВ ТЕКСТОВЫХ КОНСТАНТ (ССЫЛКИ НА АУДИОПОТОКИ)
const char *addressLIST[]{
"",// https://str2.pcradio.ru/radcap_spacesynth-med
"http://pool.radiopaloma.de/RADIOPALOMA.mp3",        //1
"http://pool.radiopaloma.de/RP-Partyschlager.mp3",   //2
"http://hubble.shoutca.st:8120/stream",              //3
"http://live.novoeradio.by:8000/narodnoe-radio-128k",//4
"https://orfeyfm.hostingradio.ru:8034/orfeyfm192.mp3",//5
"https://rhm1.de/listen.mp3",                        //6
"http://de-hz-fal-stream01.rautemusik.fm/schlager",  //7
"http://s0.radiohost.pl:9563/stream",                //8
"http://listen2.myradio24.com:9000/1987",            //9
"http://pool.radiopaloma.de/RP-Fresh.mp3",           //10
};
// http://listen1.myradio24.com:9000/3355


//Devices class declarations
Rotary encoder=Rotary(ENCODER_PIN_A, ENCODER_PIN_B);//ОБЪЯВЛЯЕМ ПИНЫ ДЛЯ ЭНКОДЕРА
Audio audio;//ОБЪЯВЛЯЕМ "audio" ДЛЯ БИБЛИОТЕКИ Audio.h :-))
LiquidCrystal_I2C lcd(0x27,16,2);//УСТАНАВЛИВАЕМ АДРЕС, ЧИСЛО СИМВОЛОВ, СТРОК У ЖКИ ЭКРАНА
SI4735 rx;//ОБЪЯВЛЯЕМ "rx" ДЛЯ БИБЛИОТЕКИ РАДИОПРИЁМНИКА SI4735.h
BD37534FV bd;//ОБЪЯВЛЯЕМ "bd" ДЛЯ БИБЛИОТЕКИ АУДИОПРОЦЕССОРА BD37534FV






///////////////////////////////////////////////////////////////////////////////////////////
//                                       S E T U P                                       //
///////////////////////////////////////////////////////////////////////////////////////////

void setup(){

#ifdef LOG_ENABLE
Serial.begin(9600);
#endif


Wire.begin();//ЗАПУСКАЕМ ШИНУ I2C
Wire.setClock(400000L);//С ЧАСТОТОЙ 400 кГц
lcd.init();lcd.clear();//ЗАПУСКАЕМ ЭКРАН 
configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);//ОТПРАВЛЯЕМ ПАРАМЕТРЫ ПОДКЛЮЧЕНИЯ К NTP


////////////////////////// ЧИТАЕМ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ ИЗ EEPROM ///////////////////////////////
EEPROM.begin(EEPROM_SIZE);//ИНИЦИАЛИЗИРУЕМ ПАМЯТЬ НА ЗАДАННОЕ КОЛИЧЕСТВО ЯЧЕЕК
volume=EEPROM.read(170)-79;//0...79 ГРОМКОСТЬ
//ПАРАМЕТРЫ ЭКВАЛАЙЗЕРА
treb=EEPROM.read(188)-20;middle=EEPROM.read(162)-20;bass=EEPROM.read(163)-20;//0...40
//input=EEPROM.read(164);//0...2  
chanWEB=EEPROM.read(165);//1...CH_MAX ТЕКУЩИЙ ВЭБ-КАНАЛ (ОПРЕДЕЛЯЕТ ПОРЯДКОВЫЙ НОМЕР В МАССИВЕ ССЫЛОК)
chanFM=EEPROM.read(166);//1...20 ТЕКУЩИЙ РАДИОКАНАЛ (ОПРЕДЕЛЯЕТ ЧАСТОТУ В МАССИВЕ ЧАСТОТ СТАНЦИЙ)
volFM=EEPROM.read(167);//0...15 ВЫХОДНАЯ ГРОМКОСТЬ ЗАДАВАЕМАЯ ДЛЯ РАДИОПРИЁМНИКА RDA5807
volWEB=EEPROM.read(168);//0...21 ВЫХОДНАЯ ГРОМКОСТЬ ЗАДАВАЕМАЯ ДЛЯ БИБЛИОТЕКИ Audio.h
LCD_led=EEPROM.read(169);//0...1 ПОДСВЕТКА ЭКРАНА
//НАСТРОЙКИ УРОВНЯ ВЫХОДНОГО СИГНАЛА ДЛЯ КАЖДОГО ИЗ 4 ВЫХОДОВ
rf=EEPROM.read(171)-79;lf=EEPROM.read(172)-79;rt=EEPROM.read(173)-79;lt=EEPROM.read(174)-79;//-79...15
R_menu=EEPROM.read(175);//1...3 СОХРАНЯЕМ ТЕКУЩИЙ "ДИАПАЗОН" И ОДНОВРЕМЕННО ВХОД АУДИОПРОЦЕССОРА
//ПАРАМЕТРЫ ЭКВАЛАЙЗЕРА
treb_c=EEPROM.read(176);mid_c=EEPROM.read(177);bas_c=EEPROM.read(178);
treb_q=EEPROM.read(180);mid_q=EEPROM.read(181);bas_q=EEPROM.read(182);
//УРОВЕНЬ ДЛЯ 3 ЗАДЕЙСТВОВАННЫХ ВХОДОВ
gain1=EEPROM.read(183);gain2=EEPROM.read(184);gain3=EEPROM.read(185);//0...20
wait=EEPROM.read(186);//ВРЕМЯ ОЖИДАНИЯ АВТОВОЗВРАТА ИЗ МЕНЮ НАСТРОЕК 
Loud=EEPROM.read(189);Loud_f=EEPROM.read(187);//УРОВЕНЬ УСИЛЕНИЯ И ЧАСТОТА LOUDNESS
bandwidth=EEPROM.read(150);//ОГРАНИЧЕНИЕ ПОЛОСЫ ПРОПУСКАНИЯ 0 - 4 (Auto,110,84,60,40)
softMute=EEPROM.read(151);//УРОВЕНЬ ПРОГРАММНОГО ПОДАВЛЕНИЯ ШУМОВ (0 - 30)
Emphasis=EEPROM.read(152);//ВРЕМЕННАЯ ЗАДЕРЖКА ПРЕДИСКАЖЕНИЙ 50 ИЛИ 75 мс. (1 - 2)
minfreqFM=EEPROM.read(153);//МИНИМАЛЬНАЯ ЧАСТОТА 64 ИЛИ 87 МГц (0 - 1)
agcIdx=EEPROM.read(154);//СОСТОЯНИЕ AGC (АРУ) ИЛИ ATTENUATION (ЗАТУХАНИЕ) (0 - 25)
showRDS=EEPROM.read(155);//ОТКЛЮЧИТЬ / ВКЛЮЧИТЬ ПОКАЗ ИНФОРМАЦИИ RDS (0 - 1)
showINFO=EEPROM.read(156);//ОТКЛЮЧИТЬ / ВКЛЮЧИТЬ ПОКАЗ ПАРАМЕТРОВ ПРИЁМА FM (0 - 1)

//ЦИКЛ ИЗВЛЕКАЕТ ИЗ ПАМЯТИ 40 ПОЛОВИНОК ЧАСТОТ В ДИАПАЗОНЕ 640...1080 (64,0...108,0 МГц)
for(byte b=0;b<21;b++){channel[b]=word(EEPROM.read(190+b),EEPROM.read(210+b));}


rx.setup(RESET_PIN, FM_BAND_TYPE); //ЗАПУСКАЕМ SI4735 ТОЛЬКО В РЕЖИМЕ FM
//rx.setI2CFastModeCustom(100000);
rx.getDeviceI2CAddress(RESET_PIN);//ПО I2C УСТАНАВЛИВАМ АДРЕС. ЕСЛИ НЕТ ПОЛУЧАЕМ "0"
rx.setTuneFrequencyAntennaCapacitor(0);//ИСПОЛЬЗУЕМ 8 ПИН (ПО УМОЛЧАНИЮ) ДЛЯ FM НТЕННЫ
rx.setFM(6400,10800,10110,10);//НИЖНЯЯ И ВЕРХНЯЯ ГРАНИЦЫ ЧАСТОТ, ЧАСТОТА, ШАГ 100кГц
//rx.setSeekFmLimits(6400, 10800);//ГРАНИЦЫ АВТОПОИСКА
//rx.setFrequencyStep(10);//ШАГ 1 - 10 кГц, 5 - 50 кГц, 10 - 100 кГц, 20 - 200 кГц

audio.setPinout(I2S_BCLK,I2S_LRC,I2S_DOUT);//ОБОЗНАЧАЕМ ВЫВОДЫ НА ВНЕШНИЙ ЦАП
audio.setVolume(volWEB);//УСТАНАВЛИВАЕМ ГРОМКОСТЬ В ДИАПАЗОНЕ 0 ... 21

pinMode(POWER_key,INPUT_PULLUP);//D13 КНОПКА ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF"
pinMode(WEB_FM_key,INPUT_PULLUP);//D12 КНОПКА ПЕРЕКЛЮЧЕНИЯ WEB РАДИО / FM РАДИО
pinMode(MENU_key,INPUT_PULLUP);//D14 КНОПКА МЕНЮ
pinMode(SW_key,INPUT);//КНОПКА ЭНКОДЕРА (ENCODER SW)
pinMode(ENCODER_PIN_A,INPUT);//КОНТАКТ ЭНКОДЕРА (ЕСЛИ ЭНКОДЕР БЕЗ ПИТАНИЯ ТО INPUT_PULLUP)
pinMode(ENCODER_PIN_B,INPUT);//КОНТАКТ ЭНКОДЕРА (ЕСЛИ ЭНКОДЕР БЕЗ ПИТАНИЯ ТО INPUT_PULLUP)
pinMode(POWER_led,OUTPUT); //D33 ИНДИКАТОР ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF" (красный)
pinMode(WEB_led,OUTPUT);   //D32 ИНДИКАТОР ВКЛЮЧЕНИЯ WEB РАДИО (белый)
pinMode(FM_led,OUTPUT);    //D35 ИНДИКАТОР ВКЛЮЧЕНИЯ FM РАДИО (жёлтый)


//ФУНКЦИЯ "rotaryEncoder" ВЫЗЫВАЕМАЯ ПРЕРЫВАНИЕМ 
attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A),rotaryEncoder,CHANGE);
attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B),rotaryEncoder,CHANGE);


//СОЗДАЁМ СПЕЦСИМВОЛЫ ДЛЯ LCD ЭКРАНА
byte lcd0[8]={24,28,30,31,30,28,24,0};//МАССИВ ДЛЯ СИМВОЛА   ТРЕУГОЛЬНИК (ЗАПОЛНЕННЫЙ)
byte lcd1[8]={24,28,14, 7,14,28,24,0};//МАССИВ ДЛЯ СИМВОЛА   СТРЕЛОЧКА (ПОЛАЯ)
byte lcd2[8]={31,14, 4, 4, 4, 4, 4,0};//МАССИВ ДЛЯ СИМВОЛА   АНТЕННА
byte lcd3[8]={ 4,21,10,17,10,21, 4,0};//МАССИВ ДЛЯ СИМВОЛА   МНОГОЛУЧЕВОЙ
byte lcd4[8]={ 5,10,20,27, 5,10,20,0};//МАССИВ ДЛЯ СИМВОЛА   SNR 
byte lcd5[8]={17,27,10,10,10,27,17,0};//МАССИВ ДЛЯ СИМВОЛА   СТЕРЕО
lcd.createChar(0,lcd0);triangle=uint8_t(0);   //ТРЕУГОЛЬНИК (ЗАПОЛНЕННЫЙ) 
lcd.createChar(1,lcd1);arrow=uint8_t(1);      //СТРЕЛОЧКА (ПОЛАЯ)  
lcd.createChar(2,lcd2);antenna=uint8_t(2);    //АНТЕННА 
lcd.createChar(3,lcd3);multiline=uint8_t(3);  //МНОГОЛУЧЕВОЙ 
lcd.createChar(4,lcd4);signvsnoise=uint8_t(4);//SNR 
lcd.createChar(5,lcd5);logostereo=uint8_t(5); //СТЕРЕО 


//Pre_settings();//РЕКОМЕНДУЮ ЗАГРУЗИТЬ ВСЕ ИЗВЕСТНЫЕ ЧАСТОТЫ РАДИОСТАНЦИЙИ И ОСНОВНЫЕ ПАРАМЕТРЫ
//НЕ ЗАБУДЬТЕ ЗАКОМЕНТИРОВАТЬ ФУНКЦИЮ ПРИ ПОВТОРНОЙ ЗАЛИВКЕ!!! :-))


}//END SEТUP






/////////////////////////////////////////////////////////////////////////////////////////////////
//                                           L O O P                                           //
/////////////////////////////////////////////////////////////////////////////////////////////////


void loop(){


mill=millis();//ПЕРЕЗАПИСЫВАЕМ "mill" И ОПЕРИРУЕМ ЗНАЧЕНИЕМ ПЕРЕМЕННОЙ ВМЕСТО ФУНКЦИИ millis()



/////////////////////////////////////////////////////////////////////////////////////////////////
//                                КНОПКА ВКЛЮЧЕНИЯ POWER ON/OFF                                //
/////////////////////////////////////////////////////////////////////////////////////////////////

POWER_btn=digitalRead(POWER_key);//ЛОГИЧЕСКИЙ "0" НАЖАТА / "1" НЕ НАЖАТА 
if(POWER_btn!=POWER_btn_status){delay(30);POWER_btn=digitalRead(POWER_key);
if(!POWER_btn&&POWER_btn_status){power=!power;FL_start_POWER=true;lcd.clear();

#ifdef LOG_ENABLE
Serial.println("НАЖАТА КНОПКА >POWER<");
#endif

}POWER_btn_status=POWER_btn;}






//////////////////////////////////////////////////////////////////////////////////////////////////
//                                       РЕЖИМ POWER ON                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////
if(power==true){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ РЕЖИМА "POWER ON"
if(FL_start_POWER==true){
Mute_bd=0;BD_audio();//ОТКЛЮЧАЕМ "MUTE" У АУДИОПРОЦЕССОРА
lcd.clear();lcd.backlight();digitalWrite(POWER_led,HIGH);
FL_start_MENU=true;BD_menu=false;FAST_menu=false;pointer=triangle;//band=false;
   FL_start_POWER=false;}


if(input==0){audio.loop();}//ПОСТОЯННЫЙ ОПРОС audio НА НАЛИЧИЕ НОВЫХ ДАННЫХ




////////////////////////////////////////////////////////////////////////////////////////////////
//                    КНОПКА WEB-FM ДЛЯ ПЕРЕКЛЮЧЕНИЯ МЕЖДУ WEB И FM РАДИО                     //
////////////////////////////////////////////////////////////////////////////////////////////////

WEB_FM_btn=digitalRead(WEB_FM_key);
if(WEB_FM_btn!=WEB_FM_btn_status){delay(30);WEB_FM_btn=digitalRead(WEB_FM_key);
if(!WEB_FM_btn&&WEB_FM_btn_status){
FL_delay_WEB_FM=true;timer_BTN=mill;//ПОДНИМАЕМ ФЛАГ КНОПКИ, ОБНУЛЯЕМ ТАЙМЕР

#ifdef LOG_ENABLE
Serial.println("НАЖАТА КНОПКА >WEB-FM<");
#endif

}WEB_FM_btn_status=WEB_FM_btn;}


//ЕСЛИ КНОПКА "WEB-FM" БЫЛА НАЖАТА, ОТПУЩЕНА, И ПРОШЛО 0,2 СЕК. ПЕРЕКЛЮЧАЕМ ДИАПАЗОН
//ЭТО НЕОБХОДИМО ЧТОБ НЕ ПРОИЗОШЛО МГНОВЕННОЕ (ЛОЖНОЕ) ПЕРЕКЛЮЧЕНИЕ ПОСЛЕ НАЖАТИЯ НА КНОПКУ, 
//А ТОЛЬКО ПОСЛЕ ТОГО КАК:
//ЛИБО ПОЛЬЗОВАТЕЛЬ ОТПУСТИТ КНОПКУ (РЕШИТ, ЧТО ЕЁ НЕ НАДО УДЕРЖИВАТЬ БОЛЕЕ 1.0 СЕК.)
//ЛИБО С МОМЕНТА НАЖАТИЯ ПРОШЛО 0,2 СЕК. (ЗАВИСИТ ОТ ТОГО, ЧТО НАСТУПИТ РАНЬШЕ)
if(FL_delay_WEB_FM==true&&WEB_FM_btn==HIGH&&mill-timer_BTN>p200){FL_delay_WEB_FM=false;

if(R_menu==1||R_menu==2){R_menu++;if(R_menu>2){R_menu=1;}FL_start_MENU=true;lcd.clear();}
if(R_menu==3){R_menu=INPUT_band;FL_start_MENU=true;lcd.clear();}

if(R_menu==0){
if(BD_menu){fmenu=BD_menu;}BD_menu=false;FAST_menu=false;pointer=triangle;lcd.clear();
  switch(band){
    case 1:WEBradioPRT();break;
    case 2:timer_MENU=mill;showChanFM();showFreqFM();showVolFM();break;
    case 3:INPUTmenuPRT();break;}
R_menu=band;}//return;

#ifdef LOG_ENABLE
Serial.println("СРАБОТАЛО ПЕРЕКЛЮЧЕНИЕ МЕЖДУ ДИАПАЗОНАМИ WEB - FM");
Serial.print("INPUT_band ");Serial.println(INPUT_band);
Serial.print("R_menu ");Serial.println(R_menu);
Serial.print("band ");Serial.println(band);
#endif

}//END


//ЕСЛИ КНОПКА "WEB-FM" БЫЛА НАЖАТА И УДЕРЖИВАЛАСЬ БОЛЕ 0,5 СЕК. ПЕРЕХОДИМ В РЕЖИМ "LINE-IN"
if(FL_delay_WEB_FM==true&&WEB_FM_btn==LOW&&mill-timer_BTN>p500){
if(band!=3){INPUT_band=band;}R_menu=3;
FL_start_MENU=true;pointer=triangle;
if(BD_menu){fmenu=BD_menu;}BD_menu=false;FAST_menu=false;lcd.clear();

#ifdef LOG_ENABLE
Serial.println("СРАБОТАЛО ПЕРЕКЛЮЧЕНИЕ В РЕЖИМ 'LINE-IN'");
Serial.print("R_menu ");Serial.println(R_menu);
Serial.print("INPUT_band ");Serial.println(INPUT_band);
Serial.print("band ");Serial.println(band);
#endif

   FL_delay_WEB_FM=false;}



////////////////////////////////////////////////////////////////////////////////////////////////
// КНОПКА ПОСЛЕДОВАТЕЛЬНОГО ПЕРЕКЛЮЧЕНИЯ МЕЖДУ НАСТРОЙКОЙ BD37534FV, SI4735 И ИНЫХ ПАРАМЕТРОВ //
////////////////////////////////////////////////////////////////////////////////////////////////

MENU_btn=digitalRead(MENU_key);//"0" НАЖАТА / "1" НЕ НАЖАТА 
if(MENU_btn!=MENU_btn_status){delay(30);MENU_btn=digitalRead(MENU_key);
if(!MENU_btn&&MENU_btn_status){
FL_delay_MENU=true;timer_BTN=mill;//ПОДНИМАЕМ ФЛАГ КНОПКИ, ОБНУЛЯЕМ ТАЙМЕР

#ifdef LOG_ENABLE
Serial.println("НАЖАТА КНОПКА >МЕНЮ ПАРАМЕТРОВ<");
#endif

}MENU_btn_status=MENU_btn;}




//ЕСЛИ КНОПКА "MENU" БЫЛА НАЖАТА, ОТПУЩЕНА, И ПРОШЛО 0,2 СЕК. 
if(FL_delay_MENU==true&&MENU_btn==HIGH&&mill-timer_BTN>p200){FL_delay_MENU=false;
R_menu=0;
FAST_menu=false;pointer=triangle;
if(!BD_menu){BD_menu=fmenu;}else{BD_menu++;if(BD_menu>9){BD_menu=1;}}
timer_AUTORET=mill;FL_com1=true;FL_com2=true;lcd.clear();
}



//УДЕРЖИВАЯ КНОПКУ "MENU" БОЛЕ 0,5 СЕК. ПЕРЕХОДИМ В РЕЖИМ БЫСТРОГО ПЕРЕКЛЮЧЕНИЯ 
//МЕЖДУ МЕНЮ С ПОМОЩЬЮ ЭНКОДЕРА
if(FL_delay_MENU==true&&MENU_btn==LOW&&mill-timer_BTN>p500){FL_delay_MENU=false;
R_menu=0;
if(BD_menu){fmenu=BD_menu;}FAST_menu=true;BD_menu=false;pointer=arrow;
timer_AUTORET=mill;FL_com2=true;lcd.clear();

#ifdef LOG_ENABLE
Serial.println("СРАБОТАЛ РЕЖИМ БЫСТРОГО ПЕРЕКЛЮЧЕНИЯ МЕЖДУ МЕНЮ BD37534FV");
#endif

}



////////////////////// РЕЖИМ БЫСТРОГО ПЕРЕКЛЮЧЕНИЯ МЕЖДУ МЕНЮ (FAST MENU) /////////////////////
if(FAST_menu==true){

//КНОПКОЙ ЭНКОДЕРА ПЕРЕКЛЮЧАЕМСЯ ИЗ РЕЖИМА БЫСТРОГО МЕНЮ В ТЕКУЩЕЕ МЕНЮ 
SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){BD_menu=fmenu;pointer=triangle;FAST_menu=false;
timer_AUTORET=mill;FL_com2=true;
}SW_btn_status=SW_btn;}

if(encoderCount!=0){fmenu=fmenu+encoderCount;timer_AUTORET=mill;FL_com2=true;
encoderCount=0;if(fmenu>9){fmenu=1;}if(fmenu<1){fmenu=9;}lcd.clear();}

if(FL_com2==true){
switch(fmenu){
  case 1: BD_menu1();break;
  case 2: BD_menu2();break;
  case 3: BD_menu3();break;
  case 4: BD_menu4();break;
  case 5: BD_menu5();break;
  case 6: BD_menu6();break;
  case 7: BD_menu7();break;
  case 8: SI_menu2();break;
  case 9: SI_menu1();break;}
  FL_com2=false;}

#ifdef LOG_ENABLE
Serial.print("ПЕЧАТАЕМ МЕНЮ ПОД НОМЕРОМ ");Serial.println(fmenu);
#endif

}//END FAST MENU






////////////////////////////////////////////////////////////////////////////////////////////////
//                                          WEB RADIO                                         //
////////////////////////////////////////////////////////////////////////////////////////////////
if(R_menu==1){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ РЕЖИМА WEB RADIO
if(FL_start_MENU==true){
if(band==2){rx.powerDown();}//ВЫКЛЮЧАЕМ SI4735
ConnectINTERNET();//ПОДКЛЮЧАЕМСЯ К WIFI
if(WiFi.status()!=WL_CONNECTED){//ЕСЛИ НЕ УДАЛОСЬ ПОДКЛЮЧИТЬСЯ WIFI
lcd.setCursor(0,1);lcd.print("NO WIFI CONNECT!");
#ifdef LOG_ENABLE
Serial.println("NO WIFI POINT CONNECT!");
#endif
}

FL_connect=true;//РАЗРЕШАЕМ ПОДКЛЮЧИТЬСЯ К РАДИО ПО АДРЕСУ ПОД НОМЕРОМ chanWEB

//OLDartistTITLE="";//ОБНУЛЯЕМ ПЕРЕМЕННУЮ
//FL_channel=true;//БЕЗУСЛОВНО РАЗРЕШАЕМ ПЕЧАТЬ НАЗВАНИЯ СТАНЦИИ
//FL_runtext=false;//ЗАПРЕЩАЕМ БЕГУЩИЙ ТЕКСТ

OLDchanWEB=0;FL_com1=true;
FAST_menu=false;BD_menu=false;
band=1;input=0;gain=gain1;BD_audio();
digitalWrite(WEB_led,HIGH);digitalWrite(FM_led,LOW);
   FL_start_MENU=false;}


//НАЖАТИЕМ НА ЭНКОДЕР ВЫБИРАЕМ ЧТО МЕНЯТЬ (НОМЕР КАНАЛА ИЛИ ГРОМКОСТЬ)
SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu0++;if(smenu0>1){smenu0=0;};FL_com1=true;
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu0){
  case 0:trig0=chanWEB;break;
  case 1:trig0=volume;break;}
OLDartistTITLE="";//ОБНУЛЯЕМ OLDartistTITLE ЧТОБ ВЫПОЛНЯЛОСЬ УСЛОВИЕ artistTITLE!=OLDartistTITLE
timer_ARTIST=mill;//ЗАДЕЖКА ПЕРЕД ЗАМЕНОЙ НАЗВАНИЯ СТАНЦИИ НА ИМЯ ИСПОЛНИТЕЛЯ (5 СЕК.)
FL_runtext=false;//ЗАПРЕЩАЕМ БЕГУЩИЙ ТЕКСТ
FL_channel=true;//РАЗРЕШАЕМ ПЕЧАТЬ НАЗВАНИЯ СТАНЦИИ
   FL_com1=false;}


//ЭНКОДЕРОМ МЕНЯЕМ НОМЕР КАНАЛА ИЛИ ГРОМКОСТЬ
if(encoderCount!=0){trig0=trig0+encoderCount;encoderCount=0;//СБРАСЫВАЕМ ЭНКОДЕР
switch(smenu0){
//МЕНЯЕМ НОМЕР КАНАЛА
  case 0:
timer_CON=mill;//СБРАСЫВЕМ ТАЙМЕР ДЛЯ ЗАДЕРЖКИ ПОДКЛЮЧЕНИЯ, ДЛЯ ВЫБОРА ЛЮБОГО КАНАЛА 
if(trig0<1){trig0=CH_MAX;};if(trig0>CH_MAX){trig0=1;};chanWEB=trig0;
lcd.setCursor(0,1);lcd.print(emptyBLANC);//ПЕЧАТАЕМ ПРОБЕЛЫ ЗАТИРАЯ СТАРУЮ ИНФОРМАЦИЮ
channelNAME="Connection...";infoLINE=channelNAME;bitrate="";
FL_connect=true;//РАЗРЕШАЕМ ПОДКЛЮЧЕНИЕ К ВЫБРАННОМУ КАНАЛУ
FL_runtext=false;//ЗАПРЕЩАЕМ БЕГУЩИЙ ТЕКСТ
break;

//МЕНЯЕМ ГРОМКОСТЬ
//  case 1:volWEB=constrain(trig0,0,21);audio.setVolume(volWEB);break;}
  case 1:volume=constrain(trig0,-79,15);bd.setVol(volume);break;}
WEBradioPRT();}//ПЕЧАТАЕМ НОМЕР КАНАЛА, ПУСТОЙ БИТРЕЙТ, УРОВЕНЬ ГРОМКОСТИ И "Connection..."



//ПОДКЛЮЧАЕМСЯ К РАДИО ПО АДРЕСУ ИЗ МАССИВА ПОД НОМЕРОМ chanWEB
if(FL_connect==true&&mill>timer_CON+p1000){audio.connecttohost(addressLIST[chanWEB]);
delay(500);
   FL_connect=false;}


//БЕЗУСЛОВНО РАЗРЕШАЕМ ПЕЧАТЬ ИЛИ ЕСЛИ ИЗМЕНИЛСЯ НОМЕР КАНАЛА И ПОЛУЧЕНО НАЗВАНИЕ СТАНЦИИ
if(FL_channel==true||chanWEB!=OLDchanWEB&&channelNAME!="Connection..."){OLDchanWEB=chanWEB;
lcd.setCursor(0,1);lcd.print(emptyBLANC);//ПЕЧАТАЕМ ПРОБЕЛЫ ЗАТИРАЯ СТАРУЮ ИНФОРМАЦИЮ
//ЕСЛИ НАЗВАНИЕ СТАНЦИИ ПРЕВЫШАЕТ КОЛИЧЕСТВО ЗАДАННЫХ СИМВОЛОВ, ОБРЕЗАЕМ ЕГО
infoLINE=channelNAME;//ПЕРЕМЕННАЯ ИНФОРМАЦИОННОЙ СТРОКИ ЗАПОЛНЯЕТСЯ НАЗВАНИЕМ СТАНЦИИ
if(channelNAME.length()>sizeL){infoLINE.remove(sizeL);}
timer_ARTIST=mill;FL_runtext=false;WEBradioPRT();
FL_channel=false;}


//ЕСЛИ ИМЯ ИСПОЛНИТЕЛЯ ИЗМЕНИЛОСЬ И ОНО НЕ ПУСТОЕ И ПОСЛЕ ПЕЧАТИ НАЗВАНИЯ СТАНЦИИ ПРОШЛО 5 СЕК.
if(artistTITLE!=OLDartistTITLE&&artistTITLE!=""&&mill>timer_ARTIST+p5000){
OLDartistTITLE=artistTITLE;//ОПУСКАЕМ ФЛАГ ДО ИЗМЕНЕНИЯ ПЕРЕМЕННОЙ artistTITLE
lcd.setCursor(0,1);lcd.print(emptyBLANC);//ПЕЧАТАЕМ ПРОБЕЛЫ ЗАТИРАЯ СТАРУЮ ИНФОРМАЦИЮ
infoLINE=artistTITLE;//ПЕРЕМЕННАЯ ИНФОРМАЦИОННОЙ СТРОКИ ЗАПОЛНЯЕТСЯ НАЗВАНИЕМ СТАНЦИИ
//ЕСЛИ ИМЯ ИСПОЛНИТЕЛЯ ПРЕВЫШАЕТ КОЛИЧЕСТВО ЗАДАННЫХ СИМВОЛОВ, ОБРЕЗАЕМ ЕГО
if(artistTITLE.length()>sizeL){infoLINE.remove(sizeL);}
WEBradioPRT();//ПЕРЕПЕЧАТЫВАЕМ МЕНЮ В КОТОРОМ МЕНЯЕТСЯ ТОЛЬКО ИМЯ ИСПОЛНИТЕЛЯ
FL_runtext=true;//ТОЛЬКО ТЕПЕРЬ РАЗРЕШАЕМ БЕГУЩИЙ ТЕКСТ
startL=0;}//ОБНУЛЯЕМ ПЕРЕМЕННУЮ НАЧАЛА ФРАГМЕНТА, ЧТОБ ПЕЧАТАТЬ ИМЯ С ПЕРВОЙ БУКВЫ


//БЕГУЩИЙ ТЕКСТ С ИМЕНЕМ АРТИСТА И НАЗВАНИЕМ КОМПОЗИЦИИ
if(FL_runtext==true&&mill>timer_LINE+p400){timer_LINE=mill;
int longL=artistTITLE.length();//ОПРЕДЕЛЯЕМ КОЛИЧЕСТВО СИМВОЛОВ В ПЕРЕМЕННОЙ artistTITLE
if(startL==0){timer_LINE=(mill+p3000);}//ДО НАЧАЛА ПРОКРУТКИ ТЕКСТА ОСТАНАВЛИВАЕМСЯ НА 3 СЕК.
stopL=startL+sizeL;//ВЫЧИСЛЯЕМ ПОСЛЕДНИЙ СИМВОЛ
infoLINE=artistTITLE.substring(startL,stopL);//СОЗДАЁМ ОТРЕЗОК ТЕКСТА 
lcd.setCursor(0,1);lcd.print(infoLINE);//ПЕРЕПЕЧАТЫВАЕМ ОТРЕЗОК КАЖДЫЕ 400 МСЕК.
startL++;//СМЕЩАЕМСЯ НА ОДИН СИМВОЛ ВПЕРЁД
if(stopL>=longL){startL=0;//ЕСЛИ ДОСТИГНУТ КОНЕЦ СТРОКИ ОБНУЛЯЕМ СИМВОЛ НАЧАЛА
timer_LINE=(mill+p1000);}}//И ОСТАНАВЛИВАЕМ ПЕЧАТЬ ОТРЕЗКА НА 1 СЕК.


//audio.loop();//ПОСТОЯННЫЙ ОПРОС audio НА НАЛИЧИЕ ИЗМЕНИВШИХСЯ ДАННЫХ ДЛЯ ФУНКЦИЙ
//audio_showstation - КАКАЯ СТАНЦИЯ
//audio_showstreamtitle - ЧТО ИГРАЕТ
//audio_bitrate - С КАКИМ КАЧЕСТВОМ


}//END WEB RADIO MENU






/////////////////////////////////////////////////////////////////////////////////////////////////
//                                          FM RADIO SI4735                                    //
/////////////////////////////////////////////////////////////////////////////////////////////////
if(R_menu==2){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ FM RADIO 
if(FL_start_MENU==true){
if(WiFi.status()==WL_CONNECTED){WiFi.disconnect(true);WiFi.mode(WIFI_OFF);}//ОТКЛЮЧАЕМСЯ ОТ WIFI 
rx.radioPowerUp();delay(1000);StartSI4735();//ЗАПУСКАЕМ РАДИОМОДУЛЬ
FAST_menu=false;BD_menu=false;
band=2;input=1;gain=gain2;BD_audio();
showChanFM();showFreqFM();timer_MENU=mill;showVolFM();
digitalWrite(WEB_led,LOW);digitalWrite(FM_led,HIGH);
   FL_start_MENU=false;}


//ОБРАБАТЫВАЕМ НАЖАТИЕ НА КНОПКУ ЭНКОДЕРА
SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){timer_BTN=mill;
if(smenu8==2){trig8=1;}else{trig8=0;}smenu8++;if(smenu8>1){smenu8=0;}FL_delay_SW=true;
showChanFM();showFreqFM();timer_MENU=mill;showVolFM();}
SW_btn_status=SW_btn;}


//УДЕРЖИВАЯ КНОПКУ ЭНКОДЕРА БОЛЕЕ 0,5 СЕК. В ЗАВИСИМОСТИ ОТ ТЕКУЩЕГО СОСТОЯНИЯ, ПЕРЕКЛЮЧАЕМСЯ 
//ЛИБО В РЕЖИМ ИЗМЕНЕНИЯ ЧАСТОТЫ, ЛИБО ЗАПИСЫВАЕМ ВЫБРАННУЮ ЧАСТОТУ В ПАМЯТЬ ТЕКУЩЕЙ ЯЧЕЙКИ
if(FL_delay_SW==true&&SW_btn_status==LOW&&mill-timer_BTN>p500){
switch(trig8){
  case 0:smenu8=2;trig8=1;break;
  case 1:smenu8=0;trig8=0;channel[chanFM]=freqFM;
lcd.setCursor(8,1);lcd.print(F(" SAVED! "));delay(p1000);channelupdate();clearRDS();break;}
timer_MENU=mill;showChanFM();showFreqFM();showVolFM(); 
   FL_delay_SW=false;}


//МЕНЯЕМ КАНАЛ 
if(smenu8==0&&encoderCount!=0){
chanFM=chanFM+encoderCount;if(chanFM>20){chanFM=1;}if(chanFM<1){chanFM=20;}freqFM=channel[chanFM];
rx.setFrequency(freqFM);showChanFM();timer_MENU=mill;showFreqFM();
encoderCount=0;}


//МЕНЯЕМ ГРОМКОСТЬ
if(smenu8==1&&encoderCount!=0){
//volFM=volFM+encoderCount;volFM=constrain(volFM,1,63);rx.setVolume(volFM);//ГРОМКОСТЬ SI4735
volume=volume+encoderCount;volume=constrain(volume,-79,15);bd.setVol(volume);showVolFM();
encoderCount=0;}//ГРОМКОСТЬ BD37534FV


//МЕНЯЕМ ЧАСТОТУ    rx.frequencyUp();  rx.frequencyDown();
if(smenu8==2&&encoderCount!=0){
freqFM=freqFM+(encoderCount*10);
if(freqFM<tabminfreqFM[minfreqFM]){freqFM=maxFM;}if(freqFM>maxFM){freqFM=tabminfreqFM[minfreqFM];}
rx.setFrequency(freqFM);timer_MENU=mill;showFreqFM();
encoderCount=0;}


//ВЫВОДИМ ДАННЫЕ О КАЧЕССТВЕ ПРИЁМА НА ЭКРАН 1 РАЗ В СЕК.
if(showINFO&&mill>timer_MENU){showMixSTRssi();if(!showRDS){showSNR_MULTI();}timer_MENU=mill+p1000;}


//ЕСЛИ ВКЛЮЧЕН RDS ЧЕРЕЗ РАВНЫЕ ИНТЕРВАЛЫ ЗАПРАШИВАЕМ ДАННЫЕ
if(showRDS&&mill-timer_RDS>POLLING_RDS){timer_RDS=mill;
//ЕСЛИ ЧАСТОТА ИЗМЕНИЛАСЬ ОЧИЩАЕМ ПОЛЕ RDS, И ЗАПРАШИВАЕМ НОВЫЕ ДАННЫЕ
if(freqFM!=OLDfreqFM){OLDfreqFM=freqFM;clearRDS();}else{checkRDS();}
}

}//END MENU FM






////////////////////////////////////////////////////////////////////////////////////////////////
//                                       РЕЖИМ LINE-IN                                        //
////////////////////////////////////////////////////////////////////////////////////////////////
if(R_menu==3){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ РЕЖИМА "LINE-IN"
if(FL_start_MENU==true){
if(WiFi.status()==WL_CONNECTED){WiFi.disconnect(true);WiFi.mode(WIFI_OFF);}//ОТКЛЮЧАЕМСЯ ОТ WIFI 
if(band==2){rx.powerDown();}//ВЫКЛЮЧАЕМ SI4735
//smenu9=0;trig9=volume;
FAST_menu=false;BD_menu=false;FL_com1=true;FL_com2=true;
band=3;input=2;gain=gain3;//BD_audio();INPUTmenuPRT();
digitalWrite(WEB_led,LOW);digitalWrite(FM_led,LOW);
   FL_start_MENU=false;}

//ПЕРЕКЛЮЧАЕМСЯ МЕЖДУ ГРОМКОСТЬЮ И ЧУСТВИТЕЛЬНОСТЬЮ ВХОДА
SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){
smenu9++;if(smenu9>1){smenu9=0;}FL_com1=true;INPUTmenuPRT();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu9){
  case 0: trig9=volume;break;
  case 1: trig9=gain3;break;}
   FL_com1=false;}

if(encoderCount!=0){trig9=trig9+encoderCount;encoderCount=0;FL_com2=true;}

if(FL_com2==true){
switch(smenu9){
  case 0: trig9=constrain(trig9,-79,15);volume=trig9;break;
  case 1: trig9=constrain(trig9,0,20);gain=gain3=trig9;break;}
BD_audio();INPUTmenuPRT();
   FL_com2=false;}
}//END LINE-IN MENU






///////////////////////////////////////////////////////////////////////////////////////////////
//                        ГРУППА МЕНЮ НАСТРОЙКИ ПАРАМЕТРОВ BD37534FV                         //
///////////////////////////////////////////////////////////////////////////////////////////////




////////////////////////////// BASS, MIDDLE, TREBLE -20...+20 dB //////////////////////////////
if(BD_menu==1){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu1++;if(smenu1>2){smenu1=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;BD_menu1();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu1){
  case 0: trig1=bass;break;
  case 1: trig1=middle;break;
  case 2: trig1=treb;break;}
   FL_com1=false;}

if(encoderCount!=0){trig1=trig1+encoderCount;BDmenu1_save=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){trig1=constrain(trig1,-20,20);//Для BD37033FV trig1=constrain(bmt,-15,15);
switch(smenu1){
  case 0: bass=trig1;break;
  case 1: middle=trig1;break;
  case 2: treb=trig1;break;}
BD_audio();BD_menu1();
   FL_com2=false;}
}//END BASS, MIDDLE, TREBLE -20...+20 dB MENU




//////////////////////////// VOLUME WEB and FM & GAIN INPUT 0 and 1 ///////////////////////////
//  ЗАДАЁМ ГРОМКОСТЬ С ВЫХОДОВ PCM5102A И SI4735 И ЧУСТВИТЕЛЬНОСТИ ВХОДОВ BD37534FV ДЛЯ НИХ  //
if(BD_menu==2){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu2++;if(smenu2>3){smenu2=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;BD_menu2();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu2){
  case 0: trig2=volWEB;break;
  case 1: trig2=gain1;break;
  case 2: trig2=volFM;break;
  case 3: trig2=gain2;break;}
  FL_com1=false;}

if(encoderCount!=0){trig2=trig2+encoderCount;BDmenu2_save=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){
switch(smenu2){
  case 0: volWEB=trig2;volWEB=constrain(volWEB,0,21);audio.setVolume(volWEB);break;
  case 1: gain1=trig2;gain1=constrain(gain1,0,20);break;
  case 2: volFM=trig2;volFM=constrain(volFM,0,63);rx.setVolume(volFM);break;
  case 3: gain2=trig2;gain2=constrain(gain2,0,20);break;}
BD_audio();BD_menu2();
   FL_com2=false;}
}//END VOLUME WEB and FM & GAIN INPUT 0 and 1 MENU




/////////////////////////////////// BASS CENTER & Q-FACTOR /////////////////////////////////////
if(BD_menu==3){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu3++;if(smenu3>1){smenu3=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;BD_menu3();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu3){
  case 0: trig3=bas_c;break;
  case 1: trig3=bas_q;break;}
  FL_com1=false;}

if(encoderCount!=0){trig3=trig3+encoderCount;factor=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){
switch(smenu3){
  case 0: bas_c=trig3;if(bas_c>3){bas_c=0;}if(bas_c<0){bas_c=3;};break;
  case 1: bas_q=trig3;if(bas_q>3){bas_q=0;}if(bas_q<0){bas_q=3;};break;}
BD_audio();BD_menu3();
   FL_com2=false;}
}//END BASS CENTER & Q-FACTOR MENU




/////////////////////////////////// MIDDLE CENTER & Q-FACTOR //////////////////////////////////
if(BD_menu==4){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu4++;if(smenu4>1){smenu4=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;BD_menu4();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu4){
  case 0: trig4=mid_c;break;
  case 1: trig4=mid_q;break;}
  FL_com1=false;}

if(encoderCount!=0){trig4=trig4+encoderCount;factor=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){
switch(smenu4){
  case 0: mid_c=trig4;if(mid_c>3){mid_c=0;}if(mid_c<0){mid_c=3;};break;
  case 1: mid_q=trig4;if(mid_q>3){mid_q=0;}if(mid_q<0){mid_q=3;};break;}
BD_audio();BD_menu4();
   FL_com2=false;}
}//END MIDDLE CENTER & Q-FACTOR MENU




/////////////////////////////////// TREBLE CENTER & Q-FACTOR //////////////////////////////////
if(BD_menu==5){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu5++;if(smenu5>1){smenu5=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;BD_menu5();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu5){
  case 0: trig5=treb_c;break;
  case 1: trig5=treb_q;break;}
  FL_com1=false;}

if(encoderCount!=0){trig5=trig5+encoderCount;factor=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){
  switch(smenu5){
    case 0: treb_c=trig5;if(treb_c>3){treb_c=0;}if(treb_c<0){treb_c=3;};break;
    case 1: treb_q=trig5;if(treb_q>1){treb_q=0;}if(treb_q<0){treb_q=1;};break;}
BD_audio();BD_menu5();
   FL_com2=false;}
}//END TREBLE CENTER & Q-FACTOR MENU




//////////////////////////////////// OUTPUT SELECTION LEVEL ////////////////////////////////////
//          ЗАДАЁМ УРОВЕНЬ ВЫХОДНОГО СИГНАЛА BD37534FV ДЛЯ ДВУХ ВЫХОДОВ ПОКАНАЛЬНО            //
if(BD_menu==6){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu6++;if(smenu6>3){smenu6=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;BD_menu6();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu6){
  case 0: trig6=lf;break;
  case 1: trig6=rf;break;
  case 2: trig6=lt;break;
  case 3: trig6=rt;break;}
   FL_com1=false;}
    
if(encoderCount!=0){trig6=trig6+encoderCount;BDmenu6_save=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){trig6=constrain(trig6,-79,15);
switch(smenu6){
  case 0: lf=trig6;break;
  case 1: rf=trig6;break;
  case 2: lt=trig6;break;
  case 3: rt=trig6;break;}
BD_audio();BD_menu6();
   FL_com2=false;}
}//END OUTPUT SELECTION LEVEL MENU




//////////////////////////// GAIN LOUDNESS FREQUENCY & AUTORETURN WAIT ////////////////////////////
if(BD_menu==7){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu7++;if(smenu7>2){smenu7=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;BD_menu7();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu7){
  case 0: trig7=Loud;break;
  case 1: trig7=Loud_f;break;
  case 2: trig7=wait;break;}
   FL_com1=false;}
    
if(encoderCount!=0){trig7=trig7+encoderCount;BDmenu7_save=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){
switch(smenu7){
  case 0: trig7=constrain(trig7,0,20);Loud=trig7;break;
  case 1: if(trig7>3){trig7=0;}if(trig7<0){trig7=3;};Loud_f=trig7;break;
  case 2: trig7=constrain(trig7,5,30);wait=trig7;break;}
BD_audio();BD_menu7();
   FL_com2=false;}
}//END GAIN LOUDNESS FREQUENCY & AUTORETURN WAIT MENU






///////////////////////////////////////////////////////////////////////////////////////////////
//                           ДВА МЕНЮ НАСТРОЙКИ ПАРАМЕТРОВ SI4735                            //
///////////////////////////////////////////////////////////////////////////////////////////////



//ШИРИНА ПОЛОСЫ, ПОДАВЛЕНИЕ ШУМОВ, ПРЕДИСКАЖЕНИЯ, МИНИМАЛЬНАЯ ЧАСТОТА 64 ИЛИ 87 МГц
if(BD_menu==8){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu10++;if(smenu10>3){smenu10=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;SI_menu2();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu10){
  case 0: trig10=bandwidth;break;
  case 1: trig10=softMute;break;
  case 2: trig10=Emphasis;break;
  case 3: trig10=minfreqFM;break;}
   FL_com1=false;}

if(encoderCount!=0){trig10=trig10+encoderCount;BDmenu10_save=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){
switch(smenu10){
  case 0: if(trig10>4){trig10=0;}if(trig10<0){trig10=4;};bandwidth=trig10;
rx.setFmBandwidth(bandwidth);break;
  case 1: if(trig10>30){trig10=0;}if(trig10<0){trig10=30;};softMute=trig10; 
rx.setFmSoftMuteMaxAttenuation(softMute);break;
  case 2: if(trig10>2){trig10=1;}if(trig10<1){trig10=2;};Emphasis=trig10;
rx.setFMDeEmphasis(Emphasis);break;
  case 3: if(trig10>1){trig10=0;}if(trig10<0){trig10=1;};minfreqFM=trig10;break;}
SI_menu2();
   FL_com2=false;}
}//END SI4735 MENU #2




//РЕЖИМ АРУ (ON/OFF) И ВКЛЮЧЕНИЕ/ОТКЛЮЧЕНИЕ РУЧНОЙ РЕГУЛИРОВКИ ЗАТУХАНИЯ (ATTENUATION),
//ОТКЛЮЧЕНИЕ RDS, ОТКЛЮЧЕНИЕ ПАРАМЕТРОВ ПРИЁМА СИГНАЛА 
if(BD_menu==9){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu11++;if(smenu11>2){smenu11=0;}pointer=triangle;
timer_AUTORET=mill;FL_com1=true;SI_menu1();
}SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu11){
  case 0: trig11=agcIdx;break;
  case 1: trig11=showRDS;break;
  case 2: trig11=showINFO;break;}
  FL_com1=false;}

if(encoderCount!=0){trig11=trig11+encoderCount;BDmenu11_save=true;encoderCount=0;
timer_AUTORET=mill;FL_com2=true;}

if(FL_com2==true){
switch(smenu11){
//СОСТОЯНИЕ AGC (АРУ) ИЛИ ATTENUATION (ЗАТУХАНИЕ 0 - 25)
  case 0: if(trig11>27){trig11=0;}if(trig11<0){trig11=27;}agcIdx=trig11;
agcNdx=agcIdx-1;if(agcIdx==27||agcIdx==0){disableAgc=0;}else{disableAgc=1;} 
rx.setAutomaticGainControl(disableAgc, agcNdx);break; 
//ВЫКЛЮЧЕН / ВКЛЮЧЕН RDS
  case 1: if(trig11>1){trig11=0;}if(trig11<0){trig11=1;}showRDS=trig11;break;
//ПЕРЕКЛЮЧАТЕЛЬ ПОКАЗА ДОПОЛНИТЕЛЬНОЙ ИНФОРМАЦИИ О СИГНАЛЕ
  case 2: if(trig11>1){trig11=0;}if(trig11<0){trig11=1;}showINFO=trig11;break;}
SI_menu1();
   FL_com2=false;}

//ВЫВОДИМ ДАННЫЕ НА ЭКРАН 1 РАЗ В СЕК.
if(mill>timer_MENU){showMixSTRssi();timer_MENU=mill+p1000;}
}//END SI4735 MENU #2






///////////////////// АВТОВОЗВРАТ ИЗ МЕНЮ НАСТРОЕК ЧЕРЕЗ ЗАДАННЫЙ ИНТЕРВАЛ /////////////////////
if(R_menu==false&&mill-timer_AUTORET>wait*p1000){lcd.clear();pointer=triangle;
switch(band){
  case 1:WEBradioPRT();break;
  case 2:showChanFM();showFreqFM();timer_MENU=mill;showVolFM();clearRDS();break;
  case 3:INPUTmenuPRT();break;}
if(BD_menu){fmenu=BD_menu;}FAST_menu=false;BD_menu=false;R_menu=band;}



}//END POWER ON 






//////////////////////////////////////////////////////////////////////////////////////////////////
//                                       POWER OFF (STANDBY)                                    //
//////////////////////////////////////////////////////////////////////////////////////////////////

if(power==false){

//ПРИ ВКЛЮЧЕНИИ РЕЖИМА "POWER OFF" 
if(FL_start_POWER==true){
Mute_bd=1;BD_audio();//ВКЛЮЧАЕМ "MUTE" У АУДИОПРОЦЕССОРА
LCD_led?(lcd.backlight()):(lcd.noBacklight());//УСТАНАВЛИВАЕМ ПОДСВЕТКУ ЭКРАНА ON/OFF 
if(band==2){rx.powerDown();}//ВЫКЛЮЧАЕМ SI4735

//ПРОВЕРЯЕМ НАЛИЧИЕ ПОДКЛЮЧЕНИЯ К WIFI
if(WiFi.status()==WL_CONNECTED){Internet=1;}else{Internet=0;}
switch(Internet){
  case 0:ConnectINTERNET();if(!Internet){ssidIdx++;passwordIdx++;
  if(ssidIdx>1){ssidIdx=0;passwordIdx=0;ConnectINTERNET();}
  if(!Internet){lcd.setCursor(0,1);lcd.print("NO WIFI CONNECT!");}}
  else{ConnectNTP();WiFi.disconnect(true);WiFi.mode(WIFI_OFF);}break;
  case 1:lcd.setCursor(0,1);lcd.print("IP: ");lcd.print(WiFi.localIP());//ПЕЧАТАЕМ АДРЕС ПОДКЛЮЧЕНИЯ
ConnectNTP();//ПОЛУЧАЕМ ВРЕМЯ С NTP СЕРВЕРА
WiFi.disconnect(true);WiFi.mode(WIFI_OFF);break;}//ОТКЛЮЧАЕМСЯ ОТ WIFI СЕТИ

if(R_menu==0){R_menu=band;}//ЕСЛИ ПИТАНИЕ ВЫКЛЮЧЕНО ИЗ РЕЖИМА НАСТРОЙКИ ПАРАМЕТРОВ
EEPROM_update();//ЗАПИСЫВАЕМ В EEPROM ОБНОВЛЁННЫЕ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ

//КАК ИТОГ ПЕРЕХОДА В РЕЖИМ "POWER OFF" ГАСИМ ВСЕ ИНДИКАТОРЫ
digitalWrite(POWER_led,LOW);digitalWrite(WEB_led,LOW);digitalWrite(FM_led,LOW);

FL_start_POWER=false;}



//////////////////////////////////////////////////////////////////////////////////////////////////
//                     КНОПКОЙ ЭНКОДЕРА УПРАВЛЯЕМ ПОДСВЕТКОЙ ЭКРАНА ON/OFF                      //
//////////////////////////////////////////////////////////////////////////////////////////////////

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){LCD_led=!LCD_led;LCD_led?(lcd.backlight()):(lcd.noBacklight());

#ifdef LOG_ENABLE
Serial.println("НАЖАТА КНОПКА ПЕРЕКЛЮЧЕНИЯ ПОДСВЕТКИ");
#endif
}SW_btn_status=SW_btn;}



//ПЕЧАТАЕМ ВРЕМЯ КАЖДУЮ СЕКУНДУ
if(Internet&&mill-timer_WATCH>=p1000){timer_WATCH=mill;printLocalTime();}
//ОБНУЛЕНИE mill КАЖДЫЕ 50 ДНЕЙ НЕ СБРОСИТ ВРЕМЯ
//КОРРЕКЦИЯ С NTP ВЫПОЛНЯЕТСЯ ПРИ КАЖДОМ ПЕРЕХОДЕ В РЕЖИМ "POWER OFF"


}//END POWER OFF


}//END LOOP


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                             END                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
