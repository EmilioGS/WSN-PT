close all
clear all

format long

%%% Current consuption %%%
currentTempSensor = 82;
currentNPKSensor = 90;
currentXBeeTx = 82;
currentXBeeRx = 77;
currentValveNPK = 165;
currentValveWater = 165;
currentSleep = 10;

activeValves = 1;

battery = 3500;

% Time use por vez
timeTempSensor = 0.00083333; %3 segundos expresados en Horas
timeDailyTempSensor = timeTempSensor*6;
timeNPKSensor = 0.00111111; %4 segundos expresados en Horas
timeDailyNPKSensor = timeNPKSensor*6;

timeValveNPK = 0.00138889; %5 segundos expresados en Horas
timeDailyValveNPK = timeValveNPK*activeValves;
timeValveWater = 0.00138889; %5 segundos expresados en Horas
timeDailyValveWater = timeValveWater*activeValves;

    %Calcular tiempo de XBee
lengthBits = 312;
speedXBeeTx = 250000;

timeXBeeTx = lengthBits/speedXBeeTx;

    %Por día
timeDailyXBeeTx = timeXBeeTx*6;
timeDailyXBeeRx = timeDailyXBeeTx;

    %En horas
timeDailyXBeeRx=timeDailyXBeeRx/3600000;
timeDailyXBeeTx=timeDailyXBeeTx/3600000;

%Porcentaje de uso respecto a un día

timeActiveTempSensor = timeDailyTempSensor/24;
timeActiveNPKSensor = timeDailyNPKSensor/24;

timeActiveValveWater = timeDailyValveWater/24;
timeActiveValveNPK = timeDailyValveNPK/24;

timeActiveXbeeTx = timeDailyXBeeTx /24;
timeActiveXbeeRx = timeDailyXBeeRx/24;

timeDailySleep = 24-(timeActiveTempSensor+timeActiveNPKSensor+timeActiveValveWater+timeActiveValveNPK+timeActiveXbeeTx+timeActiveXbeeRx);
timeActiveSleep=timeDailySleep/24;

%Ecuación Prop
average=(timeActiveTempSensor*currentTempSensor)+(timeActiveNPKSensor*currentNPKSensor)+(timeActiveXbeeTx*currentXBeeTx)+ (timeActiveXbeeRx*currentXBeeRx)+ (timeActiveSleep*currentSleep);
timeTotal = battery/average;
totalDays = timeTotal/24



