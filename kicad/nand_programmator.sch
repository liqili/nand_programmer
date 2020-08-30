EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "NANDO"
Date ""
Rev "v3.1"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L nand_programmator:STM32F103VCTx U2
U 1 1 5884C913
P 4450 3700
F 0 "U2" H 1750 6525 50  0000 L BNN
F 1 "STM32F103VCT6" H 7150 6525 50  0000 R BNN
F 2 "lib_fp:LQFP-100_14x14mm_Pitch0.5mm" H 7150 6475 50  0000 R TNN
F 3 "" H 4450 3700 50  0000 C CNN
F 4 "1624139" H 4450 3700 50  0001 C CNN "Farnell Ref"
	1    4450 3700
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:LM1117-3.3 U1
U 1 1 58851592
P 1650 7050
F 0 "U1" H 1750 6800 50  0000 C CNN
F 1 "LM1117-3.3" H 1650 7300 50  0000 C CNN
F 2 "lib_fp:SOT-223" H 1650 7050 50  0001 C CNN
F 3 "" H 1650 7050 50  0000 C CNN
F 4 "3007498" H 1650 7050 50  0001 C CNN "Farnell Ref"
	1    1650 7050
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+5V #PWR04
U 1 1 58851624
P 900 7000
F 0 "#PWR04" H 900 6850 50  0001 C CNN
F 1 "+5V" H 900 7140 50  0000 C CNN
F 2 "" H 900 7000 50  0000 C CNN
F 3 "" H 900 7000 50  0000 C CNN
	1    900  7000
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR07
U 1 1 5885184B
P 1650 7500
F 0 "#PWR07" H 1650 7250 50  0001 C CNN
F 1 "GNDD" H 1650 7350 50  0000 C CNN
F 2 "" H 1650 7500 50  0000 C CNN
F 3 "" H 1650 7500 50  0000 C CNN
	1    1650 7500
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:C C4
U 1 1 588518F7
P 1250 7250
F 0 "C4" H 1275 7350 50  0000 L CNN
F 1 "0.1uF" H 1275 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 1288 7100 50  0001 C CNN
F 3 "" H 1250 7250 50  0000 C CNN
F 4 "2320839" H 1250 7250 50  0001 C CNN "Farnell Ref"
	1    1250 7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR08
U 1 1 58851AFE
P 2450 7050
F 0 "#PWR08" H 2450 6900 50  0001 C CNN
F 1 "+3.3V" H 2450 7190 50  0000 C CNN
F 2 "" H 2450 7050 50  0000 C CNN
F 3 "" H 2450 7050 50  0000 C CNN
	1    2450 7050
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR011
U 1 1 58851FD8
P 4750 700
F 0 "#PWR011" H 4750 550 50  0001 C CNN
F 1 "+3.3V" H 4750 840 50  0000 C CNN
F 2 "" H 4750 700 50  0000 C CNN
F 3 "" H 4750 700 50  0000 C CNN
	1    4750 700 
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR012
U 1 1 5885248D
P 4750 6600
F 0 "#PWR012" H 4750 6350 50  0001 C CNN
F 1 "GNDD" H 4750 6450 50  0000 C CNN
F 2 "" H 4750 6600 50  0000 C CNN
F 3 "" H 4750 6600 50  0000 C CNN
	1    4750 6600
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+5V #PWR021
U 1 1 58853126
P 8750 5600
F 0 "#PWR021" H 8750 5450 50  0001 C CNN
F 1 "+5V" H 8750 5740 50  0000 C CNN
F 2 "" H 8750 5600 50  0000 C CNN
F 3 "" H 8750 5600 50  0000 C CNN
	1    8750 5600
	-1   0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR022
U 1 1 58853209
P 9150 6300
F 0 "#PWR022" H 9150 6050 50  0001 C CNN
F 1 "GNDD" H 9150 6150 50  0000 C CNN
F 2 "" H 9150 6300 50  0000 C CNN
F 3 "" H 9150 6300 50  0000 C CNN
	1    9150 6300
	-1   0    0    -1  
$EndComp
$Comp
L nand_programmator:R R5
U 1 1 5885374A
P 8150 5850
F 0 "R5" V 8230 5850 50  0000 C CNN
F 1 "1.5k" V 8150 5850 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 8080 5850 50  0001 C CNN
F 3 "" H 8150 5850 50  0000 C CNN
F 4 "2447592" H 8150 5850 50  0001 C CNN "Farnell Ref"
	1    8150 5850
	0    1    -1   0   
$EndComp
Text GLabel 8400 5250 1    60   BiDi ~ 0
USB_DP
Text GLabel 8550 5250 1    60   BiDi ~ 0
USB_DN
$Comp
L nand_programmator:R R7
U 1 1 58854FF3
P 8550 5500
F 0 "R7" V 8630 5500 50  0000 C CNN
F 1 "22" V 8550 5500 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 8480 5500 50  0001 C CNN
F 3 "" H 8550 5500 50  0000 C CNN
F 4 "2447609" H 8550 5500 50  0001 C CNN "Farnell Ref"
	1    8550 5500
	-1   0    0    -1  
$EndComp
$Comp
L nand_programmator:R R6
U 1 1 5885503E
P 8400 5500
F 0 "R6" V 8480 5500 50  0000 C CNN
F 1 "22" V 8400 5500 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 8330 5500 50  0001 C CNN
F 3 "" H 8400 5500 50  0000 C CNN
F 4 "2447609" H 8400 5500 50  0001 C CNN "Farnell Ref"
	1    8400 5500
	-1   0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR015
U 1 1 588553B9
P 7900 5800
F 0 "#PWR015" H 7900 5650 50  0001 C CNN
F 1 "+3.3V" H 7900 5940 50  0000 C CNN
F 2 "" H 7900 5800 50  0000 C CNN
F 3 "" H 7900 5800 50  0000 C CNN
	1    7900 5800
	-1   0    0    -1  
$EndComp
Text GLabel 7650 2250 2    60   BiDi ~ 0
USB_DN
Text GLabel 7650 2400 2    60   BiDi ~ 0
USB_DP
$Comp
L nand_programmator:C C1
U 1 1 58857883
P 850 1900
F 0 "C1" H 875 2000 50  0000 L CNN
F 1 "56pF" H 875 1800 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 888 1750 50  0001 C CNN
F 3 "" H 850 1900 50  0000 C CNN
F 4 "2905288" H 850 1900 50  0001 C CNN "Farnell Ref"
	1    850  1900
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:C C2
U 1 1 58857926
P 850 2200
F 0 "C2" H 875 2300 50  0000 L CNN
F 1 "56pF" H 875 2100 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 888 2050 50  0001 C CNN
F 3 "" H 850 2200 50  0000 C CNN
F 4 "2905288" H 850 2200 50  0001 C CNN "Farnell Ref"
	1    850  2200
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:GNDD #PWR03
U 1 1 58857997
P 600 2300
F 0 "#PWR03" H 600 2050 50  0001 C CNN
F 1 "GNDD" H 600 2150 50  0000 C CNN
F 2 "" H 600 2300 50  0000 C CNN
F 3 "" H 600 2300 50  0000 C CNN
	1    600  2300
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR02
U 1 1 5885825D
P 650 1500
F 0 "#PWR02" H 650 1250 50  0001 C CNN
F 1 "GNDD" H 650 1350 50  0000 C CNN
F 2 "" H 650 1500 50  0000 C CNN
F 3 "" H 650 1500 50  0000 C CNN
	1    650  1500
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR06
U 1 1 58858301
P 1500 1600
F 0 "#PWR06" H 1500 1450 50  0001 C CNN
F 1 "+3.3V" H 1500 1740 50  0000 C CNN
F 2 "" H 1500 1600 50  0000 C CNN
F 3 "" H 1500 1600 50  0000 C CNN
	1    1500 1600
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:R R2
U 1 1 588585FC
P 1400 1000
F 0 "R2" V 1480 1000 50  0000 C CNN
F 1 "10k" V 1400 1000 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 1330 1000 50  0001 C CNN
F 3 "" H 1400 1000 50  0000 C CNN
F 4 "2447553" H 1400 1000 50  0001 C CNN "Farnell Ref"
	1    1400 1000
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR05
U 1 1 58858683
P 1400 750
F 0 "#PWR05" H 1400 600 50  0001 C CNN
F 1 "+3.3V" H 1400 890 50  0000 C CNN
F 2 "" H 1400 750 50  0000 C CNN
F 3 "" H 1400 750 50  0000 C CNN
	1    1400 750 
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:LED D1
U 1 1 588D17A1
P 3900 7300
F 0 "D1" H 3900 7400 50  0000 C CNN
F 1 "GREEN" H 3900 7200 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 3900 7300 50  0001 C CNN
F 3 "" H 3900 7300 50  0000 C CNN
F 4 "2099239" H 3900 7300 50  0001 C CNN "Farnell Ref"
	1    3900 7300
	-1   0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR09
U 1 1 588D23AE
P 3250 7150
F 0 "#PWR09" H 3250 7000 50  0001 C CNN
F 1 "+3.3V" H 3250 7290 50  0000 C CNN
F 2 "" H 3250 7150 50  0000 C CNN
F 3 "" H 3250 7150 50  0000 C CNN
	1    3250 7150
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:R R3
U 1 1 588D249A
P 3500 7300
F 0 "R3" V 3580 7300 50  0000 C CNN
F 1 "1k" V 3500 7300 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 3430 7300 50  0001 C CNN
F 3 "" H 3500 7300 50  0000 C CNN
F 4 "2447587" H 3500 7300 50  0001 C CNN "Farnell Ref"
	1    3500 7300
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:GNDD #PWR010
U 1 1 588D2529
P 4200 7350
F 0 "#PWR010" H 4200 7100 50  0001 C CNN
F 1 "GNDD" H 4200 7200 50  0000 C CNN
F 2 "" H 4200 7350 50  0000 C CNN
F 3 "" H 4200 7350 50  0000 C CNN
	1    4200 7350
	1    0    0    -1  
$EndComp
Text GLabel 9900 4650 0    60   BiDi ~ 0
SWDIO
Text GLabel 9900 4450 0    60   Output ~ 0
SWCLK
Text GLabel 7650 2550 2    60   BiDi ~ 0
SWDIO
Text GLabel 7650 2700 2    60   Input ~ 0
SWCLK
Text GLabel 1150 6300 0    60   BiDi ~ 0
FSMC_D0
Text GLabel 1150 6450 0    60   BiDi ~ 0
FSMC_D1
Text GLabel 1150 4600 0    60   BiDi ~ 0
FSMC_D2
Text GLabel 1150 4750 0    60   BiDi ~ 0
FSMC_D3
Text GLabel 1150 3250 0    60   BiDi ~ 0
FSMC_D4
Text GLabel 1150 3400 0    60   BiDi ~ 0
FSMC_D5
Text GLabel 1150 3550 0    60   BiDi ~ 0
FSMC_D6
Text GLabel 1150 3700 0    60   BiDi ~ 0
FSMC_D7
Text GLabel 1150 5400 0    60   Output ~ 0
FSMC_NCE2
Text GLabel 1150 5100 0    60   Output ~ 0
FSMC_NWE
Text GLabel 1150 4950 0    60   Output ~ 0
FSMC_NOE
Text GLabel 1150 5250 0    60   Input ~ 0
FSMC_NWAIT
Text GLabel 10650 3600 2    60   BiDi ~ 0
FSMC_D0
Text GLabel 9500 3300 0    60   BiDi ~ 0
FSMC_D1
Text GLabel 10650 3450 2    60   BiDi ~ 0
FSMC_D2
Text GLabel 9500 3150 0    60   BiDi ~ 0
FSMC_D3
Text GLabel 10650 2800 2    60   BiDi ~ 0
FSMC_D4
Text GLabel 9500 2800 0    60   BiDi ~ 0
FSMC_D5
Text GLabel 10650 2650 2    60   BiDi ~ 0
FSMC_D6
Text GLabel 9500 2650 0    60   BiDi ~ 0
FSMC_D7
Text GLabel 9500 1250 0    60   Input ~ 0
FSMC_NCE2
Text GLabel 10650 1750 2    60   Input ~ 0
FSMC_NWE
Text GLabel 10650 1050 2    60   Input ~ 0
FSMC_NOE
Text GLabel 9500 1100 0    60   Output ~ 0
FSMC_NWAIT
$Comp
L nand_programmator:R R12
U 1 1 588EC712
P 9600 900
F 0 "R12" V 9680 900 50  0000 C CNN
F 1 "10k" V 9600 900 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 9530 900 50  0001 C CNN
F 3 "" H 9600 900 50  0000 C CNN
F 4 "2447553" H 9600 900 50  0001 C CNN "Farnell Ref"
	1    9600 900 
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR026
U 1 1 588EC76B
P 9600 700
F 0 "#PWR026" H 9600 550 50  0001 C CNN
F 1 "+3.3V" H 9600 840 50  0000 C CNN
F 2 "" H 9600 700 50  0000 C CNN
F 3 "" H 9600 700 50  0000 C CNN
	1    9600 700 
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR027
U 1 1 588ECFFC
P 10600 1300
F 0 "#PWR027" H 10600 1150 50  0001 C CNN
F 1 "+3.3V" H 10600 1440 50  0000 C CNN
F 2 "" H 10600 1300 50  0000 C CNN
F 3 "" H 10600 1300 50  0000 C CNN
	1    10600 1300
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR028
U 1 1 588ED731
P 11000 1300
F 0 "#PWR028" H 11000 1050 50  0001 C CNN
F 1 "GNDD" H 11000 1150 50  0000 C CNN
F 2 "" H 11000 1300 50  0000 C CNN
F 3 "" H 11000 1300 50  0000 C CNN
	1    11000 1300
	0    -1   -1   0   
$EndComp
Text GLabel 1150 6000 0    60   Output ~ 0
FSMC_CLE
Text GLabel 1150 6150 0    60   Output ~ 0
FSMC_ALE
Text GLabel 9400 1600 0    60   Input ~ 0
FSMC_ALE
$Comp
L nand_programmator:R R11
U 1 1 588EF55B
P 9200 1750
F 0 "R11" V 9280 1750 50  0000 C CNN
F 1 "10k" V 9200 1750 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 9130 1750 50  0001 C CNN
F 3 "" H 9200 1750 50  0000 C CNN
F 4 "2447553" H 9200 1750 50  0001 C CNN "Farnell Ref"
	1    9200 1750
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:R R10
U 1 1 588F3B03
P 9100 1400
F 0 "R10" V 9180 1400 50  0000 C CNN
F 1 "10k" V 9100 1400 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 9030 1400 50  0001 C CNN
F 3 "" H 9100 1400 50  0000 C CNN
F 4 "2447553" H 9100 1400 50  0001 C CNN "Farnell Ref"
	1    9100 1400
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:GNDD #PWR030
U 1 1 588F4BB2
P 11000 3050
F 0 "#PWR030" H 11000 2800 50  0001 C CNN
F 1 "GNDD" H 11000 2900 50  0000 C CNN
F 2 "" H 11000 3050 50  0000 C CNN
F 3 "" H 11000 3050 50  0000 C CNN
	1    11000 3050
	0    -1   -1   0   
$EndComp
$Comp
L nand_programmator:+3.3V #PWR029
U 1 1 588F4C2E
P 10600 3050
F 0 "#PWR029" H 10600 2900 50  0001 C CNN
F 1 "+3.3V" H 10600 3190 50  0000 C CNN
F 2 "" H 10600 3050 50  0000 C CNN
F 3 "" H 10600 3050 50  0000 C CNN
	1    10600 3050
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:C C14
U 1 1 589143F1
P 10800 3050
F 0 "C14" H 10825 3150 50  0000 L CNN
F 1 "0.1uF" H 10825 2950 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 10838 2900 50  0001 C CNN
F 3 "" H 10800 3050 50  0000 C CNN
F 4 "2320839" H 10800 3050 50  0001 C CNN "Farnell Ref"
	1    10800 3050
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:C C13
U 1 1 58914F0F
P 10800 1300
F 0 "C13" H 10825 1400 50  0000 L CNN
F 1 "0.1uF" H 10825 1200 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 10838 1150 50  0001 C CNN
F 3 "" H 10800 1300 50  0000 C CNN
F 4 "2320839" H 10800 1300 50  0001 C CNN "Farnell Ref"
	1    10800 1300
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:C C8
U 1 1 58919709
P 5150 7250
F 0 "C8" H 5175 7350 50  0000 L CNN
F 1 "0.1uF" H 5175 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 5188 7100 50  0001 C CNN
F 3 "" H 5150 7250 50  0000 C CNN
F 4 "2320839" H 5150 7250 50  0001 C CNN "Farnell Ref"
	1    5150 7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:C C9
U 1 1 58919D82
P 5500 7250
F 0 "C9" H 5525 7350 50  0000 L CNN
F 1 "0.1uF" H 5525 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 5538 7100 50  0001 C CNN
F 3 "" H 5500 7250 50  0000 C CNN
F 4 "2320839" H 5500 7250 50  0001 C CNN "Farnell Ref"
	1    5500 7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:C C10
U 1 1 58919FC6
P 5850 7250
F 0 "C10" H 5875 7350 50  0000 L CNN
F 1 "0.1uF" H 5875 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 5888 7100 50  0001 C CNN
F 3 "" H 5850 7250 50  0000 C CNN
F 4 "2320839" H 5850 7250 50  0001 C CNN "Farnell Ref"
	1    5850 7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:C C11
U 1 1 5891A0C3
P 6200 7250
F 0 "C11" H 6225 7350 50  0000 L CNN
F 1 "0.1uF" H 6225 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 6238 7100 50  0001 C CNN
F 3 "" H 6200 7250 50  0000 C CNN
F 4 "2320839" H 6200 7250 50  0001 C CNN "Farnell Ref"
	1    6200 7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:C C12
U 1 1 5891A177
P 6550 7250
F 0 "C12" H 6575 7350 50  0000 L CNN
F 1 "0.1uF" H 6575 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 6588 7100 50  0001 C CNN
F 3 "" H 6550 7250 50  0000 C CNN
F 4 "2320839" H 6550 7250 50  0001 C CNN "Farnell Ref"
	1    6550 7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR014
U 1 1 5891BBFE
P 6550 7500
F 0 "#PWR014" H 6550 7250 50  0001 C CNN
F 1 "GNDD" H 6550 7350 50  0000 C CNN
F 2 "" H 6550 7500 50  0000 C CNN
F 3 "" H 6550 7500 50  0000 C CNN
	1    6550 7500
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR025
U 1 1 588F4456
P 9000 1750
F 0 "#PWR025" H 9000 1600 50  0001 C CNN
F 1 "+3.3V" H 9000 1890 50  0000 C CNN
F 2 "" H 9000 1750 50  0000 C CNN
F 3 "" H 9000 1750 50  0000 C CNN
	1    9000 1750
	0    -1   -1   0   
$EndComp
$Comp
L nand_programmator:C C7
U 1 1 588FCFBD
P 4850 7250
F 0 "C7" H 4875 7350 50  0000 L CNN
F 1 "0.1uF" H 4875 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 4888 7100 50  0001 C CNN
F 3 "" H 4850 7250 50  0000 C CNN
F 4 "2320839" H 4850 7250 50  0001 C CNN "Farnell Ref"
	1    4850 7250
	1    0    0    -1  
$EndComp
Wire Wire Line
	900  7050 1250 7050
Wire Wire Line
	1650 7350 1650 7450
Connection ~ 1250 7050
Wire Wire Line
	1950 7050 2050 7050
Connection ~ 2050 7050
Wire Wire Line
	4150 700  4250 700 
Connection ~ 4550 700 
Connection ~ 4450 700 
Connection ~ 4350 700 
Connection ~ 4250 700 
Connection ~ 4650 700 
Wire Wire Line
	4150 6600 4250 6600
Connection ~ 4250 6600
Connection ~ 4350 6600
Connection ~ 4450 6600
Connection ~ 4550 6600
Connection ~ 4650 6600
Wire Wire Line
	8400 5650 8400 5850
Wire Wire Line
	8300 5850 8400 5850
Wire Wire Line
	8550 5250 8550 5350
Wire Wire Line
	8400 5250 8400 5350
Wire Wire Line
	7900 5850 8000 5850
Wire Wire Line
	7650 2250 7550 2250
Wire Wire Line
	7550 2250 7550 2300
Wire Wire Line
	7550 2300 7350 2300
Wire Wire Line
	7350 2400 7650 2400
Wire Wire Line
	1550 2000 1450 2000
Wire Wire Line
	1450 2000 1450 1900
Wire Wire Line
	1450 2200 1450 2100
Wire Wire Line
	1450 2100 1550 2100
Wire Wire Line
	700  1900 600  1900
Wire Wire Line
	700  2200 600  2200
Connection ~ 600  2200
Wire Wire Line
	1500 1600 1550 1600
Wire Wire Line
	1400 750  1400 850 
Wire Wire Line
	1400 1150 1400 1200
Wire Wire Line
	3250 7150 3250 7300
Wire Wire Line
	3250 7300 3350 7300
Wire Wire Line
	3650 7300 3750 7300
Wire Wire Line
	4050 7300 4200 7300
Wire Wire Line
	4200 7300 4200 7350
Wire Wire Line
	7900 5800 7900 5850
Wire Wire Line
	7550 2500 7350 2500
Wire Wire Line
	7550 2500 7550 2550
Wire Wire Line
	7550 2550 7650 2550
Wire Wire Line
	7350 2600 7550 2600
Wire Wire Line
	7550 2600 7550 2700
Wire Wire Line
	7550 2700 7650 2700
Wire Wire Line
	1350 4700 1550 4700
Wire Wire Line
	1350 4950 1350 5000
Wire Wire Line
	1350 5000 1550 5000
Wire Wire Line
	1350 5400 1350 5300
Wire Wire Line
	1350 5300 1550 5300
Wire Wire Line
	1350 5250 1350 5200
Wire Wire Line
	1350 5200 1550 5200
Wire Wire Line
	4850 7100 5150 7100
Connection ~ 5500 7100
Connection ~ 5850 7100
Connection ~ 6200 7100
Wire Wire Line
	4850 7400 5150 7400
Connection ~ 5500 7400
Connection ~ 5850 7400
Connection ~ 6200 7400
Wire Wire Line
	6550 7100 6550 7000
Wire Wire Line
	6550 7400 6550 7500
Wire Wire Line
	1350 4700 1350 4750
Wire Wire Line
	9000 1750 9050 1750
Connection ~ 5150 7100
Connection ~ 5150 7400
Wire Wire Line
	900  7000 900  7050
Wire Wire Line
	900  7400 900  7450
Wire Wire Line
	900  7450 1250 7450
Connection ~ 1650 7450
Wire Wire Line
	1250 7400 1250 7450
Connection ~ 1250 7450
$Comp
L nand_programmator:C C5
U 1 1 58900C4D
P 2050 7250
F 0 "C5" H 2075 7350 50  0000 L CNN
F 1 "0.1uF" H 2075 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 2088 7100 50  0001 C CNN
F 3 "" H 2050 7250 50  0000 C CNN
F 4 "2320839" H 2050 7250 50  0001 C CNN "Farnell Ref"
	1    2050 7250
	1    0    0    -1  
$EndComp
Wire Wire Line
	2050 7450 2050 7400
Wire Wire Line
	2450 7050 2450 7100
Wire Wire Line
	2450 7450 2450 7400
Connection ~ 2050 7450
Wire Wire Line
	2050 7100 2050 7050
Wire Wire Line
	1250 7050 1250 7100
Connection ~ 900  7050
$Comp
L nand_programmator:C C3
U 1 1 58903E4F
P 900 7250
F 0 "C3" H 925 7350 50  0000 L CNN
F 1 "10uF" H 925 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 938 7100 50  0001 C CNN
F 3 "" H 900 7250 50  0000 C CNN
F 4 "2112850" H 900 7250 50  0001 C CNN "Farnell Ref"
	1    900  7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:C C6
U 1 1 5890498A
P 2450 7250
F 0 "C6" H 2475 7350 50  0000 L CNN
F 1 "10uF" H 2475 7150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 2488 7100 50  0001 C CNN
F 3 "" H 2450 7250 50  0000 C CNN
F 4 "2112850" H 2450 7250 50  0001 C CNN "Farnell Ref"
	1    2450 7250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR018
U 1 1 5890602E
P 8900 1400
F 0 "#PWR018" H 8900 1250 50  0001 C CNN
F 1 "+3.3V" H 8900 1540 50  0000 C CNN
F 2 "" H 8900 1400 50  0000 C CNN
F 3 "" H 8900 1400 50  0000 C CNN
	1    8900 1400
	0    -1   -1   0   
$EndComp
$Comp
L nand_programmator:+3.3V #PWR013
U 1 1 5895143A
P 6550 7000
F 0 "#PWR013" H 6550 6850 50  0001 C CNN
F 1 "+3.3V" H 6550 7140 50  0000 C CNN
F 2 "" H 6550 7000 50  0000 C CNN
F 3 "" H 6550 7000 50  0000 C CNN
	1    6550 7000
	1    0    0    -1  
$EndComp
Wire Wire Line
	1250 7050 1350 7050
Wire Wire Line
	2050 7050 2450 7050
Wire Wire Line
	4550 700  4650 700 
Wire Wire Line
	4450 700  4550 700 
Wire Wire Line
	4350 700  4450 700 
Wire Wire Line
	4250 700  4350 700 
Wire Wire Line
	4650 700  4750 700 
Wire Wire Line
	4250 6600 4350 6600
Wire Wire Line
	4350 6600 4450 6600
Wire Wire Line
	4450 6600 4550 6600
Wire Wire Line
	4550 6600 4650 6600
Wire Wire Line
	4650 6600 4750 6600
Wire Wire Line
	9150 6250 9150 6300
Wire Wire Line
	600  2200 600  2300
Wire Wire Line
	1400 1200 1550 1200
Wire Wire Line
	5500 7100 5850 7100
Wire Wire Line
	5850 7100 6200 7100
Wire Wire Line
	6200 7100 6550 7100
Wire Wire Line
	5500 7400 5850 7400
Wire Wire Line
	5850 7400 6200 7400
Wire Wire Line
	6200 7400 6550 7400
Wire Wire Line
	5150 7100 5500 7100
Wire Wire Line
	5150 7400 5500 7400
Wire Wire Line
	1650 7450 1650 7500
Wire Wire Line
	1650 7450 2050 7450
Wire Wire Line
	1250 7450 1650 7450
Wire Wire Line
	2050 7450 2450 7450
Wire Wire Line
	900  7050 900  7100
$Comp
L nand_programmator:GNDD #PWR016
U 1 1 5C06872D
P 7950 3100
F 0 "#PWR016" H 7950 2850 50  0001 C CNN
F 1 "GNDD" V 7955 2972 50  0000 R CNN
F 2 "" H 7950 3100 50  0000 C CNN
F 3 "" H 7950 3100 50  0000 C CNN
	1    7950 3100
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:R R4
U 1 1 5C0B8AF4
P 7750 3100
F 0 "R4" V 7830 3100 50  0000 C CNN
F 1 "10k" V 7750 3100 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 7680 3100 50  0001 C CNN
F 3 "" H 7750 3100 50  0000 C CNN
F 4 "2447553" H 7750 3100 50  0001 C CNN "Farnell Ref"
	1    7750 3100
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J1
U 1 1 5C069405
P 10650 4450
F 0 "J1" H 10730 4442 50  0000 L CNN
F 1 "Conn_01x04" H 10730 4351 50  0000 L CNN
F 2 "lib_fp:PinHeader_1x04_P2.54mm_Vertical" H 10650 4450 50  0001 C CNN
F 3 "~" H 10650 4450 50  0001 C CNN
F 4 "3049529" H 10650 4450 50  0001 C CNN "Farnell Ref"
	1    10650 4450
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR023
U 1 1 5C091AF2
P 10400 4300
F 0 "#PWR023" H 10400 4150 50  0001 C CNN
F 1 "+3.3V" H 10400 4440 50  0000 C CNN
F 2 "" H 10400 4300 50  0000 C CNN
F 3 "" H 10400 4300 50  0000 C CNN
	1    10400 4300
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR019
U 1 1 5C27F907
P 9950 4150
F 0 "#PWR019" H 9950 3900 50  0001 C CNN
F 1 "GNDD" H 9955 3977 50  0000 C CNN
F 2 "" H 9950 4150 50  0000 C CNN
F 3 "" H 9950 4150 50  0000 C CNN
	1    9950 4150
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:R R8
U 1 1 5C2FEBFB
P 10150 4250
F 0 "R8" V 10230 4250 50  0000 C CNN
F 1 "10k" V 10150 4250 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 10080 4250 50  0001 C CNN
F 3 "" H 10150 4250 50  0000 C CNN
F 4 "2447553" H 10150 4250 50  0001 C CNN "Farnell Ref"
	1    10150 4250
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:R R9
U 1 1 5C48B28F
P 10150 4850
F 0 "R9" V 10230 4850 50  0000 C CNN
F 1 "10k" V 10150 4850 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 10080 4850 50  0001 C CNN
F 3 "" H 10150 4850 50  0000 C CNN
F 4 "2447553" H 10150 4850 50  0001 C CNN "Farnell Ref"
	1    10150 4850
	1    0    0    1   
$EndComp
Wire Wire Line
	9900 4450 10150 4450
Wire Wire Line
	10400 4300 10400 4350
Wire Wire Line
	10400 4350 10450 4350
Wire Wire Line
	9950 4150 9950 4100
Wire Wire Line
	10150 4400 10150 4450
Connection ~ 10150 4450
Wire Wire Line
	10150 4450 10450 4450
Wire Wire Line
	9900 4650 10150 4650
$Comp
L nand_programmator:GNDD #PWR024
U 1 1 5CC873DE
P 10400 4700
F 0 "#PWR024" H 10400 4450 50  0001 C CNN
F 1 "GNDD" H 10405 4527 50  0000 C CNN
F 2 "" H 10400 4700 50  0000 C CNN
F 3 "" H 10400 4700 50  0000 C CNN
	1    10400 4700
	1    0    0    -1  
$EndComp
Wire Wire Line
	10400 4700 10400 4550
Wire Wire Line
	10400 4550 10450 4550
Wire Wire Line
	10150 4700 10150 4650
Connection ~ 10150 4650
Wire Wire Line
	10150 4650 10450 4650
$Comp
L nand_programmator:+3.3V #PWR020
U 1 1 5CD08965
P 9950 4950
F 0 "#PWR020" H 9950 4800 50  0001 C CNN
F 1 "+3.3V" H 9950 5090 50  0000 C CNN
F 2 "" H 9950 4950 50  0000 C CNN
F 3 "" H 9950 4950 50  0000 C CNN
	1    9950 4950
	1    0    0    -1  
$EndComp
Wire Wire Line
	9950 4950 9950 5000
Wire Wire Line
	9950 5000 10150 5000
Wire Wire Line
	9950 4100 10150 4100
Wire Wire Line
	7900 3100 7950 3100
Wire Wire Line
	7350 3100 7600 3100
$Comp
L Connector_Generic:Conn_01x03 J2
U 1 1 5D0A1D76
P 8100 1900
F 0 "J2" H 8180 1942 50  0000 L CNN
F 1 "Conn_01x03" H 8180 1851 50  0000 L CNN
F 2 "lib_fp:PinHeader_1x03_P2.54mm_Vertical" H 8100 1900 50  0001 C CNN
F 3 "~" H 8100 1900 50  0001 C CNN
F 4 "3049527" H 8100 1900 50  0001 C CNN "Farnell Ref"
	1    8100 1900
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:GNDD #PWR031
U 1 1 5D0AECED
P 7750 1550
F 0 "#PWR031" H 7750 1300 50  0001 C CNN
F 1 "GNDD" V 7755 1422 50  0000 R CNN
F 2 "" H 7750 1550 50  0000 C CNN
F 3 "" H 7750 1550 50  0000 C CNN
	1    7750 1550
	1    0    0    -1  
$EndComp
Wire Wire Line
	7350 2200 7550 2200
Wire Wire Line
	7750 1550 7750 1500
Wire Wire Line
	7550 2200 7550 2000
Wire Wire Line
	7550 2000 7900 2000
Wire Wire Line
	7900 1900 7500 1900
Wire Wire Line
	7500 1900 7500 2100
Wire Wire Line
	7500 2100 7350 2100
Wire Wire Line
	7750 1500 7900 1500
Wire Wire Line
	7900 1500 7900 1800
$Comp
L nand_programmator:LED D2
U 1 1 5D1A0E8C
P 8150 700
F 0 "D2" H 8150 800 50  0000 C CNN
F 1 "RED" H 8150 600 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 8150 700 50  0001 C CNN
F 3 "" H 8150 700 50  0000 C CNN
F 4 "2099236" H 8150 700 50  0001 C CNN "Farnell Ref"
	1    8150 700 
	-1   0    0    -1  
$EndComp
$Comp
L nand_programmator:R R13
U 1 1 5D1A0E93
P 7750 700
F 0 "R13" V 7830 700 50  0000 C CNN
F 1 "1k" V 7750 700 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 7680 700 50  0001 C CNN
F 3 "" H 7750 700 50  0000 C CNN
F 4 "2447587" H 7750 700 50  0001 C CNN "Farnell Ref"
	1    7750 700 
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:GNDD #PWR032
U 1 1 5D1A0E9A
P 8450 750
F 0 "#PWR032" H 8450 500 50  0001 C CNN
F 1 "GNDD" H 8450 600 50  0000 C CNN
F 2 "" H 8450 750 50  0000 C CNN
F 3 "" H 8450 750 50  0000 C CNN
	1    8450 750 
	1    0    0    -1  
$EndComp
Wire Wire Line
	7900 700  8000 700 
Wire Wire Line
	8300 700  8450 700 
Wire Wire Line
	8450 700  8450 750 
$Comp
L nand_programmator:LED D3
U 1 1 5D1AEE1F
P 8150 1000
F 0 "D3" H 8150 1100 50  0000 C CNN
F 1 "YELLOW" H 8150 900 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 8150 1000 50  0001 C CNN
F 3 "" H 8150 1000 50  0000 C CNN
F 4 "2099243" H 8150 1000 50  0001 C CNN "Farnell Ref"
	1    8150 1000
	-1   0    0    -1  
$EndComp
$Comp
L nand_programmator:R R14
U 1 1 5D1AEE26
P 7750 1000
F 0 "R14" V 7830 1000 50  0000 C CNN
F 1 "1k" V 7750 1000 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 7680 1000 50  0001 C CNN
F 3 "" H 7750 1000 50  0000 C CNN
F 4 "2447587" H 7750 1000 50  0001 C CNN "Farnell Ref"
	1    7750 1000
	0    1    1    0   
$EndComp
$Comp
L nand_programmator:GNDD #PWR033
U 1 1 5D1AEE2D
P 8450 1050
F 0 "#PWR033" H 8450 800 50  0001 C CNN
F 1 "GNDD" H 8450 900 50  0000 C CNN
F 2 "" H 8450 1050 50  0000 C CNN
F 3 "" H 8450 1050 50  0000 C CNN
	1    8450 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	7900 1000 8000 1000
Wire Wire Line
	8300 1000 8450 1000
Wire Wire Line
	8450 1000 8450 1050
Wire Wire Line
	7600 1000 7600 1300
Wire Wire Line
	7600 1300 7350 1300
Wire Wire Line
	7350 1200 7550 1200
Wire Wire Line
	7550 1200 7550 700 
Wire Wire Line
	7550 700  7600 700 
Text Label 8600 5950 0    60   ~ 0
USB_N
Text Label 8600 5850 0    60   ~ 0
USB_P
$Comp
L Device:Crystal Y1
U 1 1 5C102083
P 1150 2050
F 0 "Y1" V 1341 2096 50  0000 L TNN
F 1 "Crystal_GND24_8MHz_30pF" H 1341 2005 50  0000 R TNN
F 2 "Crystal:Crystal_SMD_HC49-SD" H 1150 2050 50  0001 C CNN
F 3 "~" H 1150 2050 50  0001 C CNN
F 4 "2449408" H 1150 2050 50  0001 C CNN "Farnell Ref"
	1    1150 2050
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1000 1900 1150 1900
Wire Wire Line
	1000 2200 1150 2200
Connection ~ 1150 1900
Wire Wire Line
	1150 1900 1450 1900
Connection ~ 1150 2200
Wire Wire Line
	1150 2200 1450 2200
$Comp
L nand_programmator:Conn_02x12_Odd_Even_Left J3
U 1 1 5CFB4F1C
P 10100 1300
F 0 "J3" H 10150 2017 50  0000 C CNN
F 1 "Conn_02x12_Odd_Even_Left" H 10150 1926 50  0000 C CNN
F 2 "lib_fp:PinHeader_2x12_P2.54mm_Vertical" H 10100 1300 50  0001 C CNN
F 3 "~" H 10100 1300 50  0001 C CNN
F 4 "3049468" H 10100 1300 50  0001 C CNN "Farnell Ref"
	1    10100 1300
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:Conn_02x12_Odd_Even_Right J4
U 1 1 5CFB50BB
P 10100 3050
F 0 "J4" H 10150 3767 50  0000 C CNN
F 1 "Conn_02x12_Odd_Even_Right" H 10150 3676 50  0000 C CNN
F 2 "lib_fp:PinHeader_2x12_P2.54mm_Vertical_2" H 10100 3050 50  0001 C CNN
F 3 "~" H 10100 3050 50  0001 C CNN
F 4 "3049468" H 10100 3050 50  0001 C CNN "Farnell Ref"
	1    10100 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	9500 1100 9600 1100
Wire Wire Line
	9600 1050 9600 1100
Connection ~ 9600 1100
Wire Wire Line
	9600 1100 9900 1100
Wire Wire Line
	9600 700  9600 750 
Wire Wire Line
	9500 1250 9600 1250
Wire Wire Line
	9600 1250 9600 1200
Wire Wire Line
	9600 1200 9900 1200
Wire Wire Line
	9250 1400 9600 1400
Wire Wire Line
	9600 1400 9600 1250
Connection ~ 9600 1250
Wire Wire Line
	10950 1300 11000 1300
$Comp
L nand_programmator:GNDD #PWR017
U 1 1 5D26F973
P 9700 1400
F 0 "#PWR017" H 9700 1150 50  0001 C CNN
F 1 "GNDD" H 9700 1250 50  0000 C CNN
F 2 "" H 9700 1400 50  0000 C CNN
F 3 "" H 9700 1400 50  0000 C CNN
	1    9700 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	9700 1400 9900 1400
Wire Wire Line
	10400 1300 10600 1300
Wire Wire Line
	10400 1500 10600 1500
Wire Wire Line
	9400 1600 9900 1600
Wire Wire Line
	10650 1750 10550 1750
Wire Wire Line
	10550 1600 10400 1600
Wire Wire Line
	9450 1750 9450 1700
Wire Wire Line
	9450 1700 9900 1700
Wire Wire Line
	9350 1750 9450 1750
Wire Wire Line
	8900 1400 8950 1400
Wire Wire Line
	10650 3050 10600 3050
Wire Wire Line
	10950 3050 11000 3050
$Comp
L nand_programmator:GNDD #PWR034
U 1 1 5D5989E0
P 9900 3150
F 0 "#PWR034" H 9900 2900 50  0001 C CNN
F 1 "GNDD" H 9900 3000 50  0000 C CNN
F 2 "" H 9900 3150 50  0000 C CNN
F 3 "" H 9900 3150 50  0000 C CNN
	1    9900 3150
	0    1    1    0   
$EndComp
Wire Wire Line
	10600 2850 10400 2850
Wire Wire Line
	10400 1100 10550 1100
Connection ~ 10600 1300
Wire Wire Line
	10600 1300 10650 1300
Wire Wire Line
	10650 1050 10550 1050
Wire Wire Line
	10550 1050 10550 1100
Wire Wire Line
	10600 1600 10650 1600
Wire Wire Line
	10600 1500 10600 1600
Text GLabel 10650 1600 2    60   Input ~ 0
FSMC_CLE
Wire Wire Line
	10550 1750 10550 1600
Connection ~ 10600 3050
Wire Wire Line
	10600 3050 10500 3050
Wire Wire Line
	10600 2850 10600 2800
Wire Wire Line
	10600 2800 10650 2800
Wire Wire Line
	10600 2750 10600 2650
Wire Wire Line
	10600 2650 10650 2650
Wire Wire Line
	10400 2750 10600 2750
Text GLabel 9500 3650 0    60   BiDi ~ 0
FSMC_D8
Wire Wire Line
	9500 3650 9900 3650
Text GLabel 1150 3850 0    60   BiDi ~ 0
FSMC_D8
Text GLabel 10650 3750 2    60   BiDi ~ 0
FSMC_D9
Text GLabel 1150 4000 0    60   BiDi ~ 0
FSMC_D9
Wire Wire Line
	1500 3250 1500 3600
Wire Wire Line
	1500 3600 1550 3600
Wire Wire Line
	1450 3400 1450 3700
Wire Wire Line
	1450 3700 1550 3700
Wire Wire Line
	1400 3550 1400 3800
Wire Wire Line
	1400 3800 1550 3800
Wire Wire Line
	1350 3700 1350 3900
Wire Wire Line
	1350 3900 1550 3900
Wire Wire Line
	1300 3850 1300 4000
Wire Wire Line
	1300 4000 1550 4000
Wire Wire Line
	1150 3850 1300 3850
Wire Wire Line
	1150 3700 1350 3700
Wire Wire Line
	1150 3550 1400 3550
Wire Wire Line
	1150 3400 1450 3400
Wire Wire Line
	1150 3250 1500 3250
Wire Wire Line
	1150 4000 1250 4000
Wire Wire Line
	1250 4000 1250 4100
Wire Wire Line
	1250 4100 1550 4100
Wire Wire Line
	9500 3150 9600 3150
Wire Wire Line
	9600 3150 9600 3350
Wire Wire Line
	9600 3350 9900 3350
Wire Wire Line
	9500 3300 9550 3300
Wire Wire Line
	9550 3300 9550 3450
Wire Wire Line
	9550 3450 9900 3450
Text GLabel 9500 3500 0    60   BiDi ~ 0
FSMC_D10
Wire Wire Line
	9500 3500 9550 3500
Wire Wire Line
	9550 3500 9550 3550
Wire Wire Line
	9550 3550 9900 3550
Text GLabel 1150 4150 0    60   BiDi ~ 0
FSMC_D10
Wire Wire Line
	1150 4150 1250 4150
Wire Wire Line
	1250 4150 1250 4200
Wire Wire Line
	1250 4200 1550 4200
Text GLabel 10650 3300 2    60   BiDi ~ 0
FSMC_D11
Wire Wire Line
	10400 3550 10500 3550
Wire Wire Line
	10500 3550 10500 3750
Wire Wire Line
	10500 3750 10650 3750
Wire Wire Line
	10400 3450 10550 3450
Wire Wire Line
	10550 3450 10550 3600
Wire Wire Line
	10550 3600 10650 3600
Wire Wire Line
	10400 3350 10600 3350
Wire Wire Line
	10600 3350 10600 3450
Wire Wire Line
	10600 3450 10650 3450
Wire Wire Line
	10400 3250 10600 3250
Wire Wire Line
	10600 3250 10600 3300
Wire Wire Line
	10600 3300 10650 3300
Text GLabel 1150 4300 0    60   BiDi ~ 0
FSMC_D11
Wire Wire Line
	1150 4300 1550 4300
Text GLabel 9500 2950 0    60   BiDi ~ 0
FSMC_D12
Wire Wire Line
	9500 2650 9550 2650
Wire Wire Line
	9550 2650 9550 2750
Wire Wire Line
	9550 2750 9900 2750
Wire Wire Line
	9500 2800 9550 2800
Wire Wire Line
	9550 2800 9550 2850
Wire Wire Line
	9550 2850 9900 2850
Wire Wire Line
	9500 2950 9900 2950
Text GLabel 1150 4450 0    60   BiDi ~ 0
FSMC_D12
Wire Wire Line
	1150 4450 1250 4450
Wire Wire Line
	1250 4450 1250 4400
Wire Wire Line
	1250 4400 1550 4400
Wire Wire Line
	1150 4600 1550 4600
Wire Wire Line
	1150 4750 1350 4750
Wire Wire Line
	1150 4950 1350 4950
Wire Wire Line
	1150 5100 1550 5100
Wire Wire Line
	1150 5250 1350 5250
Wire Wire Line
	1150 5400 1350 5400
Text GLabel 10650 2500 2    60   BiDi ~ 0
FSMC_D13
Wire Wire Line
	10400 2650 10550 2650
Wire Wire Line
	10550 2650 10550 2500
Wire Wire Line
	10550 2500 10650 2500
Text GLabel 1150 5550 0    60   BiDi ~ 0
FSMC_D13
Connection ~ 2450 7050
Wire Wire Line
	1550 6100 1500 6100
Wire Wire Line
	1550 6000 1450 6000
Wire Wire Line
	1550 5800 1400 5800
Wire Wire Line
	1550 5700 1350 5700
Wire Wire Line
	1150 5550 1200 5550
Wire Wire Line
	1200 5550 1200 5450
Wire Wire Line
	1200 5450 1400 5450
Wire Wire Line
	1400 5450 1400 5400
Wire Wire Line
	1400 5400 1550 5400
Text GLabel 1150 5700 0    60   BiDi ~ 0
FSMC_D14
Wire Wire Line
	1550 5500 1250 5500
Wire Wire Line
	1250 5500 1250 5700
Wire Wire Line
	1250 5700 1150 5700
Text GLabel 1150 5850 0    60   BiDi ~ 0
FSMC_D15
Wire Wire Line
	1150 5850 1300 5850
Wire Wire Line
	1300 5850 1300 5600
Wire Wire Line
	1300 5600 1550 5600
Wire Wire Line
	1350 5700 1350 6000
Wire Wire Line
	1350 6000 1150 6000
Wire Wire Line
	1150 6150 1400 6150
Wire Wire Line
	1400 6150 1400 5800
Wire Wire Line
	1150 6300 1450 6300
Wire Wire Line
	1450 6300 1450 6000
Wire Wire Line
	1150 6450 1500 6450
Wire Wire Line
	1500 6450 1500 6100
Text GLabel 9500 2500 0    60   BiDi ~ 0
FSMC_D14
Wire Wire Line
	9500 2500 9600 2500
Wire Wire Line
	9600 2500 9600 2650
Wire Wire Line
	9600 2650 9900 2650
Text GLabel 10650 2350 2    60   BiDi ~ 0
FSMC_D15
Wire Wire Line
	10650 2350 10500 2350
Wire Wire Line
	10500 2350 10500 2550
Wire Wire Line
	10500 2550 10400 2550
$Comp
L nand_programmator:GNDD #PWR036
U 1 1 5E55AAEE
P 9750 2450
F 0 "#PWR036" H 9750 2200 50  0001 C CNN
F 1 "GNDD" H 9750 2300 50  0000 C CNN
F 2 "" H 9750 2450 50  0000 C CNN
F 3 "" H 9750 2450 50  0000 C CNN
	1    9750 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	9750 2450 9900 2450
Wire Wire Line
	9900 2450 9900 2550
$Comp
L nand_programmator:GNDD #PWR037
U 1 1 5E5852F6
P 10450 3750
F 0 "#PWR037" H 10450 3500 50  0001 C CNN
F 1 "GNDD" H 10450 3600 50  0000 C CNN
F 2 "" H 10450 3750 50  0000 C CNN
F 3 "" H 10450 3750 50  0000 C CNN
	1    10450 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	10400 3650 10450 3650
Wire Wire Line
	10450 3650 10450 3750
$Comp
L nand_programmator:+3.3V #PWR035
U 1 1 5E6317BD
P 9650 3150
F 0 "#PWR035" H 9650 3000 50  0001 C CNN
F 1 "+3.3V" H 9650 3290 50  0000 C CNN
F 2 "" H 9650 3150 50  0000 C CNN
F 3 "" H 9650 3150 50  0000 C CNN
	1    9650 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	9650 3150 9650 3250
Wire Wire Line
	9650 3250 9900 3250
Wire Wire Line
	10400 2950 10500 2950
Wire Wire Line
	10500 2950 10500 3050
Connection ~ 10500 3050
Wire Wire Line
	10500 3050 10400 3050
$Comp
L nand_programmator:R R1
U 1 1 5D9B58B5
P 1150 900
F 0 "R1" V 1230 900 50  0000 C CNN
F 1 "510" V 1150 900 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 1080 900 50  0001 C CNN
F 3 "" H 1150 900 50  0000 C CNN
F 4 "2447679" H 1150 900 50  0001 C CNN "Farnell Ref"
	1    1150 900 
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:+3.3V #PWR01
U 1 1 5D9CBDC4
P 1150 650
F 0 "#PWR01" H 1150 500 50  0001 C CNN
F 1 "+3.3V" H 1150 790 50  0000 C CNN
F 2 "" H 1150 650 50  0000 C CNN
F 3 "" H 1150 650 50  0000 C CNN
	1    1150 650 
	1    0    0    -1  
$EndComp
$Comp
L nand_programmator:R R15
U 1 1 5DA670AF
P 850 1400
F 0 "R15" V 930 1400 50  0000 C CNN
F 1 "10k" V 850 1400 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 780 1400 50  0001 C CNN
F 3 "" H 850 1400 50  0000 C CNN
F 4 "2447553" H 850 1400 50  0001 C CNN "Farnell Ref"
	1    850  1400
	0    -1   -1   0   
$EndComp
Wire Wire Line
	650  1400 650  1500
Wire Wire Line
	1150 650  1150 750 
$Comp
L Connector_Generic:Conn_01x02 J5
U 1 1 5DB1A96D
P 750 1200
F 0 "J5" H 670 875 50  0000 C CNN
F 1 "Conn_01x02" H 670 966 50  0000 C CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm" H 750 1200 50  0001 C CNN
F 3 "~" H 750 1200 50  0001 C CNN
	1    750  1200
	-1   0    0    1   
$EndComp
Wire Wire Line
	700  1400 650  1400
Wire Wire Line
	1150 1100 950  1100
Wire Wire Line
	950  1200 1150 1200
Wire Wire Line
	1150 1050 1150 1100
Wire Wire Line
	1000 1400 1150 1400
Wire Wire Line
	1150 1200 1150 1400
Connection ~ 1150 1400
Wire Wire Line
	1150 1400 1550 1400
Wire Wire Line
	600  1900 600  2200
$Comp
L Connector:USB_B_Micro P1
U 1 1 588508F5
P 9150 5850
F 0 "P1" H 8950 6200 50  0000 C CNN
F 1 "USB" H 9150 6200 50  0000 C CNN
F 2 "Connector_USB:USB_Micro-B_Molex-105017-0001" V 9100 5750 50  0001 C CNN
F 3 "" V 9100 5750 50  0000 C CNN
F 4 "2293836" H 9150 5850 50  0001 C CNN "Farnell Ref"
	1    9150 5850
	-1   0    0    -1  
$EndComp
Wire Wire Line
	9150 6250 9250 6250
Wire Wire Line
	8850 5650 8750 5650
Wire Wire Line
	8750 5650 8750 5600
Connection ~ 9150 6250
Wire Wire Line
	8850 5850 8400 5850
Connection ~ 8400 5850
Wire Wire Line
	8550 5650 8550 5950
Wire Wire Line
	8550 5950 8850 5950
NoConn ~ 8850 6050
$EndSCHEMATC
