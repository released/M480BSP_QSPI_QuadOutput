# M480BSP_QSPI_QuadOutput
 M480BSP_QSPI_QuadOutput


update @ 2022/11/01

1. add define : ENABLE_QSPI_MANUAL_SS , ENALBE_PDMA_IRQ , ENALBE_PDMA_POLLING , ENALBE_QSPI_REGULAR_TRX

2. below is CS pin / clock difference between these define , 

when QSPI speed is 800K , use auto SS function , with define : ENALBE_QSPI_REGULAR_TRX
![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/800K_ENALBE_QSPI_REGULAR_TRX_clock.jpg)	


when QSPI speed is 800K , use manual SS function , with define : ENALBE_PDMA_IRQ
![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/800K_ENALBE_PDMA_IRQ_ManualSS.jpg)	


when QSPI speed is 800K , use auto SS function , with define : ENALBE_PDMA_IRQ
![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/800K_ENALBE_PDMA_IRQ_AutoSS.jpg)	


when QSPI speed is 24MHz , use auto SS function , with define : ENALBE_QSPI_REGULAR_TRX
![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/24MHz_ENALBE_QSPI_REGULAR_TRX_clock.jpg)	

timing between 2 SS low , 
![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/24MHz_ENALBE_QSPI_REGULAR_TRX_SS.jpg)	


when QSPI speed is 96MHz , use auto SS function , with define : ENALBE_QSPI_REGULAR_TRX
![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/96MHz_ENALBE_QSPI_REGULAR_TRX_clock.jpg)	

timing between 2 SS low , 
![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/96MHz_ENALBE_QSPI_REGULAR_TRX_SS.jpg)	



update @ 2022/10/24

1. initial QSPI0 TX , RX with PDMA IRQ

	- SS : PH9

	- CLK : PA2

	- MOSI0 : PC0

	- MOSI1 : PH11

	- MISO0 : PE1

	- MISO1 : PA5

2. Below is LA capture 

![image](https://github.com/released/M480BSP_QSPI_QuadOutput/blob/main/LA_capture.jpg)	


