//////////////////// ДВУХДИАПАЗОННЫЙ РАДИОПРИЁМНИК (Two-band radio receiver) ////////////////////
////////////////////////////////////// ВЕРСИЯ (Version) 1.9 /////////////////////////////////////
///////////////////// СКЕТЧ АНАТОЛИЯ НЕВЗОРОВА / CODE BY ANATOLY NEVZOROFF //////////////////////
////////////////////////////// https://github.com/AnatolyNevzoroff //////////////////////////////

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Audio.h>         // https://github.com/schreibfaul1/ESP32-audioI2S.git
#include <ESP32Encoder.h>  // https://github.com/madhephaestus/ESP32Encoder.git
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <RDA5807.h>//ОРИГИНАЛЬНАЯ БИБЛИОТЕКА https://github.com/pu2clr/RDA5807 
//ВНИМАНИЕ! 
//НЕКОТОРЫЕ ФУНКЦИИ ИСПОЛЬЗУЕМЫЕ ЗДЕСЬ РАБОТАЮТ ТОЛЬКО В МОДИФИЦИРОВАННОЙ ВЕРСИЕЙ RDA5807.h \
 https://github.com/AnatolyNevzoroff/DESKTOP_AUDIO_SYSTEM/blob/main/LIBRARIES/RDA5807-master.zip
#include <BD37534FV.h>// https://github.com/liman324/BD37534FV/archive/master.zip 


Audio audio;//ОБЪЯВЛЯЕМ "audio" ДЛЯ БИБЛИОТЕКИ Audio.h :-)
ESP32Encoder encoder;//ОБЪЯВЛЯЕМ "encoder" ДЛЯ БИБЛИОТЕКИ ESP32Encoder.h
LiquidCrystal_I2C lcd(0x27,16,2);//УСТАНАВЛИВАЕМ АДРЕС, ЧИСЛО СИМВОЛОВ, СТРОК У ЖКИ ЭКРАНА
RDA5807 rx;//ОБЪЯВЛЯЕМ "rx" ДЛЯ БИБЛИОТЕКИ RDA5807.h
BD37534FV bd;//ОБЪЯВЛЯЕМ ОБЪЕКТ "bd" ДЛЯ БИБЛИОТЕКИ АУДИОПРОЦЕССОРА BD37534FV


//БОЛЬШИЕ ЦИФРЫ ДЛЯ mill
#define p10000 10000L //КОНСТАНТА ДЛЯ МАНИПУЛЯЦИЙ С ЧАСТОТОЙ
#define p5000 5000L //ЗАДЕРЖКА ДЛЯ ПОКАЗА НАЗВАНИЯ WEB СТАНЦИИ
#define p3000 3000L //ПАУЗА ДО НАЧАЛА ПРОКРУТКИ ТЕКСТА
#define p1000 1000L //ИНТЕРВАЛ 1000 mS (1 СЕКУНДА), ТИП ДАННЫХ "L" (long)
#define p500 500L //ВРЕМЯ ОБНОВЛЕНИЯ ЭКРАНА 0,5 СЕК.
#define p400 400L //ИНТЕРВАЛ ДЛЯ СМЕЩЕНИЯ СИМВОЛОВ "БЕГУЩЕЙ СТРОКИ"
#define p200 200L //ЗАДЕРЖКА ДЛЯ ГАРАНТИРОВАННОГО ПРЕКЛЮЧЕНИЯ ДИАПАЗОНОВ
#define POLLING_RDS 40L //ЗАДЕРЖКА ДЛЯ ЗАПРОСА ДАННЫХ "RDS"
#define minFM 8700 //МИНИМАЛЬНАЯ ЧАСТОТА
#define maxFM 10800 //МАКСИМАЛЬНАЯ ЧАСТОТА
#define CH_MAX 10 //МАКСИМАЛЬНОЕ КОЛЛИЧЕСТВО WEB СТАНЦИЙ
#define text_Q "Q-FACTOR "


//ЦАП ПО I2S
#define I2S_DOUT 25 //D25 ВЫВОД  DIN (DATA connection)
#define I2S_LRC  26 //D26 ВЫВОД  LCK (Left Right Clock)
#define I2S_BCLK 27 //D27 ВЫВОД  BCK (Bit clock)

//ЭНКОДЕР
#define CLK 4       //D4 ВЫВОД (CLK ИЛИ S1) 
#define DT 2        //D2 ВЫВОД (DT ИЛИ S2) 
#define SW_key 15   //D15 КНОПКА СУБМЕНЮ (ENCODER SW) 

//КНОПКИ
#define POWER_key 13  //D13 КНОПКА ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF"
#define WEB_FM_key 12 //D12 КНОПКА ПЕРЕКЛЮЧЕНИЯ WEB РАДИО / FM РАДИО
#define MENU_key 14   //D14 КНОПКА МЕНЮ

//СВЕТОДИОДЫ-ИНДИКАТОРЫ
#define POWER_led 33  //D33 ИНДИКАТОР ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF" (красный)
#define WEB_led 32    //D32 ИНДИКАТОР ВКЛЮЧЕНИЯ WEB РАДИО (белый)
#define FM_led 23     //D23 ИНДИКАТОР ВКЛЮЧЕНИЯ FM РАДИО (жёлтый)

//ШИНА I2C
//#define SDA_pin 21
//#define SCL_pin 22


/////////////////////////////////// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ //////////////////////////////////////

//ТАЙМЕРЫ ДЛЯ ФУНКЦИИ mill  unsigned long от 0 до 4294967295
uint32_t mill,timer_AUTORET,timer_BTN,timer_MENU,timer_WATCH;
uint32_t timer_ARTIST,timer_LINE,timer_CON,timer_FIFO,timer_RDS;

//const char *ssid="TP-Link 901";//ИМЯ (SSID) WI-FI ТОЧКИ
const char *ssid="ZyXEL NWA1123";//ИМЯ (SSID) WI-FI ТОЧКИ
const char *password="tolikn23";//ПАРОЛЬ WI-FI ТОЧКИ
const char *ntpServer="1.asia.pool.ntp.org";//NTP СЕРВЕР (МОЖНО ЛЮБОЙ ДРУГОЙ)
const long gmtOffset_sec=10800;//СДВИГ В СЕКУНДАХ ОТНОСИТЕЛЬНО ГРИНВИЧА 60*60*3 (+3 Москва)
const int daylightOffset_sec=0;//СДВИГ НА ПОЛОВИНУ СУТОК (ДЛЯ АМЕРИКИ)



//String ssid="TP-Link 901";//ИМЯ (SSID) WI-FI ТОЧКИ 
//String password="tolikn23";//ПАРОЛЬ WI-FI ТОЧКИ 
String channelNAME="Connection...";//ДО ПОЛУЧЕНИЯ ДАННЫХ ПЕЧАТАЕМ "Соединение..."
String emptyBLANC="                ";//ПУСТАЯ СТРОКА 16 СИМВОЛОВ
String bitrate;//БИТРЕЙТ ВЕЩАНИЯ СТАНЦИИ
String artistTITLE;//ИПОЛНИТЕЛЬ И НАЗВАНИЕ КОМПОЗИЦИИ (ЕСЛИ ЕСТЬ)
String OLDartistTITLE;//ВРЕМЕННОЕ ХРАНЕНИЕ ИМЁН АРТИСТОВ
String infoLINE;//ДЛЯ ФУНКЦИИ ПЕЧАТИ НА ЭКРАНЕ
String RDS_station_NAME;//ИМЯ FM РАДИОСТАНЦИИ


//int от -32768 до 32767
int16_t startL,stopL;//ПЕРЕМЕННЫЕ ДЛЯ БЕГУЩЕЙ СТРОКИ
int16_t sizeL=16;//КОЛИЧЕСТВО СИМВОЛОВ В СТРОКЕ ИНФОРМАЦИИ (ДЛЯ LCD2004 - 20)
int16_t channel[21],freqFM;

//char от -128 до 127
int8_t oldPos,newPos;
int8_t chanWEB,chanWEB_MOD,OLDchanWEB,volWEB,volWEB_MOD,volFM,volFM_MOD;
int8_t smenu0,smenu1,smenu2,smenu3,smenu4,smenu5,smenu6,smenu7,smenu8,smenu9;
int8_t trig0,trig1,trig2,trig3,trig4,trig5,trig6,trig7,trig8,trig9;
int8_t input,gain,gain1,gain2,gain3,gain3_MOD,treb_c,mid_c,bas_c,treb_q,mid_q,bas_q;
int8_t rf,lf,rt,lt,volume,volume_MOD,bass,middle,treb,Loud_f,Loud,fmenu=1;

//byte от 0 до 255
uint8_t FL_runtext,FL_channel,FL_one_start,LCD_led,LCD_led_MOD;
uint8_t chanFM,chanFM_MOD,band,INPUT_band,BD_menu,R_menu,R_menu_MOD,wait,factor;
uint8_t triangle,arrow,antenna,pointer;//СИМВОЛЫ

//boolean от false до true 
bool power,FL_btn,FL_connect,FL_start_POWER=true,FL_start_MENU,FL_com1,FL_com2,RdsInfo,Mute_bd;
bool POWER_btn,POWER_btn_status=HIGH,WEB_FM_btn,WEB_FM_btn_status=HIGH,FAST_menu,FL_delay_MENU;
bool MENU_btn,MENU_btn_status=HIGH,SW_btn,SW_btn_status=HIGH,FL_delay_WEB_FM,FL_delay_SW;
bool animat,BDmenu1_save,BDmenu6_save,BDmenu7_save;


//МАССИВ ТЕКСТОВЫХ КОНСТАНТ (ССЫЛКИ НА АУДИОПОТОКИ)
const char *addressLIST[]{
"",// https://str2.pcradio.ru/radcap_spacesynth-med
"http://pool.radiopaloma.de/RADIOPALOMA.mp3",        //1
"http://pool.radiopaloma.de/RP-Partyschlager.mp3",   //2
"http://hubble.shoutca.st:8120/stream",              //3
"http://live.novoeradio.by:8000/narodnoe-radio-128k",//4
"http://listen1.myradio24.com:9000/3355",            //5
"https://rhm1.de/listen.mp3",                        //6
"http://de-hz-fal-stream01.rautemusik.fm/schlager",  //7
"http://s0.radiohost.pl:9563/stream",                //8
"http://listen2.myradio24.com:9000/1987",            //9
"http://pool.radiopaloma.de/RP-Fresh.mp3",           //10
};


///////////////////////////////////////////////////////////////////////////////////////////
//                                       S E T U P                                       //
///////////////////////////////////////////////////////////////////////////////////////////

void setup(){

//Serial.begin(9600);

encoder.attachHalfQuad(DT,CLK);//УКАЗЫВАЕМ ПИНЫ ПОДКЛЮЧЕНИЯ ЭНКОДЕРА
encoder.setCount(0);//ОБНУЛЯЕМ ЗНАЧЕНИЕ ЭНКОДЕРА
Wire.begin();//ЗАПУСКАЕМ ШИНУ I2C
//Wire.setClock(400000L);//С ЧАСТОТОЙ 400 кГц
lcd.init();lcd.clear();//ЗАПУСКАЕМ ЭКРАН 
configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);//ОТПРАВЛЯЕМ ПАРАМЕТРЫ ДЛЯ NTP



////////////////////////// ЧИТАЕМ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ ИЗ EEPROM ///////////////////////////////
EEPROM.begin(70);//ИНИЦИАЛИЗИРУЕМ ПАМЯТЬ НА ЗАДАННОЕ КОЛИЧЕСТВО ЯЧЕЕК
volume=EEPROM.read(0)-79;//0...79 ГРОМКОСТЬ
//ПАРАМЕТРЫ ЭКВАЛАЙЗЕРА
treb=EEPROM.read(1)-20;middle=EEPROM.read(2)-20;bass=EEPROM.read(3)-20;//0...40
//input=EEPROM.read(4);//0...2  
chanWEB=EEPROM.read(5);//1...CH_MAX ТЕКУЩИЙ ВЭБ-КАНАЛ (ОПРЕДЕЛЯЕТ ПОРЯДКОВЫЙ НОМЕР В МАССИВЕ ССЫЛОК)
chanFM=EEPROM.read(6);//1...20 ТЕКУЩИЙ РАДИОКАНАЛ (ОПРЕДЕЛЯЕТ ЧАСТОТУ В МАССИВЕ ЧАСТОТ СТАНЦИЙ)
volFM=EEPROM.read(7);//0...15 ВЫХОДНАЯ ГРОМКОСТЬ ЗАДАВАЕМАЯ ДЛЯ ЧИПА RDA5807
volWEB=EEPROM.read(8);//0...21 ВЫХОДНАЯ ГРОМКОСТЬ ЗАДАВАЕМАЯ ДЛЯ БИБЛИОТЕКИ Audio.h
LCD_led=EEPROM.read(9);//0...1 ПОДСВЕТКА ЭКРАНА
//НАСТРОЙКИ УРОВНЯ ВЫХОДНОГО СИГНАЛА ДЛЯ КАЖДОГО ИЗ 4 ВЫХОДОВ
rf=EEPROM.read(11)-79;lf=EEPROM.read(12)-79;rt=EEPROM.read(13)-79;lt=EEPROM.read(14)-79;//-79...15
R_menu=EEPROM.read(15);//1...3 СОХРАНЯЕМ ТЕКУЩИЙ ДИАПАЗОН И ОДНОВРЕМЕННО ВХОД
//ПАРАМЕТРЫ ЭКВАЛАЙЗЕРА
treb_c=EEPROM.read(16);mid_c=EEPROM.read(17);bas_c=EEPROM.read(18);
treb_q=EEPROM.read(20);mid_q=EEPROM.read(21);bas_q=EEPROM.read(22);
//УРОВЕНЬ ДЛЯ 3 ЗАДЕЙСТВОВАННЫХ ВХОДОВ
gain1=EEPROM.read(23);gain2=EEPROM.read(24);gain3=EEPROM.read(25);//0...20
wait=EEPROM.read(4);//ВРЕМЯ ОЖИДАНИЯ АВТОВОЗВРАТА ИЗ МЕНЮ НАСТРОЕК
Loud=EEPROM.read(19);Loud_f=EEPROM.read(27);//УРОВЕНЬ УСИЛЕНИЯ И ЧАСТОТА LOUDNESS

//ЦИКЛ ИЗВЛЕКАЕТ ИЗ ПАМЯТИ 40 ПОЛОВИНОК ЧАСТОТ В ДИАПАЗОНЕ 880...1080 (88,0...108,0 МГц)
for(byte b=0;b<21;b++){channel[b]=word(EEPROM.read(30+b),EEPROM.read(50+b));}



audio.setPinout(I2S_BCLK,I2S_LRC,I2S_DOUT);//ОБОЗНАЧАЕМ ВЫВОДЫ НА ВНЕШНИЙ ЦАП
audio.setVolume(volWEB);//УСТАНАВЛИВАЕМ ГРОМКОСТЬ В ДИАПАЗОНЕ 0 ... 21

pinMode(POWER_key,INPUT_PULLUP);//D13 КНОПКА ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF"
pinMode(WEB_FM_key,INPUT_PULLUP);//D12 КНОПКА ПЕРЕКЛЮЧЕНИЯ WEB РАДИО / FM РАДИО
pinMode(MENU_key,INPUT_PULLUP);//D14 КНОПКА МЕНЮ
pinMode(SW_key,INPUT);//D15 КНОПКА ЭНКОДЕРА (ENCODER SW)

pinMode(POWER_led,OUTPUT); //D33 ИНДИКАТОР ВКЛЮЧЕНИЯ/ОТКЛЮЧЕНИЯ "POWER ON/OFF" (красный)
pinMode(WEB_led,OUTPUT);   //D32 ИНДИКАТОР ВКЛЮЧЕНИЯ WEB РАДИО (белый)
pinMode(FM_led,OUTPUT);    //D35 ИНДИКАТОР ВКЛЮЧЕНИЯ FM РАДИО (жёлтый)



//СОЗДАЁМ СПЕЦСИМВОЛЫ ДЛЯ LCD ЭКРАНА
byte lcd1[8]={24,28,30,31,30,28,24,0};//МАССИВ ДЛЯ СИМВОЛА   ТРЕУГОЛЬНИК (ЗАПОЛНЕННЫЙ)
byte lcd2[8]={24,28,14, 7,14,28,24,0};//МАССИВ ДЛЯ СИМВОЛА   СТРЕЛОЧКА (ПОЛАЯ)
byte lcd3[8]={21,21,14, 4, 4, 4, 4,0};//МАССИВ ДЛЯ СИМВОЛА   АНТЕННА
lcd.createChar(0,lcd1);triangle=uint8_t(0);//ТРЕУГОЛЬНИК (ЗАПОЛНЕННЫЙ)
lcd.createChar(1,lcd2);arrow=uint8_t(1);   //СТРЕЛОЧКА (ПОЛАЯ)
lcd.createChar(2,lcd3);antenna=uint8_t(2); //АНТЕННА

FL_one_start=true;//ФЛАГ ДЛЯ ЗАГРУЗКИ НАЧАЛЬНЫХ ПАРАМЕТРОВ
//LocalFMstation();//МОЖНО ЗАГРУЗИТЬ ВСЕ ИЗВЕСТНЫЕ ЧАСТОТЫ РАДИОСТАНЦИЙ


R_menu=2;

}//END SENUP



/////////////////////////////////////////////////////////////////////////////////////////////////
//                                           L O O P                                           //
/////////////////////////////////////////////////////////////////////////////////////////////////


void loop(){

newPos=encoder.getCount()/2;//ОПРАШИВАЕМ ЭНКОДЕР ПОЛУЧЕННОЕ ЗНАЧЕНИЕ ОТСЧЁТОВ ДЕЛИМ НА 2
mill=millis();//ПЕРЕЗАПИСЫВАЕМ "mill" ОПЕРИРУЯ ПЕРЕМЕННОЙ ВМЕСТО ФУНКЦИИ millis()



/////////////////////////////////////////////////////////////////////////////////////////////////
//                                КНОПКА ВКЛЮЧЕНИЯ POWER ON/OFF                                //
/////////////////////////////////////////////////////////////////////////////////////////////////

POWER_btn=digitalRead(POWER_key);//ЛОГИЧЕСКИЙ "0" НАЖАТА / "1" НЕ НАЖАТА 
if(POWER_btn!=POWER_btn_status){delay(30);POWER_btn=digitalRead(POWER_key);
if(!POWER_btn&&POWER_btn_status){power=!power;FL_start_POWER=true;lcd.clear();}
POWER_btn_status=POWER_btn;}



//////////////////////////////////////////////////////////////////////////////////////////////////
//                                       РЕЖИМ POWER ON                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////
if(power==true){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ РЕЖИМА "POWER ON"
if(FL_start_POWER==true){
lcd.clear();lcd.backlight();digitalWrite(POWER_led,HIGH);
FL_start_MENU=true;BD_menu=false;band=false;pointer=triangle;

//ЗАПИСЫВАЕМ ТЕКУЩИЕ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ СОХРАНЯЕМЫХ В ПАМЯТЬ ВО ВРЕМЕННЫЕ ПЕРЕМЕННЫЕ 
//ЧТОБЫ СРАВНИТЬ ИХ ПЕРЕД ЗАПИСЬЮ В EEPROM
volume_MOD=volume;//ОБЩАЯ ГРОМКОСТЬ
chanWEB_MOD=chanWEB;//НОМЕР WEB КАНАЛА
chanFM_MOD=chanFM;//НОМЕР FM РАДИОСТАНЦИИ
volWEB_MOD=volWEB;//ГРОМКОСТЬ WEB
volFM_MOD=volFM;//ГРОМКОСТЬ FM
R_menu_MOD=R_menu;//ВЫБОР ДИАПАЗОНА ПРИ ВКЛЮЧЕНИИ
gain3_MOD=gain3;//УСИЛЕНИЕ ЛИНЕЙНОГО ВЫХОДА


WiFi.mode(WIFI_STA);
WiFi.setSleep(false);
WiFi.begin(ssid,password);
while(WiFi.status()!=WL_CONNECTED){
Serial.printf("Connecting to %s ",ssid);
delay(500);}//ОЖИДАЕМ ПОДКЛЮЧЕНИЯ К СЕТИ
Serial.println("CONNECTED!");
FL_start_POWER=false;}


if(input==0){audio.loop();}//ПОСТОЯННЫЙ ОПРОС audio НА НАЛИЧИЕ НОВЫХ ДАННЫХ


////////////////////////////////////////////////////////////////////////////////////////////////
//                    КНОПКА WEB-FM ДЛЯ ПЕРЕКЛЮЧЕНИЯ МЕЖДУ WEB И FM РАДИО                     //
////////////////////////////////////////////////////////////////////////////////////////////////

WEB_FM_btn=digitalRead(WEB_FM_key);
if(WEB_FM_btn!=WEB_FM_btn_status){delay(30);WEB_FM_btn=digitalRead(WEB_FM_key);
if(!WEB_FM_btn&&WEB_FM_btn_status){
FL_delay_WEB_FM=true;timer_BTN=mill;}//ПОДНИМАЕМ ФЛАГ КНОПКИ, ОБНУЛЯЕМ ТАЙМЕР
WEB_FM_btn_status=WEB_FM_btn;}


//ЕСЛИ КНОПКА "WEB-FM" БЫЛА НАЖАТА И ОТПУЩЕНА И ПРОШЛО НЕ МЕНЕЕ 0,2 сек. ТО ПЕРЕКЛЮЧАЕМ ДИАПАЗОН
if(FL_delay_WEB_FM==true&&WEB_FM_btn==HIGH&&mill-timer_BTN>p200){FL_delay_WEB_FM=false;


if(R_menu==0){//BD_menu==true||FAST_menu==true
BD_menu=false;FAST_menu=false;pointer=triangle;lcd.clear();
  switch(band){
    case 1:WEBradioPRT();break;
    case 2:FMradioPRT();break;
    case 3:INPUTmenuPRT();break;}
R_menu=band;return;}

if(R_menu==1||R_menu==2){R_menu++;if(R_menu>2){R_menu=1;}FL_start_MENU=true;lcd.clear();}
if(R_menu==3){R_menu=INPUT_band;FL_start_MENU=true;lcd.clear();}


Serial.println("КНОПКА WEB-FM 2");
Serial.print("INPUT_band ");Serial.println(INPUT_band);
Serial.print("R_menu 1 ");Serial.println(R_menu);
Serial.print("band 1 ");Serial.println(band);


}//END


//ЕСЛИ КНОПКА "WEB-FM" НАЖАТА И УДЕРЖИВАЕТСЯ БОЛЕ 1 СЕК. ПЕРЕХОДИМ В РЕЖИМ "LINE-IN"
if(FL_delay_WEB_FM==true&&WEB_FM_btn==LOW&&mill-timer_BTN>p1000){
INPUT_band=R_menu;band=R_menu;R_menu=3;
FL_start_MENU=true;BD_menu=false;FAST_menu=false;lcd.clear();
Serial.print("R_menu 3 ");Serial.println(R_menu);
Serial.print("INPUT_band ");Serial.println(INPUT_band);
Serial.print("band 3 ");Serial.println(band);
FL_delay_WEB_FM=false;}



////////////////////////////////////////////////////////////////////////////////////////////////
//                КНОПКА ПОСЛЕДОВАТЕЛЬНОГО ПЕРЕКЛЮЧЕНИЯ МЕЖДУ МЕНЮ BD37534FV                  //
////////////////////////////////////////////////////////////////////////////////////////////////

MENU_btn=digitalRead(MENU_key);//"0" НАЖАТА / "1" НЕ НАЖАТА 
if(MENU_btn!=MENU_btn_status){delay(30);MENU_btn=digitalRead(MENU_key);
if(!MENU_btn&&MENU_btn_status){
if(R_menu!=0){band=R_menu;}R_menu=0;
BD_menu++;if(BD_menu>7){BD_menu=1;}timer_BTN=mill;pointer=triangle;cl_TMR();FAST_menu=false;
FL_delay_MENU=true;lcd.clear();}
MENU_btn_status=MENU_btn;}//BD_menu=fmenu;



//УДЕРЖИВАЯ КНОПКУ "MENU" БОЛЕ 0,5 СЕК. ПЕРЕХОДИМ В РЕЖИМ БЫСТРОГО ПЕРЕКЛЮЧЕНИЯ МЕЖДУ МЕНЮ 
if(FL_delay_MENU==true&&MENU_btn==LOW&&mill-timer_BTN>p500){
fmenu=BD_menu;FAST_menu=true;pointer=arrow;cl_TMR();BD_menu=false;
   FL_delay_MENU=false;}



////////////////////// РЕЖИМ БЫСТРОГО ПЕРЕКЛЮЧЕНИЯ МЕЖДУ МЕНЮ (FAST MENU) /////////////////////
if(FAST_menu==true){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){BD_menu=fmenu;pointer=triangle;cl_TMR();FAST_menu=false;}
SW_btn_status=SW_btn;}

if(newPos!=oldPos){fmenu=fmenu+newPos;cl_TMR();cl_ENC();
if(fmenu>7){fmenu=1;}if(fmenu<1){fmenu=7;}lcd.clear();}

if(FL_com2==true){
switch(fmenu){
  case 1: BD_menu1();break;
  case 2: BD_menu2();break;
  case 3: BD_menu3();break;
  case 4: BD_menu4();break;
  case 5: BD_menu5();break;
  case 6: BD_menu6();break;
  case 7: BD_menu7();break;}
   FL_com2=false;}
}//END FAST MENU




////////////////////////////////////////////////////////////////////////////////////////////////
//                                          WEB RADIO                                         //
////////////////////////////////////////////////////////////////////////////////////////////////
if(R_menu==1){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ РЕЖИМА WEB RADIO
if(FL_start_MENU==true){
Mute_bd=1;BD_audio();
rx.powerDown();//ВЫКЛЮЧАЕМ RDA5807M
FAST_menu=false;BD_menu=false;
OLDchanWEB=0;
OLDartistTITLE="";//ОБНУЛЯЕМ ПЕРЕМЕННУЮ
FL_connect=true;//РАЗРЕШАЕМ ПОДКЛЮЧИТЬСЯ К РАДИО ПО АДРЕСУ ПОД НОМЕРОМ chanWEB
FL_channel=true;//БЕЗУСЛОВНО РАЗРЕШАЕМ ПЕЧАТЬ НАЗВАНИЯ СТАНЦИИ
FL_runtext=false;//ЗАПРЕЩАЕМ БЕГУЩИЙ ТЕКСТ
digitalWrite(WEB_led,HIGH);digitalWrite(FM_led,LOW);
input=0;gain=gain1;Mute_bd=0;delay(100);BD_audio();
FL_start_MENU=false;}


//НАЖАТИЕМ НА ЭНКОДЕР ВЫБИРАЕМ ЧТО МЕНЯТЬ (НОМЕР КАНАЛА ИЛИ ГРОМКОСТЬ)
SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu0++;if(smenu0>1){smenu0=0;};
switch(smenu0){
  case 0:trig0=chanWEB;break;
  case 1:trig0=volume;break;}
OLDartistTITLE="";//ОБНУЛЯЕМ OLDartistTITLE ЧТОБ ВЫПОЛНЯЛОСЬ УСЛОВИЕ artistTITLE!=OLDartistTITLE
timer_ARTIST=mill;//ЗАДЕЖКА ПЕРЕД ЗАМЕНОЙ НАЗВАНИЯ СТАНЦИИ НА ИМЯ ИСПОЛНИТЕЛЯ (5 СЕК.)
FL_runtext=false;//ЗАПРЕЩАЕМ БЕГУЩИЙ ТЕКСТ
FL_channel=true;}//БЕЗУСЛОВНО РАЗРЕШАЕМ ПЕЧАТЬ НАЗВАНИЯ СТАНЦИИ
SW_btn_status=SW_btn;}


//ЭНКОДЕРОМ МЕНЯЕМ НОМЕР КАНАЛА ИЛИ ГРОМКОСТЬ
if(newPos!=oldPos){trig0=trig0+newPos;
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
WEBradioPRT();//ПЕЧАТАЕМ НОМЕР КАНАЛА, ПУСТОЙ БИТРЕЙТ, УРОВЕНЬ ГРОМКОСТИ И "Connection..."
encoder.setCount(0);newPos=0;oldPos=0;}//СБРАСЫВАЕМ ЭНКОДЕР


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
//                                         FM RADIO RDA5807                                    //
/////////////////////////////////////////////////////////////////////////////////////////////////
if(R_menu==2){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ FM RADIO 
if(FL_start_MENU==true){
Mute_bd=1;BD_audio();
Start_RDA5807M();//ЗАПУСКАЕМ РАДИОМОДУЛЬ
FAST_menu=false;BD_menu=false;
digitalWrite(WEB_led,LOW);digitalWrite(FM_led,HIGH);timer_MENU=mill;
input=1;gain=gain2;Mute_bd=0;delay(100);BD_audio();
FL_start_MENU=false;}


//ОБРАБАТЫВАЕМ НАЖАТИЕ НА КНОПКУ ЭНКОДЕРА
SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){timer_BTN=mill;timer_MENU=mill;
if(smenu8==2){trig8=1;}else{trig8=0;}smenu8++;if(smenu8>1){smenu8=0;}FL_delay_SW=true;}
SW_btn_status=SW_btn;}


//УДЕРЖИВАЯ КНОПКУ БОЛЕЕ 0,5 СЕК. ПЕРЕКЛЮЧАЕМСЯ НА РЕЖИМ ИЗМЕНЕНИЯ ЧАСТОТЫ ИЛИ 
//ЗАПИСЫВАЕМ ВЫБРАННУЮ ЧАСТОТУ В ПАМЯТЬ ТЕКУЩЕЙ ЯЧЕЙКИ
if(FL_delay_SW==true&&SW_btn_status==LOW&&mill-timer_BTN>p500){
switch(trig8){
  case 0:smenu8=2;trig8=1;timer_MENU=mill;break;
  case 1:smenu8=0;trig8=0;channel[chanFM]=freqFM;timer_MENU=mill+p1000;
lcd.setCursor(8,1);lcd.print(F(" SAVED! "));channelupdate();break;}
FL_delay_SW=false;}


//МЕНЯЕМ КАНАЛ 
if(smenu8==0&&newPos!=oldPos){
chanFM=chanFM+newPos;if(chanFM>20){chanFM=1;}if(chanFM<1){chanFM=20;}freqFM=channel[chanFM];
rx.setFrequency(freqFM);
encoder.setCount(0);newPos=0;oldPos=0;timer_MENU=mill;}


//МЕНЯЕМ ГРОМКОСТЬ
if(smenu8==1&&newPos!=oldPos){
//volFM=volFM+newPos;volFM=constrain(volFM,1,15);rx.setVolume(volFM);
volume=volume+newPos;volume=constrain(volume,-79,15);bd.setVol(volume);
encoder.setCount(0);newPos=0;oldPos=0;timer_MENU=mill;}


//МЕНЯЕМ ЧАСТОТУ
if(smenu8==2&&newPos!=oldPos){
freqFM=freqFM+(newPos*10);if(freqFM<minFM){freqFM=maxFM;}if(freqFM>maxFM){freqFM=minFM;}
rx.setFrequency(freqFM);
encoder.setCount(0);newPos=0;oldPos=0;timer_MENU=mill;}


//ВЫВОДИМ ДАННЫЕ НА ЭКРАН 1 РАЗ В СЕК.
if(mill>timer_MENU){FMradioPRT();timer_MENU=mill+p1000;}


//ОПРАШИВАЕМ RDA5807M НА НАЛИЧИЕ RDS 
if(mill-timer_RDS>POLLING_RDS){RDS_station_NAME=rx.getRdsText0A();timer_RDS=mill;}


//СБРОС ОЧЕРЕДИ ПАКЕТОВ RDS
if(mill-timer_FIFO>p10000){rx.clearRdsFifo();timer_FIFO=mill;}

}//END MENU FM




////////////////////////////////////////////////////////////////////////////////////////////////
//                                       РЕЖИМ LINE-IN                                        //
////////////////////////////////////////////////////////////////////////////////////////////////
if(R_menu==3){

//ЗАДАЁМ НАЧАЛЬНЫЕ ЗНАЧЕНИЯ ДЛЯ РЕЖИМА "LINE-IN"
if(FL_start_MENU==true){
Mute_bd=1;BD_audio();
rx.powerDown();//ВЫКЛЮЧАЕМ RDA5807M
smenu9=0;trig9=volume;FL_com2=true;
FAST_menu=false;BD_menu=false;
digitalWrite(WEB_led,LOW);digitalWrite(FM_led,LOW);
input=2;gain=gain3;Mute_bd=0;delay(100);BD_audio();
   FL_start_MENU=false;}

Animation();//АНИМИРУЕМ СТРЕЛОЧКУ ДЛЯ АНТУРАЖА

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){
smenu9++;if(smenu9>1){smenu9=0;}FL_com2=true;
switch(smenu9){
  case 0: trig9=volume;break;
  case 1: trig9=gain3;break;}}
SW_btn_status=SW_btn;}

if(newPos!=oldPos){trig9=trig9+newPos;encoder.setCount(0);newPos=0;oldPos=0;FL_com2=true;}

if(FL_com2==true){
switch(smenu9){
  case 0: trig9=constrain(trig9,-79,15);volume=trig9;break;
  case 1: trig9=constrain(trig9,0,20);gain3=trig9;break;}
gain=gain3;BD_audio();INPUTmenuPRT();FL_com2=false;}
}//END LINE-IN MENU






///////////////////////////////////////////////////////////////////////////////////////////////
//                                       BD37534FV MENU                                      //
///////////////////////////////////////////////////////////////////////////////////////////////




////////////////////////////// BASS, MIDDLE, TREBLE -20...+20 dB //////////////////////////////
if(BD_menu==1){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu1++;if(smenu1>2){smenu1=0;}pointer=triangle;cl_TMR();}
SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu1){
  case 0: trig1=bass;break;
  case 1: trig1=middle;break;
  case 2: trig1=treb;break;}
   FL_com1=false;}

if(newPos!=oldPos){trig1=trig1+newPos;BDmenu1_save=true;cl_TMR();cl_ENC();}

if(FL_com2==true){trig1=constrain(trig1,-20,20);//Для BD37033FV trig1=constrain(bmt,-15,15);
switch(smenu1){
  case 0: bass=trig1;break;
  case 1: middle=trig1;break;
  case 2: treb=trig1;break;}
BD_audio();BD_menu1();FL_com2=false;}
}//END BASS, MIDDLE, TREBLE -20...+20 dB MENU



//////////////////////////// VOLUME WEB and FM & GAIN INPUT 0 and 1 ////////////////////////////
if(BD_menu==2){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu2++;if(smenu2>3){smenu2=0;}pointer=triangle;cl_TMR();}
SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu2){
  case 0: trig2=volWEB;break;
  case 1: trig2=gain1;break;
  case 2: trig2=volFM;break;
  case 3: trig2=gain2;break;}FL_com1=false;}

if(newPos!=oldPos){trig2=trig2+newPos;cl_TMR();cl_ENC();}

if(FL_com2==true){
switch(smenu2){
  case 0: volWEB=trig2;volWEB=constrain(volWEB,0,21);audio.setVolume(volWEB);break;
  case 1: gain1=trig2;gain1=constrain(gain1,0,20);break;
  case 2: volFM=trig2;volFM=constrain(volFM,0,15);rx.setVolume(volFM);break;
  case 3: gain2=trig2;gain2=constrain(gain2,0,20);break;}
BD_audio();BD_menu2();FL_com2=false;}
}//END VOLUME WEB and FM & GAIN INPUT 0 and 1 MENU


/////////////////////////////////// BASS CENTER & Q-FACTOR /////////////////////////////////////
if(BD_menu==3){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu3++;if(smenu3>1){smenu3=0;}pointer=triangle;cl_TMR();}
SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu3){
  case 0: trig3=bas_c;break;
  case 1: trig3=bas_q;break;}FL_com1=false;}

if(newPos!=oldPos){trig3=trig3+newPos;factor=true;cl_TMR();cl_ENC();}

if(FL_com2==true){
switch(smenu3){
  case 0: bas_c=trig3;if(bas_c>3){bas_c=0;}if(bas_c<0){bas_c=3;};break;
  case 1: bas_q=trig3;if(bas_q>3){bas_q=0;}if(bas_q<0){bas_q=3;};break;}
BD_audio();BD_menu3();FL_com2=false;}
}//END BASS CENTER & Q-FACTOR MENU


/////////////////////////////////// MIDDLE CENTER & Q-FACTOR //////////////////////////////////
if(BD_menu==4){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu4++;if(smenu4>1){smenu4=0;}pointer=triangle;cl_TMR();}
SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu4){
  case 0: trig4=mid_c;break;
  case 1: trig4=mid_q;break;}FL_com1=false;}

if(newPos!=oldPos){trig4=trig4+newPos;factor=true;cl_TMR();cl_ENC();}

if(FL_com2==true){
switch(smenu4){
  case 0: mid_c=trig4;if(mid_c>3){mid_c=0;}if(mid_c<0){mid_c=3;};break;
  case 1: mid_q=trig4;if(mid_q>3){mid_q=0;}if(mid_q<0){mid_q=3;};break;}
BD_audio();BD_menu4();FL_com2=false;}
}//END MIDDLE CENTER & Q-FACTOR MENU


/////////////////////////////////// TREBLE CENTER & Q-FACTOR //////////////////////////////////
if(BD_menu==5){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu5++;if(smenu5>1){smenu5=0;}pointer=triangle;cl_TMR();}
SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu5){
  case 0: trig5=treb_c;break;
  case 1: trig5=treb_q;break;}FL_com1=false;}

if(newPos!=oldPos){trig5=trig5+newPos;factor=true;cl_TMR();cl_ENC();}

if(FL_com2==true){
  switch(smenu5){
    case 0: treb_c=trig5;if(treb_c>3){treb_c=0;}if(treb_c<0){treb_c=3;};break;
    case 1: treb_q=trig5;if(treb_q>1){treb_q=0;}if(treb_q<0){treb_q=1;};break;}
BD_audio();BD_menu5();FL_com2=false;}
}//END TREBLE CENTER & Q-FACTOR MENU


//////////////////////////////////// OUTPUT SELECTION LEVEL ////////////////////////////////////
if(BD_menu==6){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu6++;if(smenu6>3){smenu6=0;}pointer=triangle;cl_TMR();}
SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu6){
  case 0: trig6=lf;break;
  case 1: trig6=rf;break;
  case 2: trig6=lt;break;
  case 3: trig6=rt;break;}FL_com1=false;}
    
if(newPos!=oldPos){trig6=trig6+newPos;BDmenu6_save=true;cl_TMR();cl_ENC();}

if(FL_com2==true){trig6=constrain(trig6,-79,15);
switch(smenu6){
  case 0: lf=trig6;break;
  case 1: rf=trig6;break;
  case 2: lt=trig6;break;
  case 3: rt=trig6;break;}
BD_audio();BD_menu6();FL_com2=false;}
}//END OUTPUT SELECTION LEVEL MENU




//////////////////////////// GAIN LOUDNESS FREQUENCY & AUTORETURN WAIT ////////////////////////////
if(BD_menu==7){

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){smenu7++;if(smenu7>2){smenu7=0;}pointer=triangle;cl_TMR();}
SW_btn_status=SW_btn;}

if(FL_com1==true){
switch(smenu7){
  case 0: trig7=Loud;break;
  case 1: trig7=Loud_f;break;
  case 2: trig7=wait;break;}
   FL_com1=false;}
    
if(newPos!=oldPos){trig7=trig7+newPos;BDmenu7_save=true;cl_TMR();cl_ENC();}

if(FL_com2==true){
switch(smenu7){
  case 0: trig7=constrain(trig7,0,20);Loud=trig7;break;
  case 1: if(trig7>3){trig7=0;}if(trig7<0){trig7=3;};Loud_f=trig7;break;
  case 2: trig7=constrain(trig7,5,30);wait=trig7;break;}
BD_audio();BD_menu7();FL_com2=false;}
}//END GAIN LOUDNESS FREQUENCY & AUTORETURN WAIT MENU




/////////////// АВТОВОЗВРАТ В МЕНЮ РАДИО ИЗ МЕНЮ НАСТРОЕК ЧЕРЕЗ ЗАДАННЫЙ ИНТЕРВАЛ ///////////////
if(R_menu==false&&mill-timer_AUTORET>wait*p1000){lcd.clear();pointer=triangle;
  switch(band){
    case 1:WEBradioPRT();break;
    case 2:FMradioPRT();break;
    case 3:INPUTmenuPRT();break;}
FAST_menu=false;BD_menu=false;R_menu=band;}



}//END POWER ON 






//////////////////////////////////////////////////////////////////////////////////////////////////
//                                       POWER OFF (STANDBY)                                    //
//////////////////////////////////////////////////////////////////////////////////////////////////

if(power==false){

//ТОЛЬКО ОДИН РАЗ! ПРИ ЗАГРУЗКЕ ESP32, ВРЕМЕННО, ПОДКЛЮЧАЕМСЯ К СЕТИ ДЛЯ ПОЛУЧЕНИЯ ВРЕМЕНИ
if(FL_one_start==true){
WiFi.mode(WIFI_STA);WiFi.setSleep(false);
WiFi.begin(ssid,password);while(WiFi.status()!=WL_CONNECTED){delay(500);}
lcd.setCursor(0,0);lcd.print("IP:");lcd.print(WiFi.localIP());//ПЕЧАТАЕМ АДРЕС ПОДКЛЮЧЕНИЯ
   FL_one_start=false;}



//ПРИ ВКЛЮЧЕНИИ РЕЖИМА "POWER OFF" 
if(FL_start_POWER==true){
Mute_bd=1;BD_audio();
rx.powerDown();//ВЫКЛЮЧАЕМ RDA5807M
//ЗАПРАШИВАЕМ ВРЕМЯ С NTP СЕРВЕРА 
struct tm timeinfo;
if(!getLocalTime(&timeinfo)){//ЕСЛИ ВРЕМЯ НЕ ПОЛУЧЕНО ВЫВОДИМ СООБЩЕНИЕ И ВОЗВРАЩАЕМСЯ В НАЧАЛО
//Serial.println("Connection NTP Server...");
lcd.setCursor(0,1);lcd.print("Connection...NTP");
return;}
else{lcd.clear();printLocalTime();}//ПЕЧАТАЕМ ВРЕМЯ И ДАТУ

WiFi.disconnect(true);//ОТКЛЮЧАЕМСЯ ОТ WIFI СЕТИ

//ГАСИМ ВСЕ ИНДИКАТОРЫ
digitalWrite(POWER_led,LOW);digitalWrite(WEB_led,LOW);digitalWrite(FM_led,LOW);

LCD_led?(lcd.backlight()):(lcd.noBacklight());//УСТАНАВЛИВАЕМ ПОДСВЕТКУ ЭКРАНА ON/OFF 

EEPROM_update();//ЗАПИСЫВАЕМ В EEPROM ОБНОВЛЁННЫЕ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ

FL_start_POWER=false;}



//////////////////////////////////////////////////////////////////////////////////////////////////
//                     КНОПКОЙ ЭНКОДЕРА УПРАВЛЯЕМ ПОДСВЕТКОЙ ЭКРАНА ON/OFF                      //
//////////////////////////////////////////////////////////////////////////////////////////////////

SW_btn=digitalRead(SW_key);
if(SW_btn!=SW_btn_status){delay(10);SW_btn=digitalRead(SW_key);
if(SW_btn==LOW&&SW_btn_status==HIGH){LCD_led=!LCD_led;LCD_led?(lcd.backlight()):(lcd.noBacklight());}
SW_btn_status=SW_btn;}



//ПЕЧАТАЕМ ВРЕМЯ КАЖДУЮ СЕКУНДУ
if(mill-timer_WATCH>=p1000){timer_WATCH=mill;printLocalTime();}
//ОБНУЛЕНИE mill КАЖДЫЕ 50 ДНЕЙ НЕ СБРОСИТ ВРЕМЯ
//КОРРЕКЦИЯ С NTP ВЫПОЛНЯЕТСЯ ПРИ КАЖДОМ ПЕРЕХОДЕ В РЕЖИМ "POWER OFF"


}//END POWER OFF


}//END LOOP


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                             END                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
