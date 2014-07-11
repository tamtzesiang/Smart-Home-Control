//Project 2B

//Lib_MCU header files
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"
//library files for BaseBoard peripherals
#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
//#include "rgb.h"  --> this has some errors and so we better don't use it

//library files included for Demo
#include "led7seg.h"
#include "project2ADemo.h"

//library files included for Project 2
#include "LPC17xx.h"//CMSIS headers required for setting up SysTick Timer (And Other Interrupts)
#include "stdio.h"
#include "temp.h"
#include "light.h"
#include "project2Bfunctions.h"
#include "lpc17xx_uart.h"		//library function required for the UART
#include "string.h"				//library function to perform string function calls

//Defining light states
#define dark 1
#define normal 2
#define too_bright 3
//Defining Mode, ie Monitoring/Countdown/Ringing
#define monitoring 0
#define fire_alarm_countdown 1
#define fire_alarm_ringing 2
#define console 3
#define console_fire_alarm_countdown 4
#define console_fire_alarm_ringing 5
#define burglarIntrusionAlarm 6

	//Global variable declaration
	uint8_t current_status = 2;				//assuming that the initial Light status is "normal lighting"
	uint8_t mode = 0;						//assuming that the initial Mode is at "monitoring mode"
	int8_t armTheBurglarDetection = -1;		//assuming that the initial Burglar Detection hasn't been armed, hence the negative value to denote Dis-armed
	volatile uint32_t *EINT0statusFlag = 0x400FC140;	//defining a pointer that points to the EINT0 interrupt status flag register
	static uint32_t msTicks = 0;
	static uint8_t *fireAlarmTone = (uint8_t*)"b1.b1,";
	static uint8_t *burglarAlarmTone = (uint8_t*)"C1.b3,";

	//String declaration for OLED
	unsigned char lightOn[10] = "LIGHT ON ";
	unsigned char lightOff[10] = "LIGHT OFF";
	unsigned char airCon[8] = "AIR-CON";
	unsigned char heater[8] = "HEATER ";
	unsigned char fanOn[8] = "FAN ON ";
	unsigned char fireAlarm[11] = "FIRE ALARM";
	unsigned char consoleEnable[15] = "CONSOLE ENABLE";
	unsigned char storePrint[96];//temporary storing array
	static char *commandList[] =
	{
		"led on",
		"led off",
		"temp",
		"light",
		"alarm",
		"alert"
	};

	//SysTick timer function called every 1ms
	void SysTick_Handler(void)
	{msTicks++;}
	//Retrieving Current Ticks when called by temp_init()
	static uint32_t getTicks(void)
	{return msTicks;}

	//Configuring Temperature Sensor's Pin
	void initializeTemp(void)
	{
		PINSEL_CFG_Type PinCfg;
		//Initialize Temperature Sensor GPIO pin connect
		PinCfg.Funcnum = 0;
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Portnum = 0;
		PinCfg.Pinnum = 2;
		PINSEL_ConfigPin(&PinCfg);
		GPIO_SetDir(0,(1<<2),0);	//Setting it as an Input reader
	}

	//Configuring Light Sensor's Pin
	void initializeLight(void)
	{
		PINSEL_CFG_Type PinCfg;
		//Initialize Light Sensor GPIO pin connect
		PinCfg.Funcnum = 0;
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Portnum = 2;
		PinCfg.Pinnum = 5;
		PINSEL_ConfigPin(&PinCfg);
		GPIO_SetDir(2,(1<<5),0);
	}

	//Configuring SW3's Pin
	void initializeSW3(void)
	{
		PINSEL_CFG_Type PinCfg;
		//Initialize SW3's pin connect
		PinCfg.Funcnum = 1;			//Setting it as an EINT0 function
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Portnum = 2;
		PinCfg.Pinnum = 10;
		PINSEL_ConfigPin(&PinCfg);
		GPIO_SetDir(2,(1<<10),0);	//Setting it as an Input reader
	}
	//SW4's pin is already configured in the "header file of project2ADemo.h"

	//Configuring UART's Pin
	void pinselUART3(void)
	{
		PINSEL_CFG_Type PinCfg;
		PinCfg.Funcnum = 2;
		PinCfg.Pinnum = 0;
		PinCfg.Portnum = 0;
		PINSEL_ConfigPin(&PinCfg);
		PinCfg.Pinnum = 1;
		PINSEL_ConfigPin(&PinCfg);
	}

	//Configuring Initialization of UART
	void initializeUART(void)
	{
		UART_CFG_Type uartCfg;
		uartCfg.Baud_rate = 115200;
		uartCfg.Databits = UART_DATABIT_8;
		uartCfg.Parity = UART_PARITY_NONE;
		uartCfg.Stopbits = UART_STOPBIT_1;
		//Doing the Pin Select for UART3;
		pinselUART3();
		//Supply Power & Setup Working par.s for UART3
		UART_Init(LPC_UART3, &uartCfg);
		//Enable Transmit for UART3
		UART_TxCmd(LPC_UART3, ENABLE);
	}

	//Interrupt Handler Routine
	void EINT3_IRQHandler(void)
	{
		if ((LPC_GPIOINT->IO2IntStatF>>5)& 0x1)		//either this : check the falling edge of the light's GPIO pin interrupt
		//if ((light_getIrqStatus())& 0x1)			//or this: check it interrupt status flag
		{
			lightStatus();
			newRange();
		}
		light_clearIrqStatus();						//clear the status flag
		LPC_GPIOINT->IO2IntClr = 1<<5;				//end off the interrupt
		NVIC_ClearPendingIRQ(EINT3_IRQn);			//clear the pending status
	}

	void EINT0_IRQHandler(Void)
	{
		//if((LPC_GPIOINT->IO2IntStatF>>10)& 0x1)    --> we do not need to check this since it is always due to EINT0 interrupt, our SW3's pin that calls this handler
		printf("EINT0 interrupt occurs due to Pin 2.10!!\n");
		getMode();
		printf("mode is %d\n", mode);
		*EINT0statusFlag = 1;					//clear the status flag
		LPC_GPIOINT->IO2IntClr = 1<<10;			//end off the interrupt
		NVIC_ClearPendingIRQ(EINT0_IRQn);		//clear the pending status
	}

	//Creating a "Change Mode" function when SW3 is pressed, ie EINT0 interrupt triggered
	void getMode(void)
	{
		if(mode == monitoring)
		{
			mode = monitoring;					//handler performs the job of staying in "monitoring mode"
		}
		else if((mode == fire_alarm_countdown) || (mode == fire_alarm_ringing))
		{
			oled_clearScreen(OLED_COLOR_BLACK);	//clearing the "fire alarm" displayed on the OLED
			mode = monitoring;					//handler performs the job of going back to "monitoring mode" for the stated conditions
			int ascii = 57;
			led7seg_setChar(ascii, FALSE);

		}
		else if((mode == console))
		{
			mode = console;						//handler performs the job of staying in "console mode"
		}
		else if((mode == console_fire_alarm_countdown) || (mode == console_fire_alarm_ringing))
		{
			oled_clearScreen(OLED_COLOR_BLACK);	//clearing the "console fire alarm" displayed on the OLED
			oled_putString(5,33,&consoleEnable,OLED_COLOR_WHITE,OLED_COLOR_BLACK);	//displaying back the "console enable" on the OLED
			mode = console;						//handler performs the job of going back to "console mode" for the stated conditions
			int ascii = 57;
            led7seg_setChar(ascii, FALSE);

		}
		else if (mode == burglarIntrusionAlarm)
		{
			oled_clearScreen(OLED_COLOR_BLACK);
			armTheBurglarDetection = -1;			//disarming the burglar detection
			mode = monitoring;						//returning to monitoring mode
		}
		else;
	}

	//Creating a time delay function
	void timeDelay(uint32_t ticks)
	{
		uint32_t currentTime = 0;
		currentTime = msTicks;
		while(msTicks - currentTime < ticks)
		{}
	}

	//Setting the current light status
	void lightStatus(void)
	{
		if (light_read() < 100)
		{
			current_status = dark;
		}
		else if ((light_read()>= 100) || (light_read()<= 1000))
		{
			current_status = normal;
		}
		else
		{
			current_status = too_bright;
		}
	}

	//Setting the new threshold for each specific state
	void newRange(void)
	{
		if (current_status==1)//dark
		{
			light_setHiThreshold(100);
			light_setLoThreshold(0);
		}

		else if(current_status==2)//normal
		{
			light_setHiThreshold(1000);
			light_setLoThreshold(100);
		}

		else if(current_status==3)//too bright
		{
			light_setHiThreshold(4000);
			light_setLoThreshold(1000);
		}
	}

	 //Turning On AirCon/Heater/Fan
	 void turnOnAirCon(void)
	 {	pca9532_setLeds(0x0002, 0x0005);}	//turning On LED 5 and turning off LED 4&6
	 void turnOnHeater(void)
	 {	pca9532_setLeds(0x0001, 0x0006);}	//turning On LED 4 and turning off LED5&6
	 void turnOnFan(void)
	 {	pca9532_setLeds(0x0004, 0x0003);}	//turning On LED 6 and turning off LED 4&5



int main (void) {

    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;

    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;

    uint8_t loopCounter = 0;		//counter to keep track of the polling loop
    uint8_t boardHasBeenMoved = 0;	//assuming that the initial board has not been moved

    int8_t xCurrent = 0;
    int8_t yCurrent = 0;
    int8_t zCurrent = 0;

    int8_t xPrevious = 0;
    int8_t yPrevious = 0;
    int8_t zPrevious = 0;

    //uint8_t dir = 1;
    //uint8_t wait = 0;

	uint8_t joyState = 0;
    //uint8_t btn1 = 1;				//Button 1 is now our SW4

	led7seg_init();
    init_i2c();
    init_ssp();
    init_GPIO();					//Initialize SW4's pin

    pca9532_init();
    joystick_init();
    acc_init();
    oled_init();

    /*
     * Assume base board in zero-g position when reading first value.
     */
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 64-z;

    /* ---- Speaker ------> */

	//GPIO_SetDir(2, 1<<0, 1);		//Same as RGB RED LED
    //GPIO_SetDir(2, 1<<1, 1);		//Same as RGB GREEN LED

    GPIO_SetDir(0, 1<<27, 1);
    GPIO_SetDir(0, 1<<28, 1);
    GPIO_SetDir(2, 1<<13, 1);
    GPIO_SetDir(0, 1<<26, 1);

    GPIO_ClearValue(0, 1<<27); //LM4811-clk
    GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
    GPIO_ClearValue(2, 1<<13); //LM4811-shutdn

    /* <---- Speaker ------ */

    //moveBar(1, dir);				//not moving the bar of LED on the pca9532 already
    oled_clearScreen(OLED_COLOR_BLACK);

    /***************************************/
    /***************************************/

    //-->My Sensor's Initialization<--

    //Temperature Sensor
    SysTick_Config(SystemCoreClock / 1000);		//to allow milliseconds ticks
    initializeTemp();
    temp_init(&getTicks);
    double temp = 0;							//using a float to obtain decimal point

    //Light Sensor
    uint32_t light = 0;
    initializeLight();
    light_enable();
    LPC_GPIOINT->IO2IntEnF |= 1<<5;		//Allow Light Sensor Interrupt to Occur
    NVIC_EnableIRQ(EINT3_IRQn);			//Enable EINT3 Interrupt Handler
    light_setRange(LIGHT_RANGE_4000);
    light_setLoThreshold(100);
    light_setHiThreshold(1000);
    light_clearIrqStatus();

    //Setting up the interrupt for SW3
    initializeSW3();
    LPC_GPIOINT->IO2IntEnF |= 1<<10;	//Allow the SW3 Interrupt to Occur
    NVIC_EnableIRQ(EINT0_IRQn);			//Enable EINT0 Interrupt Handler
    //NVIC_SetPriorityGrouping(uint32_t PriorityGroup)		???????**need**???????   //good practice to have though...says alwyn
    NVIC_SetPriority(EINT0_IRQn,1);
    NVIC_SetPriority(EINT3_IRQn,2);

    //RGB Red LED initialization
    GPIO_SetDir(2,(1<<0),1);			//RGB RED LED is Port 2 Pin 0

    //SW4 variable initialization
    uint8_t SW4 = 1;					//Setting SW4 as high, ie off mode since it is active low

    //UART3 initialization
    initializeUART();

    //Display Tera Terminal Status
    sprintf(storePrint,"CONSOLE DISABLE\r\n");		//storing the print in an array
    UART_SendString(LPC_UART3, &storePrint);		//Sending to the PC console tera terminal to display 'Console Disable'

    int ascii = 57;
    led7seg_setChar(ascii, FALSE);

 while (1)
 {
	 //Light Sensor
	 light = light_read();
	 printf("Light is %d lux\n", light);
	 sprintf(storePrint,"Light = %d lux", light);					//storing the print in an array
	 oled_putString(0,10,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);	//OLED display reading
	 if(current_status == 1)//dark
	 {
		 pca9532_setLeds(0xFFF8,0);	//turning on LED 7 to 19 or ON Lights
		 oled_putString(20,44,&lightOn,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	 }
	 else if(current_status == 2 || current_status == 3)//normal or too bright
	 {
		 pca9532_setLeds(0,0xFFF8);	//turning off LED 7 to 19 or OFF Lights
		 oled_putString(20,44,&lightOff,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	 }

	 //Temperature Polling
	 temp = temp_read() * 0.1;	//diving by 10 since the read function returns 10 times the required value
	 sprintf(storePrint,"Temp = %0.1f deg", temp);	//storing the print in an array
	 oled_putString(0,20,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);	//OLED display reading
	 if (temp > 26)//hot
	 {
		 printf("the Hot temperature is %0.1f Degrees\n",temp);
		 oled_putString(25,33,&airCon,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
		 turnOnAirCon();		//ON LED 5
	 }
	 else if (temp < 20)//cold
	 {
		 printf("the Cold temperature is %0.1f Degrees\n",temp);
		 oled_putString(25,33,&heater,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
		 turnOnHeater();		//ON LED 4
	 }
	 else //temperature between 20 to 26 degree celsius
	 {
		 printf("the Normal temperature is %0.1f Degrees\n",temp);
		 oled_putString(25,33,&fanOn,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
		 turnOnFan();			//ON LED 6
	 }

	 //Checking TOO BRIGHT and TOO HOT ie fire!
	 if ((temp > 26) && (light > 1000))
	 {
		 oled_clearScreen(OLED_COLOR_BLACK);
		 oled_putString(20,33,&fireAlarm,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
		 mode = fire_alarm_countdown;		//Setting to "Fire Alarm" count down Mode
		 int ascii = 57;	//meaning the number 9

		 while ((ascii >= 48) && (mode == fire_alarm_countdown))	//while count down mode
		 {
			 led7seg_setChar(ascii, FALSE);
			 timeDelay(1000);	//1000ms = 1sec, thus we implement 1 sec interval in this loop
			 ascii--;
			 if (ascii == 47)	//meaning the number 0
			 {
				 mode = fire_alarm_ringing;		//Setting to "Fire Alarm" ringing Mode
			 }
		 }
		 while ((ascii == 47) && (mode == fire_alarm_ringing))		//while Ringing mode
		 {
			 led7seg_setChar('0', FALSE);
			 pca9532_setLeds(0xFFFF,0);	//turning on LED 4 to 19 or ON ALL Lights
			 GPIO_SetValue(2,1);		//turn on the RGB RED LED
			 if (mode == fire_alarm_ringing)	//Ensuring that when SW3 or EINT0 Handler is called, the programme which resumes here from the stack doesn't continue to have the unwanted final beep sound, by changing the state
			 {							//However, when the programme is interrupt during the beep itself, the remaining beep sound still cannot be eliminated as it returns to play the final beep before exiting this if condition
				playSong(fireAlarmTone);	//by playing the alarm tone, we have actually created a delay as well during the LED ON part
			 }							//Hence, the 'if' condition is to minimise remaining alarm tone being played when it should be terminated
			 pca9532_setLeds(0,0xFFFF);	//turning off LED 4 to 19 or OFF ALL Lights
			 GPIO_ClearValue(2,1);		//turn off the RGB RED LED
			 timeDelay(500);			//Delaying for 0.5sec for the LED OFF part, so that the previous delay + this 0.5sec delay gives a total of 1sec delay for the blinking cycle
		 }
	 }

	 //Checking if SW4 is activated to go into Console Mode
	 SW4 = (GPIO_ReadValue(1) >> 31) & 0x01;
	 if (SW4 == 0)
	 {
		 printf("SW4 is activated!! Entering Console Mode!\n");
		 oled_clearScreen(OLED_COLOR_BLACK);
		 oled_putString(5,33,&consoleEnable,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
		 sprintf(storePrint,"CONSOLE ENABLE\r\n");		//storing the print in an array
		 UART_SendString(LPC_UART3, &storePrint);		//Sending to the PC console tera terminal to display 'Console Enable'
		 mode = console;

		 uint8_t singleInput = 0;
		 uint32_t length = 0;
		 uint8_t message[64];
		 uint8_t counter = 0;
		 uint8_t index = 0;

		 //Receive Input message without knowing the incoming message's length
		 while (mode == console)
		 {
			 length = 0;	//resetting the length count, so that the array to store the message can restart at the zero position
			 do
			 {
				 UART_Receive(LPC_UART3, &singleInput, 1, BLOCKING);	//LPC Receiving from the terminal, ie need handshaking
				 UART_Send(LPC_UART3, &singleInput, 1, BLOCKING);		//LPC Sending single input to the terminal to display character being typed
				 if (singleInput != '\r')
				 {
					 length++;
					 message[length-1] = singleInput;
				 }
			 } while ((length<64) && (singleInput != '\r'));	//while data length is less than 64 and "Enter" hasn't been pressed yet, continue to ask for input
			 message[length]=0;									//string terminator of ASCII value 0, to terminate the data, such that it becomes a string with \0 by adding the 0 behind
			 sprintf(storePrint,"%s\r\n", message);				//storing the print in an array
			 UART_SendString(LPC_UART3, &storePrint);			//Sending to the PC console tera terminal for display
			 printf("--%s--\n", message);
			 for(counter = 0; counter <6; counter++)
			 {
				 if(strcmp(message, commandList[counter]) == 0)
				 {
					 printf("the message is '%s' !!\n", message);
					 index = counter + 1;
					 printf("index value is %d !!\n", index);
					 break;
				 }
				 else
				 {
					 printf("the Command Ignored is '%s' !!\n", commandList[counter]);
					 index = 0;
					 printf("index value is %d !!\n", index);
				 }
			 }
			 switch (index)
			 {
				 case 1:	GPIO_SetValue(2,1);		//turn on the RGB RED LED
				            sprintf(storePrint,"> The RGB Red LED is turned ON\r\n");
				 			UART_SendString(LPC_UART3, &storePrint);
					 break;
				 case 2:	GPIO_ClearValue(2,1);	//turn off the RGB RED LED
				            sprintf(storePrint,"> The RGB Red LED is turned OFF\r\n");
				 		    UART_SendString(LPC_UART3, &storePrint);
					 break;
				 case 3:	temp = temp_read() * 0.1;
							sprintf(storePrint,"> Current Temperature is %0.1f deg\r\n", temp);	//storing the print in an array
							UART_SendString(LPC_UART3, &storePrint);			//Sending the print array to the PC console tera terminal
					 break;
				 case 4:	light = light_read();
							sprintf(storePrint,"> Current Light reading is %d lux\r\n", light);	//storing the print in an array
							UART_SendString(LPC_UART3, &storePrint);			//Sending the print array to the PC console tera terminal
					 break;
				 case 5:	oled_putString(15,44,&fireAlarm,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
							mode = console_fire_alarm_countdown;

							sprintf(storePrint,"> Your alarm is counting down and will rings in 10seconds. Turn it Off by pressing SW3\r\n");
							UART_SendString(LPC_UART3, &storePrint);

							int ascii = 57;
							while ((ascii >= 48)	&& (mode == console_fire_alarm_countdown))	//while count down mode
							{
								 led7seg_setChar(ascii, FALSE);
								 timeDelay(1000);
								 ascii--;
								 if (ascii == 47)
								 {
									 mode = console_fire_alarm_ringing;	//Setting to "Fire Alarm" ringing Mode
								 }
							}
							while ((ascii == 47) && (mode == console_fire_alarm_ringing))		//while Ringing mode
							{
								led7seg_setChar('0', FALSE);
								pca9532_setLeds(0xFFFF,0);	//turning on LED 4 to 19 or ON ALL Lights
								GPIO_SetValue(2,1);			//turn on the RGB RED LED
								if (mode == console_fire_alarm_ringing)		//to minimise remaining alarm tone being played when it should be terminated
								{
									playSong(fireAlarmTone);
								}
								pca9532_setLeds(0,0xFFFF);	//turning off LED 4 to 19 or OFF ALL Lights
								GPIO_ClearValue(2,1);		//turn off the RGB RED LED
								timeDelay(500);
							}
					 break;
				 case 6:	mode = monitoring;
							oled_clearScreen(OLED_COLOR_BLACK);				//clearing the OLED's 'Console Enable'
							//Display Tera Terminal 'Console Disable'
							sprintf(storePrint,"CONSOLE DISABLE\r\n");		//storing the print in an array
							UART_SendString(LPC_UART3, &storePrint);		//Sending to the PC console tera terminal to display 'Console Disable'
					 break;
				 default: //for index = 0 meaning no matched commands, we do nothing
					      sprintf(storePrint,">'No such command, please re-enter your command'\r\n");
					 	  UART_SendString(LPC_UART3, &storePrint);
					 break;
			 }
		 }
	 }

		//--> Burglar Detection and its Functionalities <--

        /* ####### Accelerometer and LEDs  ###### */
        /* # */

		loopCounter++;				//keeping track of the polling loop

		if (armTheBurglarDetection == 1)
		{
			sprintf(storePrint,"( ARMED )");
			oled_putString(19,55,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
		}
		else//(armTheBurglarDetection == -1);
		{
	    	sprintf(storePrint,"( DIS-ARMED )");
			oled_putString(7,55,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
		}

		acc_read(&x, &y, &z);
        x = x+xoff;
        y = y+yoff;
        z = z+zoff;

        xCurrent = x;
        yCurrent = y;
        zCurrent = z;
        printf("xCurrent is %d , yCurrent is %d , zCurrent is %d \n", xCurrent, yCurrent, zCurrent);

        if (loopCounter == 1)		//meaning the 1st polling loop before board has been moved status is being updated
		{
			//Since the 1st loop doesn't have the Previous value, the very 1st value is the reference value for the next loop
			xPrevious = xCurrent;
			yPrevious = yCurrent;
			zPrevious = zCurrent;
			printf("xPrevious is %d , yPrevious is %d , zPrevious is %d IN THE FIRST LOOP!!!\n", xPrevious, yPrevious, zPrevious);
		}

        boardHasBeenMoved = 0;
        //Checking to see if the board has been moved, using 'x' AND 'y' AND 'z' to confirm shifted, to allow some buffer when the board is accidentally tilted slightly
        if ( ((xCurrent - xPrevious > 10) || (xPrevious - xCurrent > 10)) && ((yCurrent - yPrevious > 10) || (yPrevious - yCurrent > 10)) && ((zCurrent - zPrevious > 10) || (zPrevious - zCurrent > 10)) )
        {
        	printf("the board is experiencing Burglar Intrusion's movement that MOVED THE BOARD !!!\n");
        	oled_clearScreen(OLED_COLOR_BLACK);
        	boardHasBeenMoved = 1;
        	loopCounter = 0;	//so that when we reset we won't re-enter this if condition, since the reset didn't set the previous value for comparison, so that it won't detect shifted right after reset is being pressed
        	if (armTheBurglarDetection == 1)
        	{
            	sprintf(storePrint,"DOORS/WINDOWS");
            	oled_putString(7,22,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
            	sprintf(storePrint,"FORCED OPEN !!");
            	oled_putString(8,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
            	timeDelay(2000);
            	oled_clearScreen(OLED_COLOR_BLACK);
        	}
        	else//(armTheBurglarDetection == -1);
        	{
            	sprintf(storePrint,"BOARD HAS");
            	oled_putString(15,22,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
            	sprintf(storePrint,"BEEN MOVED !!");
            	oled_putString(8,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
            	timeDelay(2000);
            	oled_clearScreen(OLED_COLOR_BLACK);
        	}

        }
    	//Updating the Previous Accelerometer for comparison
        xPrevious = xCurrent;
    	yPrevious = yCurrent;
    	zPrevious = zCurrent;
    	//printf("xPrevious is %d , yPrevious is %d , zPrevious is %d \n", xPrevious, yPrevious, zPrevious);			//same print output as the printing of current value's


        /* ############################################# */


        /* ####### Joystick and OLED  ###### */
        /* # */

    	joyState = joystick_read();
        if (joyState != 0)
        {
        	if ((joyState & JOYSTICK_CENTER) != 0)
        	{
        		oled_clearScreen(OLED_COLOR_BLACK);
        		armTheBurglarDetection = armTheBurglarDetection * -1;
        		printf("the Burglar Detection's 'Arm' state is '%d' !!!\n", armTheBurglarDetection);
        		if (armTheBurglarDetection == 1)
        		{
        			sprintf(storePrint,"BURGLAR");
        			oled_putString(25,22,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        			sprintf(storePrint,"SYSTEM IS");
					oled_putString(20,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
					sprintf(storePrint,"ARMED !!");
					oled_putString(25,44,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        		}
        		else//(armTheBurglarDetection == -1);
        		{
        			sprintf(storePrint,"BURGLAR");
        			oled_putString(25,22,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        			sprintf(storePrint,"SYSTEM IS");
        			oled_putString(20,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        			sprintf(storePrint,"DIS-ARMED !!");
        			oled_putString(15,44,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        		}
				timeDelay(1500);
				oled_clearScreen(OLED_COLOR_BLACK);
        	}
        }
            //drawOled(joyState);

        //Checking whether the Burglar has intruded into the home and blocked the Light Sensor, thus causing the Light reading to be low also
        if ( (light < 100) && (boardHasBeenMoved == 1) && (armTheBurglarDetection == 1) )
        {

        	printf("Burglar Conditions have been met!! Entering BURGULAR INTRUSION MODE !!! \n");
        	oled_clearScreen(OLED_COLOR_BLACK);
     		sprintf(storePrint,"BURGLAR");
     		oled_putString(25,22,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
     		sprintf(storePrint,"INTRUSION MODE !!");
        	oled_putString(5,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        	uint8_t i = 0;
        	for (i=0; i<7; i++)
        	{
        		pca9532_setLeds(0,0xFFFF);						//clear 16 led
        		switch (i)
        		{
        			case 1:	GPIO_ClearValue( 2, (1<<0) );
        				break;
        			case 2:	//
        				break;
        			case 3:	pca9532_setLeds(0x0001,0);
        				break;
        			case 4:	pca9532_setLeds(0x0002,0x0001);
        					GPIO_SetValue( 2, (1<<0) );
        				break;
        			case 5:	pca9532_setLeds(0x0004,0x0002);
        				break;
        			case 6:	pca9532_setLeds(0x0009,0x0003);
        				break;
        		}
        		pca9532_setLeds((37449<<i),(28086<<i));
        		timeDelay(400);
        	}
     		oled_clearScreen(OLED_COLOR_BLACK);
           	mode = burglarIntrusionAlarm;

        	while (mode == burglarIntrusionAlarm)
        	{
        		sprintf(storePrint,"INTRUDER !!");
        		oled_putString(20,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        		pca9532_setLeds(0xFFFF,0);	//turning on LED 4 to 19 or ON ALL Lights
        		GPIO_SetValue(2,(1<<0));	//turn on the RGB RED LED
        		playSong(burglarAlarmTone);
        		burglarAnimation();			//Display the animation for burglar alarm
        		pca9532_setLeds(0,0xFFFF);	//turning off LED 4 to 19 or OFF ALL Lights
        		GPIO_ClearValue(2,1);		//turn off the RGB RED LED
        		timeDelay(500);
        		oled_clearScreen(OLED_COLOR_BLACK);

        		//The "2nd CYCLE" of the Intruder part
        		pca9532_setLeds(0xFFFF,0);	//turning on LED 4 to 19 or ON ALL Lights
        		GPIO_SetValue(2,1);			//turn on the RGB RED LED
        		oled_putString(20,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
        		playSong(burglarAlarmTone);
        		oled_clearScreen(OLED_COLOR_BLACK);
        		pca9532_setLeds(0,0xFFFF);		//turning off LED 4 to 19 or OFF ALL Lights
        		GPIO_ClearValue(2,(1<<0));		//turn off the RGB RED LED
        		timeDelay(500);
        	}
        }

        /* # */
        /* ############################################# */

 //       btn1 = (GPIO_ReadValue(1) >> 31) & 0x01;

        /* ############ Trimpot and RGB LED  ########### */
        /* # */


/*        if (btn1 == 0)
        {
        	led7seg_setChar('5', FALSE);
        	playSong(song);
        }
        led7seg_setChar('7', FALSE);

        Timer0_Wait(1);


*/
    }
}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
