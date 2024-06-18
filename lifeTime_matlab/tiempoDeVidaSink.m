close all
clear all

format long

%%% Current consuption %%%
currentSleep = 15;
currentWorking = 46;
currentWifiTx = 125;
currentWiFiRx=125;
currentZigBeeTx=95;
currentZigBeeRx=90;

battery = 3500;

%Calcular tiempo de XBee
lengthBits = 312;
speedXBeeTx = 250000;

timeXBeeTx = lengthBits/speedXBeeTx;

    %Por día
timeDailyXBeeTx = timeXBeeTx*6*4;
timeDailyXBeeRx = timeDailyXBeeTx;

    %En horas
timeDailyXBeeRx=timeDailyXBeeRx/3600000;
timeDailyXBeeTx=timeDailyXBeeTx/3600000;

%Calcular tiempo de WiFi
lengthBitsWiFi = 160*8;
speedWiFiTx = 54000000;

timeWiFi = lengthBitsWiFi/speedWiFiTx;

    %Por día
timeDailyWiFiTx = timeWiFi*6*4;
timeDailyWiFiRx = timeDailyWiFiTx;

    %En horas
timeDailyWiFiRx=timeDailyWiFiRx/3600000;
timeDailyWiFiTx=timeDailyWiFiTx/3600000;

%Porcentaje de uso respecto a un día

timeActiveWiFiTx = timeDailyWiFiTx/24;
timeActiveWiFiRx = timeDailyWiFiRx/24;

timeActiveXbeeTx = timeDailyXBeeTx /24;
timeActiveXbeeRx = timeDailyXBeeRx/24;

timeDailySleep = 24-(timeActiveWiFiTx+timeActiveWiFiRx+timeActiveXbeeTx+timeActiveXbeeRx);
timeActiveSleep=timeDailySleep/24;

%Ecuación Prop
average=(timeActiveWiFiTx*currentWifiTx)+(timeActiveWiFiRx*currentWiFiRx)+(timeActiveXbeeTx*currentZigBeeTx)+(timeActiveXbeeRx*currentZigBeeRx)+(timeActiveSleep*currentSleep);
timeTotal = battery/average
totalDays = timeTotal/24