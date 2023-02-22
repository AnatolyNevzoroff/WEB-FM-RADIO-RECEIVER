/////////////////////////////////////// ФУНКЦИИ / FUNCTIONS ///////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
//                                            WEB RADIO                                      //
///////////////////////////////////////////////////////////////////////////////////////////////

//ФУНКЦИЯ ПЕЧАТИ НА ЭКРАНЕ МЕНЮ WEB RADIO
void WEBradioPRT(){
lcd.setCursor(0,0);
if(smenu0==0){lcd.write(triangle);}else{lcd.print(' ');}//ТРЕУГОЛЬНИК-УКАЗАТЕЛЬ ИЛИ ПУСТОТА
if(chanWEB<10){lcd.print(' ');}lcd.print(chanWEB);//НОМЕР КАНАЛА
lcd.setCursor(4,0);
//bitrate.toInt();lcd.print(float(bitrate.toInt()/1000),0);//ВАРИАНТ С float ПЕРЕМЕННОЙ
bitrate.remove(3);if(bitrate==""){lcd.print("   ");}lcd.print(bitrate);
lcd.print('k');lcd.print(' ');//БИТРЕЙТ ВЕЩАНИЯ СТАНЦИИ
lcd.setCursor(9,0);
if(smenu0==1){lcd.write(triangle);}else{lcd.print(' ');}//ТРЕУГОЛЬНИК-УКАЗАТЕЛЬ ИЛИ ПУСТОТА

//ВАРИАНТ РЕГУЛИРОВКИ ГРОМКОСТИ В РАМКАХ БИБЛИОТЕКИ
//lcd.print("VOL:");if(volWEB<10){lcd.print(' ');}lcd.print(volWEB);
//ВАРИАНТ РЕГУЛИРОВКИ ГРОМКОСТИ В "dB"
//lcd.print("VOL");if(volume>-10&&volume<10){lcd.print(' ');}if(volume>0){lcd.print('+');}
//if(volume==0){lcd.print(' ');}lcd.print(volume);//ГРОМКОСТЬ

lcd.print("VOL:");
if(volume<-74){lcd.print(' ');}lcd.print(volume+84);//ГРОМКОСТЬ 

lcd.setCursor(0,1);
lcd.print(infoLINE);//НАЗВАНИЕ СТАНЦИИ ИЛИ ИМЯ АРТИСТА И НАЗВАНИЕ КОМПОЗИЦИИ
Serial.println("PRINT WEB MENU");
}


void audio_showstation(const char *info){channelNAME=info;
Serial.print("station     ");Serial.println(channelNAME);
}
void audio_showstreamtitle(const char *info){artistTITLE=info;
Serial.print("streamtitle ");Serial.println(artistTITLE);
}
void audio_bitrate(const char *info){bitrate=info;
Serial.print("bitrate     ");Serial.println(bitrate);
}



//////////////////////////////////////////////////////////////////////////////////////////////
//                                          FM RADIO                                        //
//////////////////////////////////////////////////////////////////////////////////////////////

//ФУНКЦИЯ ЗАПУСКА И УСТАНОВКИ НЕКОТОРЫХ ПАРАМЕТРОВ ДЛЯ РАДИОМОДУЛЯ RDA5807M
void Start_RDA5807M(){
rx.setup();//УСТАНОВКА НАЧАЛЬНЫХ ПАРАМЕТРОВ ПРИ ВКЛЮЧЕНИИ RDA5807M
rx.setMono(false);//ОТКЛЮЧАЕМ РЕЖИМ ПРИНУДИТЕЛЬНОГО МОНО (СНИЖАЕТ ЧУСТВИТЕЛЬНОСТЬ)
rx.setBass(false);//ОТКЛЮЧАЕМ УСИЛЕНИЕ БАСОВ
rx.setNewMethod(true);//ТОЛЬКО В МОДИФИЦИРОВАННОЙ ВЕРСИИ БИБЛИОТЕКИ! (ЕСТЬ В АРХИВЕ)
rx.setFmDeemphasis(1);//ЗАДЕРЖКА ПРЕДИСКАЖЕНИЙ "1" ДЛЯ ЕВРОППЫ 50МСЕК ("0" ДЛЯ АМЕРИКИ 75МСЕК)
rx.setSoftmute(false);//ОТКЛЮЧАЕМ БЕСШУМНУЮ НАСТРОЙКУ
rx.setRDS(true);//ВКЛЮЧАЕМ ПРИЁМ ДАННЫХ "RDS" 
rx.setRBDS(false);//ОТКЛЮЧАЕМ ПРИЁМ "RВDS" (РАБОТАЕТ ТОЛЬКО ДЛЯ СЕВЕРОАМЕРИКАНСКИХ FM-РАДИОСТАНЦИЙ)
rx.setRdsFifo(true);//ВКЛЮЧАЕМ ОЧЕРЁДНОСТЬ ПАКЕТОВ ПРИ ПРИЁМЕ "RDS" 
//rx.clearRdsFifo();
rx.setVolume(volFM);//ПЕРЕДАЁМ УРОВЕНЬ ГРОМКОСТИ В RDA5807M В ДИАПАЗОНЕ 0...15
freqFM=channel[chanFM];//ВЫБИРАЕМ ЧАСТОТУ ЗАПИСАННУЮ ПОД ТЕКУЩИМ НОМЕРОМ КАНАЛА
rx.setFrequency(freqFM);//ПЕРЕДАЁМ ЧАСТОТУ В RDA5807M
}


//ЦИКЛ ПЕРЕЗАПИСИ 20 ЧАСТОТ В EEPROM, АДРЕСА ЯЧЕЕК (highByte С 31 ПО 50, lowByte С 51 ПО 70)
void channelupdate(){
for(byte b=0;b<21;b++){EEPROM.write(30+b,highByte(channel[b]));
                       EEPROM.write(50+b,lowByte(channel[b]));}
}


//ФУНКЦИЯ ПЕЧАТИ НА ЭКРАНЕ МЕНЮ FM RADIO (RDA5807M) 
void FMradioPRT(){
//ПЕЧАТАЕМ НОМЕР ЯЧЕЙКИ В КОТОРОЙ ЗАПИСАНА ЧАСТОТА СТАНЦИИ
lcd.setCursor(0,0);
if(smenu8==0){lcd.write(triangle);}else{lcd.print(' ');}
if(chanFM<10){lcd.print(' ');}lcd.print(chanFM);

//ПЕЧАТАЕМ ЧАСТОТУ СТАНЦИИ
lcd.setCursor(4,0);
if(smenu8==2){lcd.write(triangle);}else{lcd.print(' ');}
if(freqFM<p10000){lcd.print(' ');}lcd.print((float)freqFM/100,1);

//lcd.setCursor(7,0);lcd.print(F("MHz"));

//ПЕЧАТАЕМ СИМВОЛ АНТЕННЫ ПРИ ЗАХВАТЕ СТАНЦИИ (ТОЛЬКО В МОДИФИЦИРОВАННОЙ ВЕРСИИ БИБЛИОТЕКИ!)
lcd.setCursor(11,0);
if(rx.getFMtrue()){lcd.write(antenna);}else{lcd.print(' ');}

lcd.print(rx.getRssi());//ПЕЧАТАЕМ УРОВЕНЬ СИГНАЛА В "dBuV" 

lcd.setCursor(15,0);
//ЗАПРАШИВАЕМ НАЛИЧИЕ ВТОРОЙ НЕСУЩЕЙ, ЕСЛИ СИГНАЛ STEREO ПЕЧАТАЕМ "S" ЕСЛИ MONO ТО "M"
lcd.print(rx.isStereo()?'S':'M');

//ПЕЧАТАЕМ НАСТРОЙКУ ГРОМКОСТИ
lcd.setCursor(0,1);
if(smenu8==1){lcd.write(triangle);}else{lcd.print(' ');}
//lcd.print("VOL:");if(volFM<10){lcd.print(' ');}lcd.print(volFM);//ГРОМКОСТЬ

//lcd.print("VOL");
//if(volume>-10&&volume<10){lcd.print(' ');}if(volume>0){lcd.print('+');}
//if(volume==0){lcd.print(' ');}lcd.print(volume);//ГРОМКОСТЬ 

lcd.print("VOL:");
if(volume<-74){lcd.print(' ');}lcd.print(volume+84);//ГРОМКОСТЬ 

//ЕСЛИ СТАНЦИЯ ПЕРЕДАЁТ RDS ПЕЧАТАЕМ НАЗВАНИЕ СТАНЦИИ, ЕСЛИ НЕТ ПЕЧАТАЕМ "No RDS"
lcd.setCursor(8,1);
if(rx.hasRdsInfo()){lcd.print(RDS_station_NAME);}
else{lcd.print(F(" No RDS "));}
Serial.println("PRINT FM MENU");
}





////////////////////////////////////////////////////////////////////////////////////////////////
//                                       РЕЖИМ LINE-IN                                        //
////////////////////////////////////////////////////////////////////////////////////////////////

void INPUTmenuPRT(){
lcd.setCursor(2,0);lcd.print("EXTERNAL INPUT");
lcd.setCursor(0,1);
if(smenu9==0){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("VOLUME ");
if(volume>-10&&volume<10){lcd.print(' ');}if(volume>0){lcd.print('+');}
if(volume==0){lcd.print(' ');}lcd.print(volume);
//if(volume<-74){lcd.print(' ');}lcd.print(volume+84);
lcd.setCursor(11,1);
if(smenu9==1){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("UP");
if(gain3<10){lcd.print(' ');}lcd.print(gain3);
}


//АНИМАЦИЯ СТРЕЛОЧКИ В МЕНЮ ПОДКЛЮЧЕНИЯ ВНЕШНЕГО ВХОДА
void Animation(){
lcd.setCursor(0,0);
if(mill>timer_MENU){timer_MENU=mill+p1000;animat=!animat; 
if(animat==true){lcd.print(' ');lcd.write(arrow);}
else{lcd.write(arrow);lcd.print(' ');}
}}


//////////////////////////////////////////////////////////////////////////////////////////////
//                                       BD37534FV MENU                                     //
//////////////////////////////////////////////////////////////////////////////////////////////

//ОБНУЛЯЕМ ТАЙМЕР АВТОВОЗВРАТА В ОСНОВНОЕ МЕНЮ, ПОДНИМАЕМ ФЛАГ ВЫПОЛНЕНИЯ ДЕЙСТВИЯ ОДИН РАЗ
void cl_TMR(){timer_AUTORET=mill;FL_com1=true;FL_com2=true;}


//ОБНУЛЯЕМ ЭНКОДЕР, ТАЙМЕР, ПОДНИМАЕМ ФЛАГ ВЫПОЛНЕНИЯ ОДНОРАЗОВОЙ КОМАНДЫ
void cl_ENC(){encoder.setCount(0);newPos=0;oldPos=0;timer_BTN=mill;timer_MENU=mill;}


//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК ЭКВАЛАЙЗЕРА
void BD_menu1(){
lcd.setCursor(1,0); lcd.print("BAS");
lcd.setCursor(7,0); lcd.print("MID");
lcd.setCursor(13,0);lcd.print("TRE");
if(smenu1==0){
lcd.setCursor(0,0);lcd.write(pointer);lcd.setCursor(0,1);lcd.write(pointer);}
else{lcd.setCursor(0,0);lcd.print(' ');lcd.setCursor(0,1);lcd.print(' ');}
if(bass>0){lcd.print('+');}if(bass==0){lcd.print(' ');}lcd.print(bass);lcd.print(' ');
if(smenu1==1){
lcd.setCursor(6,0);lcd.write(pointer);lcd.setCursor(6,1);lcd.write(pointer);}
else{lcd.setCursor(6,0);lcd.print(' ');lcd.setCursor(6,1);lcd.print(' ');}
if(middle>0){lcd.print('+');}if(middle==0){lcd.print(' ');}lcd.print(middle);lcd.print(' ');
if(smenu1==2){
lcd.setCursor(12,0);lcd.write(pointer);lcd.setCursor(12,1);lcd.write(pointer);}
else{lcd.setCursor(12,0);lcd.print(' ');lcd.setCursor(12,1);lcd.print(' ');}
if(treb>0){lcd.print('+');}if(treb==0){lcd.print(' ');}lcd.print(treb);lcd.print(' ');
}


//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК УРОВНЕЙ СИГНАЛА ДЛЯ ВХОДОВ 0 И 1
void BD_menu2(){
lcd.setCursor(0,0);lcd.print("WEB");
lcd.setCursor(4,0);
if(smenu2==0){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("VOL");
if(volWEB<10){lcd.print(' ');}lcd.print(volWEB);
lcd.setCursor(11,0);
if(smenu2==1){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("UP");
if(gain1<10){lcd.print(' ');}lcd.print(gain1);
lcd.setCursor(0,1);lcd.print("FM");
lcd.setCursor(4,1);
if(smenu2==2){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("VOL");
if(volFM<10){lcd.print(' ');}lcd.print(volFM);
lcd.setCursor(11,1);
if(smenu2==3){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("UP");
if(gain2<10){lcd.print(' ');}lcd.print(gain2);
}


//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК ЭКВАЛАЙЗЕРА
void BD_menu3(){
lcd.setCursor(0,0);
if(smenu3==0){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("BASS ");
  switch(bas_c){
    case 0: lcd.print(" 6");break;
    case 1: lcd.print(" 8");break;
    case 2: lcd.print("10");break;
    case 3: lcd.print("12");break;}
lcd.print("0 Hz");
lcd.setCursor(0,1);
if(smenu3==1){lcd.write(pointer);}else{lcd.print(' ');}lcd.print(text_Q);
  switch(bas_q){
    case 0: lcd.print("0.50");break;
    case 1: lcd.print("1.00");break;
    case 2: lcd.print("1.50");break;
    case 3: lcd.print("2.00");break;}
}


//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК ЭКВАЛАЙЗЕРА
void BD_menu4(){
lcd.setCursor(0,0);
if(smenu4==0){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("MIDDLE ");
  switch(mid_c){
    case 0: lcd.print(" 5");break;
    case 1: lcd.print("10");break;
    case 2: lcd.print("15");break;
    case 3: lcd.print("25");break;}
lcd.print("00 Hz");
lcd.setCursor(0,1);
if(smenu4==1){lcd.write(pointer);}else{lcd.print(' ');}lcd.print(text_Q);
  switch(mid_q){
    case 0: lcd.print("0.75");break;
    case 1: lcd.print("1.00");break;
    case 2: lcd.print("1.25");break;
    case 3: lcd.print("1.50");break;}
}


//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК ЭКВАЛАЙЗЕРА
void BD_menu5(){
lcd.setCursor(0,0);
if(smenu5==0){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("TREBLE ");
  switch(treb_c){
    case 0: lcd.print(" 75");break;
    case 1: lcd.print("100");break;
    case 2: lcd.print("125");break;
    case 3: lcd.print("150");break;}
lcd.print("00 Hz");
lcd.setCursor(0,1);
if(smenu5==1){lcd.write(pointer);}else{lcd.print(' ');}lcd.print(text_Q);
  switch(treb_q){
    case 0: lcd.print("0.75");break;
    case 1: lcd.print("1.25");break;}
}


//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК УРОВНЕЙ 4 ВЫХОДОВ
void BD_menu6(){
lcd.setCursor(0,0);
if(smenu6==0){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("LSP");
lcd.setCursor(4,0);
if(lf>0){lcd.print('+');}if(lf==0){lcd.print(' ');}lcd.print(lf);lcd.print(' ');
lcd.setCursor(9,0);
if(smenu6==1){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("RSP");
lcd.setCursor(13,0);
if(rf>0){lcd.print('+');}if(rf==0){lcd.print(' ');}lcd.print(rf);lcd.print(' ');
lcd.setCursor(0,1);
if(smenu6==2){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("LHP");
lcd.setCursor(4,1);
if(lt>0){lcd.print('+');}if(lt==0){lcd.print(' ');}lcd.print(lt);lcd.print(' ');
lcd.setCursor(9,1);
if(smenu6==3){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("RHP");
lcd.setCursor(13,1);
if(rt>0){lcd.print('+');}if(rt==0){lcd.print(' ');}lcd.print(rt);lcd.print(' ');
}


//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК LOUDNESS И ОЖИДАНИЯ АВТОВОЗВРАТА
void BD_menu7(){
lcd.setCursor(0,0);
if(smenu7==0){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("UP ");
if(Loud<10){lcd.print(' ');}lcd.print(Loud);
lcd.setCursor(7,0);
if(smenu7==1){lcd.write(pointer);}else{lcd.print(' ');}lcd.print("FR ");
  switch(Loud_f){
    case 0: lcd.print("250Hz");break;
    case 1: lcd.print("400Hz");break;
    case 2: lcd.print("800Hz");break;
    case 3: lcd.print("CLOSE");break;}
lcd.setCursor(0,1);lcd.print("AUTO RETURN");
lcd.setCursor(12,1);
if(smenu7==2){lcd.write(pointer);}else{lcd.print(' ');}
if(wait<10){lcd.print(' ');}lcd.print(wait);lcd.print('s');
}






//////////////////////////////////////////////////////////////////////////////////////////////
//                  ФУНКЦИЯ ЗАПИСИ ПАРАМЕТРОВ В АУДИОПРОЦЕССОР BD37534FV                    //
//////////////////////////////////////////////////////////////////////////////////////////////
void BD_audio(){
bd.setIn(input);//Input Selector = default in IC (0) //0...6
bd.setIn_gain(gain,Mute_bd);//Input Gain, Mute ON/OFF = default in IC (0,0) 
   //Input Gain = 0 dB ... 20 dB, (0...20)
   //Mute ON/OFF = OFF / ON, (0 / 1)
bd.setVol(volume);//Volume Gain / Attenuation -79 dB ... +15 dB, (-79...15) default in IC (-∞dB)
bd.setFront_1(rf);//Fader Gain / Attenuation -79 dB ... +15 dB, (-79...15) default in IC (-∞dB)
bd.setFront_2(lf);//Fader Gain / Attenuation -79 dB ... +15 dB, (-79...15) default in IC (-∞dB)
bd.setRear_1(rt); //Fader Gain / Attenuation -79 dB ... +15 dB, (-79...15) default in IC (-∞dB)
bd.setRear_2(lt); //Fader Gain / Attenuation -79 dB ... +15 dB, (-79...15) default in IC (-∞dB)
//bd.setSub(sub);   //Fader Gain / Attenuation -79 dB ... +15 dB, (-79...15) default in IC (-∞dB)
bd.setBass_setup(bas_q,bas_c);//Bass setup, default in IC (0,0)
   //Bass Q = 0.5 / 1.0 / 1.5 / 2.0, (0...3)
   //Bass fo = 60Hz / 80Hz / 100Hz / 120Hz, (0...3)
bd.setMiddle_setup(mid_q,mid_c);//Middle setup, default in IC (0,0)
   //Middle Q = 0.75 / 1.0 / 1.25 / 1.5, (0...3)
   //Middle fo = 500Hz / 1kHz / 1.5kHz / 2.5kHz, (0...3)
bd.setTreble_setup(treb_q,treb_c);//Treble setup, default in IC (0,0)
   //Treble Q = 0.75 / 1.25, (0 / 1)
   //Treble fo = 7.5kHz / 10kHz / 12.5kHz / 15kHz, (0...3)
bd.setBass_gain(bass);      //Bass gain = -20 dB ... +20 dB, (-20 ... 20) default in IC (0)
bd.setMiddle_gain(middle);//Middle gain = -20 dB ... +20 dB, (-20 ... 20) default in IC (0)
bd.setTreble_gain(treb);  //Treble gain = -20 dB ... +20 dB, (-20 ... 20) default in IC (0)
bd.setLoudness_gain(Loud); //Loudness Gain = 0 dB ... +20 dB, (0 ... 20) default in IC (0)
bd.setLoudness_f(Loud_f); //Initial setup 3 = default in IC (0)
   //250Hz / 400Hz / 800Hz / Prohibition, (0...3)


//bd.setSetup_2(sub_f,sub_out,0,faza);//Initial setup 2, default in IC (0,0,0,0)
   //Subwoofer LPF fc = OFF / 55Hz / 85Hz / 120Hz / 160Hz, (0...4)
   //Subwoofer Output Select = LPF / Front / Rear / Prohibition, (0...3)
   //Level Meter RESET = HOLD / REST, (0 / 1)
   //LPF Phase = 0 / 180, (0 / 1)
//bd.mix(); //Level Meter RESET = default in IC (0,2,1)
//bd.setSetup_1(0,2,1);//Initial setup 1 = default in IC (0,2,1)
   //Advanced switch time of Mute = 0.6 msec / 1.0 msec / 1.4 msec / 3.2 msec, (0...3)
   //Advanced switch time of Input = 4.7 msec / 7.1 msec / 11.2 msec / 14.4 msec, (0...3)
   //Advanced switch ON/OFF = OFF / ON, (0 / 1)
//bd.System_Reset();
}


///////////////////////////////////////////////////////////////////////////////////////////////
//                                     OTHER FUNCNION                                        //
///////////////////////////////////////////////////////////////////////////////////////////////


//ЕСЛИ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ В РЕЖИМЕ "POWER ON" ИЗМЕНИЛИСЬ ПЕРЕЗАПИСЫВАЕМ ИХ В EEPROM
void EEPROM_update(){
if(volume!=volume_MOD){EEPROM.write(0,volume+79);}
if(BDmenu1_save==true){
EEPROM.write(1,treb+20);EEPROM.write(2,middle+20);EEPROM.write(3,bass+20);
   BDmenu1_save=false;}
if(chanWEB!=chanWEB_MOD){EEPROM.write(5,chanWEB);}
if(chanFM!=chanFM_MOD){EEPROM.write(6,chanFM);}
if(volFM!=volFM_MOD){EEPROM.write(7,volFM);EEPROM.write(24,gain2);}
if(volWEB!=volWEB_MOD){EEPROM.write(8,volWEB);EEPROM.write(23,gain1);}
//LCD_led МЕНЯЕТСЯ В "POWER OFF" ДЛЯ ЕЁ СОХРАНЕНИЯ НАДО ВКЛЮЧИТЬ И ВЫКЛЮЧИТЬ УСТРОЙСТВО
if(LCD_led!=LCD_led_MOD){EEPROM.write(9,LCD_led);LCD_led_MOD=LCD_led;}
if(BDmenu6_save==true){
EEPROM.write(11,rf+79);EEPROM.write(12,lf+79);EEPROM.write(13,rt+79);EEPROM.write(14,lt+79);
BDmenu6_save=false;}
if(BDmenu7_save==true){
EEPROM.write(4,wait);EEPROM.write(19,Loud);EEPROM.write(27,Loud_f);
BDmenu7_save=false;}
if(R_menu!=R_menu_MOD){EEPROM.write(15,R_menu);}
if(factor==true){EEPROM.write(16,treb_c);EEPROM.write(17,mid_c);EEPROM.write(18,bas_c);
EEPROM.write(20,treb_q);EEPROM.write(21,mid_q);EEPROM.write(22,bas_q);factor=false;}
if(gain3!=gain3_MOD){EEPROM.write(25,gain3);}
EEPROM.commit();//СОХРАНЯЕМ 
}



//МЕНЮ ПЕЧАТИ ВРЕМЕНИ И ДАТЫ
void printLocalTime(){
struct tm timeinfo;
getLocalTime(&timeinfo);//ЗАПРАШИВАЕМ ДАННЫЕ С NTP СЕРВЕРА И ПЕЧАТАЕМ ПОЛУЧЕННОЕ ВРЕМЯ И ДАТУ
//if(!getLocalTime(&timeinfo)){Serial.println("Failed to obtain time");return;}
//%H - ЧАСЫ, %М - МИНУТЫ, %S - СЕКУНДЫ, 
//%А - ДЕНЬ НЕДЕЛИ ТИПА "Thursday", %a - ДЕНЬ НЕДЕЛИ ТИПА "Thu"
//%d - ДЕНЬ МЕСЯЦА, %Y - ГОД, %m - МЕСЯЦ ТИПА "09", %B - МЕСЯЦ ТИПА "September"
Serial.println(&timeinfo, "%A  %d-%B-%Y  %H:%M:%S");
lcd.setCursor(4,0);
lcd.print(&timeinfo,"%H:%M:%S");
lcd.setCursor(0,1);
lcd.print(&timeinfo,"%a   %d-%m-%Y");
}


//ЧАСТОТЫ ДЛЯ FM РАДИО МОЖНО ЗАДАТЬ С ПОМОЩЮ ФУНКЦИИ
//void LocalFMstation(){
//channel[1]=10190;channel[2]=10110;channel[3]=10310;channel[4]=10370;channel[5]=10440;
//channel[6]=10480;channel[7]=10520;channel[8]=10570;channel[9]=10610;channel[10]=10690;
//channel[11]=10790;channel[12]=8750;channel[13]=8830;channel[14]=8920;channel[15]=9990;
//channel[16]=9990;channel[17]=9990;channel[18]=9990;channel[19]=9990;channel[20]=9990;}




//БИБЛИРТЕКА audio СОДЕРЖИТ ИНФОРМАЦИОННЫЕ ФУНКЦИИ
//void audio_info(const char *info){\
Serial.print("info        ");Serial.println(info);}//audio info
//void audio_icyurl(const char *info){\
Serial.print("icyurl      ");Serial.println(info);}//station URL
//void audio_lasthost(const char *info){\
Serial.print("lasthost    ");Serial.println(info);}//stream URL played
//void audio_id3data(const char *info){\
Serial.print("id3data     ");Serial.println(info);}//id3 metadata
//void audio_eof_mp3(const char *info){\
Serial.print("eof_mp3     ");Serial.println(info);}//end of file
//void audio_commercial(const char *info){\
Serial.print("commercial  ");Serial.println(info);}//duration in sec
//void audio_eof_speech(const char *info){\
Serial.print("eof_speech  ");Serial.println(info);}
