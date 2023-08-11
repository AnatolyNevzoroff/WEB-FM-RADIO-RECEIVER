/////////////////////////////////////// ФУНКЦИИ / FUNCTIONS ///////////////////////////////////


//ФУНКЦИЯ ОБРАБОТКИ ПРЕРЫВАНИЯ ОТ СИГНАЛОВ С ЭНКОДЕРА
void rotaryEncoder(){
uint8_t encoderStatus=encoder.process();
if(encoderStatus){encoderCount=(encoderStatus==DIR_CW) ? 1 : -1;}
}






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

#ifdef LOG_ENABLE
Serial.println("PRINT WEB MENU");
#endif
}




//ГРУППА ФУНКЦИЙ ДЛЯ ПОЛУЧЕНИЯ ДАННЫХ:
//НАЗВАНИЯ СТАНЦИИ
void audio_showstation(const char *info){channelNAME=info;
#ifdef LOG_ENABLE
Serial.print("station     ");Serial.println(channelNAME);
#endif
}
//ИМЕНИ ИСПОЛНИТЕЛЯ (НАЗВАНИЕ ИСПОЛНЯЕМОГО ТРЕКА)
void audio_showstreamtitle(const char *info){artistTITLE=info;
#ifdef LOG_ENABLE
Serial.print("streamtitle ");Serial.println(artistTITLE);
#endif
}
//БИТРЕЙТ ВЕЩАНИЯ
void audio_bitrate(const char *info){bitrate=info;
#ifdef LOG_ENABLE
Serial.print("bitrate     ");Serial.println(bitrate);
#endif
}






//////////////////////////////////////////////////////////////////////////////////////////////
//                                          FM RADIO                                        //
//////////////////////////////////////////////////////////////////////////////////////////////

//ФУНКЦИЯ ЗАПУСКА И УСТАНОВКИ НЕКОТОРЫХ ПАРАМЕТРОВ ДЛЯ РАДИОМОДУЛЯ SI4735
void StartSI4735(){
freqFM=channel[chanFM];//ВЫБИРАЕМ ЧАСТОТУ ЗАПИСАННУЮ ПОД ТЕКУЩИМ НОМЕРОМ КАНАЛА
rx.setFrequency(freqFM);//ПЕРЕДАЁМ ЧАСТОТУ В SI4735
rx.setRdsConfig(1, 2, 2, 2, 2);//ПАРАМЕТРЫ ПРИЁМА RDS
rx.setFifoCount(1);//ВКЛЮЧИТЬ ЗАМЕНУ КОНТЕНТА
rx.setVolume(volFM);//ПЕРЕДАЁМ УРОВЕНЬ ГРОМКОСТИ В RDA5807M В ДИАПАЗОНЕ 0...63
rx.setFmBandwidth(bandwidth);//ЗАДАЁМ ШИРИНУ ПОЛОСЫ
rx.setFmSoftMuteMaxAttenuation(softMute);//ПРОГРАММНОЕ СНИЖЕНИЕ ШУМОВ
rx.setFMDeEmphasis(Emphasis);//ПРЕДИСКАЖЕНИЯ (ДЛЯ ЕВРОППЫ 50 μs.)
agcNdx=agcIdx-1;if(agcIdx==26||agcIdx==0){disableAgc=0;}else{disableAgc=1;} 
rx.setAutomaticGainControl(disableAgc,agcNdx);//(0 = AGC ВКЛ. 1 = AGC ВЫКЛ.),(0 - 25)

#ifdef LOG_ENABLE
Serial.println("START RDA5807M");
#endif
}





///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// ФУНКЦИИ ПЕЧАТИ НА ЭКРАНЕ МЕНЮ FM RADIO (SI4735) //////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


//ПЕЧАТАЕМ НОМЕР (КАНАЛА) ЯЧЕЙКИ В КОТОРОЙ ЗАПИСАНА ЧАСТОТА СТАНЦИИ
void showChanFM(){
lcd.setCursor(0,1);
if(smenu8==0){lcd.write(triangle);}else{lcd.print(' ');}
if(chanFM<10){lcd.print(' ');}lcd.print(chanFM);

#ifdef LOG_ENABLE
Serial.print("КАНАЛ №: ");Serial.println(chanFM);
#endif
}



//ПЕЧАТАЕМ ЧАСТОТУ СТАНЦИИ
void showFreqFM(){
lcd.setCursor(0,0);
if(smenu8==2){lcd.write(triangle);}else{lcd.print(' ');}
if(freqFM<p10000){lcd.print(' ');}
//lcd.print((float)freqFM/100,1);
lcd.print(freqFM/100);lcd.print('.');lcd.print(freqFM/10%10);

#ifdef LOG_ENABLE
Serial.print("ЗАДАННАЯ ЧАСТОТА: ");Serial.print(freqFM);Serial.println(" MHz");
Serial.print("РЕАЛЬНАЯ ЧАСТОТА: ");Serial.print(rx.getFrequency());Serial.println(" MHz");
#endif
}




//ПЕЧАТАЕМ ПРОЦЕНТ МИКШИРОВАНИЯ СТЕРЕО СИГНАЛА В "%" и УРОВЕНЬ СИГНАЛА В "dBuV" 
void showMixSTRssi(){
rx.getCurrentReceivedSignalQuality();
int aux=rx.getCurrentRSSI();
int mo_st=rx.getCurrentStereoBlend();

if(rssi!=aux||mixing!=mo_st){rssi=aux;mixing=mo_st;}
lcd.setCursor(8,0);
if(mixing==100){lcd.print("STE");}
else{lcd.write(logostereo);lcd.print(mixing);if(mixing<10){lcd.print('%');}}//lcd.print('S');
if(!rx.getCurrentPilot()){lcd.setCursor(8,0);lcd.print("MON");pilot=true;}else{pilot=false;}

lcd.setCursor(13,0);lcd.write(antenna);lcd.print(rssi);

#ifdef LOG_ENABLE
Serial.print("УРОВЕНЬ СИГНАЛА ");Serial.print(rssi);Serial.println(" dBuV");
Serial.print("МОНО / СТЕРЕО ");Serial.print(mixing);Serial.println(" %");
#endif
}




//ФУНКЦИЯ ПЕЧАТИ СООТНОШЕНИЯ СИГНАЛ / ШУМ (SNR) И ПОМЕХ ОТ ОТРАЖЁННОГО СИГНАЛА
void showSNR_MULTI(){
int snrload=rx.getCurrentSNR();//SNR (dB)
int curmult=rx.getCurrentMultipath();//МНОГОЛУЧЕВЫЕ ПОМЕХИ (ОТРАЖЁННЫЕ СИГНАЛЫ В %)
//int curmult=rx.getCurrentSignedFrequencyOffset();//СМЕЩЕНИЕ В КГЦ (МЕНЕЕ ИНТЕРЕСНЫЙ ПОКАЗАТЕЛЬ)
if(signoise!=snrload||multipath!=curmult){signoise=snrload;multipath=curmult;

//SNR
lcd.setCursor(8,1);
//if(pilot){lcd.print("MON");}else{lcd.write(signvsnoise);lcd.print(signoise);}
lcd.write(signvsnoise);if(signoise<10){lcd.print(' ');}lcd.print(signoise);

//МНОГОЛУЧЕВЫЕ ПОМЕХИ
lcd.setCursor(13,1);lcd.write(multiline);
if(multipath==100){lcd.write(multiline);lcd.write(multiline);}
else{lcd.print(multipath);if(multipath<10){lcd.print('%');}}
}

#ifdef LOG_ENABLE
Serial.print("СИГНАЛ / ШУМ ");Serial.print(signoise);Serial.println(" dB");
#endif
}




//ПЕЧАТАЕМ НАСТРОЙКУ ГРОМКОСТИ
void showVolFM(){
lcd.setCursor(3,1);
if(smenu8==1){lcd.write(triangle);}else{lcd.print(' ');}
//lcd.print("VOL:");if(volFM<10){lcd.print(' ');}lcd.print(volFM);//ГРОМКОСТЬ SI4735
lcd.print('V');
if(volume<-74){lcd.print(' ');}lcd.print(volume+84);//ГРОМКОСТЬ BD37534FV
}




//ПРОВЕРЯЕМ НАЛИЧИЕ ПИЛОТ-ТОНА RDS И ЕСЛИ СИГНАЛ УСТОЙЧИВЫЙ ЗАПИСЫВАЕМ ЗНАЧЕНИЕ БЛОКА 0А
void checkRDS(){
rx.getRdsStatus();
if(rx.getRdsReceived()){
if(rx.getRdsSync()&&rx.getRdsSyncFound()&&!rx.getRdsSyncLost()&&!rx.getGroupLost()){
stationName=rx.getRdsText0A();//}}
if(stationName!=NULL){showRDSStation();}
}}

#ifdef LOG_ENABLE
Serial.print("RDS Received: ");Serial.println(rx.getRdsReceived());
Serial.print("RDS Stantion Name: ");Serial.println(stationName);
#endif
}




//ЕСЛИ СТАНЦИЯ НЕ ПЕРЕДАЁТ RDS ПЕЧАТАЕМ ПРОБЕЛЫ
void clearRDS(){stationName=(char *)"        ";
//for(int i=0; i<8; i++){bufferStatioName[i]=NULL;}
showRDSStation();

#ifdef LOG_ENABLE 
Serial.println("ПОЛЕ RDS ОЧИЩЕНО");
#endif
}




//ПОСИМВОЛЬНО ПЕЧАТАЕМ ИМЯ СТАНЦИИ (ИЛИ ТО ЧТО ПЕРЕДАЁТ РАДИОВЕЩАТЕЛЬ)
void showRDSStation(){
for(int i=0; i<8; i++){
if(stationName[i]!=bufferStatioName[i]){
lcd.setCursor(i+8,1);
lcd.print(stationName[i]); 
bufferStatioName[i]=stationName[i];}}
}





////////////////////////////////////////////////////////////////////////////////////////////////
//                                       РЕЖИМ LINE-IN                                        //
////////////////////////////////////////////////////////////////////////////////////////////////


//ФУНКЦИЯ ПЕЧАТИ СОСТОЯНИЯ И НАСТРОЙКИ ВНЕШНЕГО ВХОДА
void INPUTmenuPRT(){
lcd.setCursor(1,0);lcd.print("EXTERNAL INPUT");
lcd.setCursor(0,1);
if(smenu9==0){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("VOLUME:");
if(volume>-10&&volume<10){lcd.print(' ');}if(volume>0){lcd.print('+');}
if(volume==0){lcd.print(' ');}lcd.print(volume);
//if(volume<-74){lcd.print(' ');}lcd.print(volume+84);
lcd.setCursor(11,1);
if(smenu9==1){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("UP");
if(gain3<10){lcd.print(' ');}lcd.print(gain3);

#ifdef LOG_ENABLE
Serial.println("PRINT LINE-IN MENU");
#endif

}




//////////////////////////////////////////////////////////////////////////////////////////////
//                                       BD37534FV MENU                                     //
//////////////////////////////////////////////////////////////////////////////////////////////




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
#ifdef LOG_ENABLE
Serial.println("PRINT BAS-MID-TRE MENU");
#endif
}




//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК УРОВНЕЙ СИГНАЛА ДЛЯ ВХОДОВ 0 И 1
void BD_menu2(){
lcd.setCursor(0,0);lcd.print("WEB");

lcd.setCursor(4,0);
if(smenu2==0){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("VOL");if(volWEB<10){lcd.print(' ');}lcd.print(volWEB);

lcd.setCursor(11,0);
if(smenu2==1){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("UP");if(gain1<10){lcd.print(' ');}lcd.print(gain1);

lcd.setCursor(0,1);lcd.print("FM");

lcd.setCursor(4,1);
if(smenu2==2){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("VOL");if(volFM<10){lcd.print(' ');}lcd.print(volFM);

lcd.setCursor(11,1);
if(smenu2==3){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("UP");if(gain2<10){lcd.print(' ');}lcd.print(gain2);

#ifdef LOG_ENABLE
Serial.println("PRINT VOL-WEB & VOL-FM MENU");
#endif
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
#ifdef LOG_ENABLE
Serial.println("PRINT BASS & Q-FACTOR MENU");
#endif
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
#ifdef LOG_ENABLE
Serial.println("PRINT MIDDLE & Q-FACTOR MENU");
#endif
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
#ifdef LOG_ENABLE
Serial.println("PRINT TREBLE & Q-FACTOR MENU");
#endif
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
#ifdef LOG_ENABLE
Serial.println("PRINT 4 OUTPUT MENU");
#endif
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
#ifdef LOG_ENABLE
Serial.println("PRINT LOUDNESS & AUTO RETURN MENU");
#endif
}




//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК SI4735 № 2
//ШИРИНА ПОЛОСЫ, ПОДАВЛЕНИЕ ШУМОВ, ПРЕДИСКАЖЕНИЯ, МИНИМАЛЬНАЯ ЧАСТОТА 64 ИЛИ 87 МГц
void SI_menu2(){
//ОГРАНИЧЕНИЕ ПОЛОСЫ ПРОПУСКАНИЯ
lcd.setCursor(0,0);
if(smenu10==0){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("BW:");
switch(bandwidth){
  case 0: lcd.print("AUT");break; //Automatic - default
  case 1: lcd.print("110");break; //Force wide (110 kHz) channel filter
  case 2: lcd.print(" 84");break; //Force narrow (84 kHz) channel filter 
  case 3: lcd.print(" 60");break; //Force narrower (60 kHz) channel filter
  case 4: lcd.print(" 40");break;}//Force narrowest (40 kHz) channel filter
//УРОВЕНЬ ПРОГРАММНОГО ПОДАВЛЕНИЯ ШУМОВ 0 - 30
lcd.setCursor(8,0);
if(smenu10==1){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("SMut:");
if(softMute<10){lcd.print(' ');}lcd.print(softMute);
//ВРЕМЕННАЯ ЗАДЕРЖКА ПРЕДИСКАЖЕНИЙ 50 ИЛИ 75 мс.
lcd.setCursor(0,1);
if(smenu10==2){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("E:");
switch(Emphasis){
  case 1: lcd.print("50ms.");break;
  case 2: lcd.print("75ms.");break;  }
//МИНИМАЛЬНАЯ ЧАСТОТА 64 ИЛИ 87 МГц
lcd.setCursor(9,1);
if(smenu10==3){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("MIN:");
lcd.print(tabminfreqFM[minfreqFM]/100);

#ifdef LOG_ENABLE
Serial.println("ШИРИНА ПОЛОСЫ, ШУМЫ, ПРЕДИСКАЖЕНИЯ, 64/87");
#endif

}//END 2 MENU SI4735




//ФУНКЦИЯ ПЕЧАТИ НАСТРОЕК SI4735 № 1
//РЕЖИМ АРУ (ON/OFF) И ВКЛЮЧЕНИЕ/ОТКЛЮЧЕНИЕ РУЧНОЙ РЕГУЛИРОВКИ ЗАТУХАНИЯ (ATTENUATION),
//ОТКЛЮЧЕНИЕ RDS, ОТКЛЮЧЕНИЕ ПАРАМЕТРОВ ПРИЁМА СИГНАЛА 
void SI_menu1(){

//Automatic Gain Control (AGC) АВТОМАТИЧЕСКАЯ РЕГУЛИРОВКА УСИЛЕНИЯ (АРУ) ЛИБО
//ВКЛЮЧЕНИЕ РУЧНОЙ РЕГУЛИРОВКИ ЗАТУХАНИЯ (ATTENUATION) (0 - МИНИМУМ, 25 - МАКСИМУМ)
lcd.setCursor(0,0);
if(smenu11==0){lcd.write(pointer);}else{lcd.print(' ');}
if(disableAgc){lcd.print("ATT:");if(agcNdx<10){lcd.print(' ');}lcd.print(agcNdx);}
else{lcd.print("AGC:ON");}

//RDS ON / OFF
lcd.setCursor(0,1);
if(smenu11==1){lcd.write(pointer);}else{lcd.print(' ');}
lcd.print("RDS:");if(showRDS){lcd.print(" ON");}else{lcd.print("OFF");}

//ЛИБО ДОПОЛНИТЕЛЬНАЯ ИНФОРМАЦИЯ ПРИ ОТКЛЮЧЕНИИ RDS, ЛИБО ПОЛНОЕ ОТКЛЮЧЕНИЕ ДАННЫХ
lcd.setCursor(9,1);
if(smenu11==2){lcd.write(pointer);}else{lcd.print(' ');}
switch(showINFO){
  case 0: lcd.print("INF0FF");break;  
  case 1: lcd.print("INF ON");break;}

#ifdef LOG_ENABLE
Serial.print("disableAgc");Serial.println(disableAgc);
Serial.print("agcIdx");Serial.println(agcIdx);
Serial.print("agcNdx");Serial.println(agcNdx);
#endif

}//END 1 MENU SI4735




//ПОДКЛЮЧАЕМСЯ К NTP СЕРВЕРУ И ЗАПРАШИВАЕМ ВРЕМЯ 
void ConnectNTP(){
//while(!getLocalTime(&timeinfo)){ 
for(byte b=0;b<5;b++){getLocalTime(&timeinfo);
lcd.setCursor(0,1);lcd.print("Connection...NTP");delay(1000);//ВЫВОДИМ СООБЩЕНИЕ
if(&timeinfo){b=5;lcd.clear();//ЕСЛИ timeinfo НЕ ПУСТАЯ, (ВРЕМЯ ПОЛУЧЕНО) 
printLocalTime();}}//ПЕЧАТАЕМ ВРЕМЯ И ДАТУ
}




//МЕНЮ ПЕЧАТИ ВРЕМЕНИ И ДАТЫ
void printLocalTime(){
//struct tm timeinfo;
getLocalTime(&timeinfo);
//if(!getLocalTime(&timeinfo)){Serial.println("Failed to obtain time");return;}
//%H - ЧАСЫ, %М - МИНУТЫ, %S - СЕКУНДЫ, 
//%А - ДЕНЬ НЕДЕЛИ ТИПА "Thursday", %a - ДЕНЬ НЕДЕЛИ ТИПА "Thu"
//%d - ДЕНЬ МЕСЯЦА, %Y - ГОД, %m - МЕСЯЦ ТИПА "09", %B - МЕСЯЦ ТИПА "September"
//ПЕЧАТАЕМ ПОЛУЧЕННОЕ ВРЕМЯ И ДАТУ
lcd.setCursor(4,0);
lcd.print(&timeinfo,"%H:%M:%S");
lcd.setCursor(0,1);
lcd.print(&timeinfo,"%a   %d-%m-%Y");

#ifdef LOG_ENABLE
Serial.println(&timeinfo, "%d-%B-%Y  %H:%M:%S  %A");
#endif
}




//ФУНКЦИЯ ПОДКЛЮЧЕНИЯ К ТОЧКЕ ДОСТУПА
void ConnectINTERNET(){
for(byte b=0;b<5;b++){
WiFi.begin(ssidLIST[ssidIdx],passwordLIST[passwordIdx]);
lcd.setCursor(0,0);lcd.print(ssidLIST[ssidIdx]);
lcd.setCursor(0,1);lcd.print("Connected ");lcd.print(b);delay(1000);
if(WiFi.status()==WL_CONNECTED){b=5;Internet=1;
WiFi.mode(WIFI_STA);WiFi.setSleep(false);lcd.clear();
lcd.setCursor(0,0);lcd.print("IP: ");lcd.print(WiFi.localIP());//ПЕЧАТАЕМ АДРЕС ПОДКЛЮЧЕНИЯ

#ifdef LOG_ENABLE
Serial.print("IP:  ");Serial.println(WiFi.localIP());
Serial.print("SSID: ");Serial.println(ssidLIST[ssidIdx]);
#endif

}else{Internet=0;}}
}




//////////////////////////////////////////////////////////////////////////////////////////////
//                  ФУНКЦИЯ ЗАПИСИ ПАРАМЕТРОВ В АУДИОПРОЦЕССОР BD37534FV                    //
//////////////////////////////////////////////////////////////////////////////////////////////
void BD_audio(){
bd.setIn(input);//Input Selector, default in IC (0)  Input 0...6
bd.setIn_gain(gain,Mute_bd);//Input Gain, Mute ON/OFF, default in IC (0,0) 
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
#ifdef LOG_ENABLE
Serial.println("PARAMETRERS TRANSFER TO BD37534FV");
#endif
}




///////////////////////////////////////////////////////////////////////////////////////////////
//                                     OTHER FUNCNION                                        //
///////////////////////////////////////////////////////////////////////////////////////////////


//ЦИКЛ ПЕРЕЗАПИСИ 20 ЧАСТОТ В EEPROM, АДРЕСА ЯЧЕЕК (highByte С 31 ПО 50, lowByte С 51 ПО 70)
void channelupdate(){
for(byte b=0;b<21;b++){EEPROM.write(190+b,highByte(channel[b]));
                       EEPROM.write(210+b,lowByte(channel[b]));}
#ifdef LOG_ENABLE
Serial.println("SAVE FREQUENCY FM STATION IN EEPROM");
#endif
}


//ЕСЛИ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ В РЕЖИМЕ "POWER ON" ИЗМЕНИЛИСЬ ПЕРЕЗАПИСЫВАЕМ ИХ В EEPROM
void EEPROM_update(){
if(volume!=volume_MOD){EEPROM.write(170,volume+79);volume_MOD=volume;}
if(BDmenu1_save==true){
EEPROM.write(188,treb+20);EEPROM.write(162,middle+20);EEPROM.write(163,bass+20);
   BDmenu1_save=false;}
if(chanWEB!=chanWEB_MOD){EEPROM.write(165,chanWEB);chanWEB_MOD=chanWEB;}
if(chanFM!=chanFM_MOD){EEPROM.write(166,chanFM);chanFM_MOD=chanFM;}
if(BDmenu2_save==true){
EEPROM.write(167,volFM);EEPROM.write(184,gain2);EEPROM.write(168,volWEB);EEPROM.write(183,gain1);
   BDmenu2_save=false;}
//LCD_led МЕНЯЕТСЯ В "POWER OFF" ДЛЯ ЕЁ СОХРАНЕНИЯ НАДО ВКЛЮЧИТЬ И ВЫКЛЮЧИТЬ УСТРОЙСТВО
if(LCD_led!=LCD_led_MOD){EEPROM.write(169,LCD_led);LCD_led_MOD=LCD_led;}
if(BDmenu6_save==true){
EEPROM.write(171,rf+79);EEPROM.write(172,lf+79);EEPROM.write(173,rt+79);EEPROM.write(174,lt+79);
   BDmenu6_save=false;}
if(BDmenu7_save==true){
EEPROM.write(186,wait);EEPROM.write(189,Loud);EEPROM.write(187,Loud_f);
   BDmenu7_save=false;}
if(R_menu!=R_menu_MOD){EEPROM.write(175,R_menu);R_menu_MOD=R_menu;}
if(factor==true){EEPROM.write(176,treb_c);EEPROM.write(177,mid_c);EEPROM.write(178,bas_c);
                 EEPROM.write(180,treb_q);EEPROM.write(181,mid_q);EEPROM.write(182,bas_q);
   factor=false;}
if(gain3!=gain3_MOD){EEPROM.write(185,gain3);gain3_MOD=gain3;}
if(BDmenu10_save==true){
EEPROM.write(150,bandwidth);EEPROM.write(151,softMute);
EEPROM.write(152,Emphasis);EEPROM.write(153,minfreqFM);
   BDmenu10_save=false;}
if(BDmenu11_save==true){
EEPROM.write(154,agcIdx);EEPROM.write(155,showRDS);EEPROM.write(156,showINFO);
   BDmenu11_save=false;}

EEPROM.commit();//СОХРАНЯЕМ 

#ifdef LOG_ENABLE
Serial.println("SAVE PARAMETRERS IN EEPROM");
#endif
}


//ЧАСТОТЫ ДЛЯ FM РАДИО И ОСНОВНЫЕ ПАРАМЕТРЫ МОЖНО ЗАДАТЬ С ПОМОЩЮ ФУНКЦИИ 
void Pre_settings(){
channel[1]=9930;channel[2]=10790;channel[3]=10370;channel[4]=10110;channel[5]=10190;
channel[6]=10480;channel[7]=10440;channel[8]=10610;channel[9]=10310;channel[10]=10690;
channel[11]=10520;channel[12]=8750;channel[13]=8830;channel[14]=8920;channel[15]=9500;
channel[16]=10250;channel[17]=10740;channel[18]=10060;channel[19]=10570;channel[20]=9980;
volume=-30;volume_MOD=0;treb=0;middle=0;bass=0;chanWEB=1;chanWEB_MOD=0;chanFM=1;chanFM_MOD=0;
volFM=63;volWEB=15;LCD_led=1;LCD_led_MOD=0;
rf=0;lf=0;rt=0;lt=0;treb_c=3;mid_c=2;bas_c=0;treb_q=1;mid_q=1;bas_q=1;R_menu=2;R_menu_MOD=0;
gain1=0;gain2=0;gain3=0;wait=25;Loud=0;Loud_f=0;bandwidth=0;softMute=30;Emphasis=1;
minfreqFM=1;agcIdx=0;showRDS=1;showINFO=1;BDmenu10_save=true;BDmenu11_save=true;
BDmenu1_save=true;BDmenu2_save=true;BDmenu6_save=true;BDmenu7_save=true;factor=true;
channelupdate();
}







//БИБЛИРТЕКА audio СОДЕРЖИТ СЛЕДУЮЩИЕ ИНФОРМАЦИОННЫЕ ФУНКЦИИ (ДЛЯ ОСОБО ПРОДВИНУТЫХ...)
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

//ОБНУЛЯЕМ ЭНКОДЕР, ПОДНИМАЕМ ФЛАГ ВЫПОЛНЕНИЯ ОДНОРАЗОВОЙ КОМАНДЫ 
//void cl_ENC(){encoderCount=0;timer_BTN=mill;}//timer_MENU=mill;
//ОБНУЛЯЕМ ТАЙМЕРЫ АВТОВОЗВРАТА В ОСНОВНОЕ МЕНЮ, ОЖИДАНИЯ НАЖАТИЯ КНОПКИ, ВЫВОДА ДАННЫХ В МЕНЮ
//void cl_TMR(){timer_AUTORET=mill;FL_com1=true;FL_com2=true;}

//АНИМАЦИЯ СТРЕЛОЧКИ В МЕНЮ ПОДКЛЮЧЕНИЯ ВНЕШНЕГО ВХОДА
//void Animation(){lcd.setCursor(0,0);if(mill>timer_MENU){timer_MENU=mill+p1000;animat=!animat; 
//if(animat==true){lcd.print(' ');lcd.write(arrow);}else{lcd.write(arrow);lcd.print(' ');}}}
