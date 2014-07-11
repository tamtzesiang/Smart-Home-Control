/*
 * project2Bfunctions.h
 *
 *  Created on: 02-Nov-2011
 *      Author: aLwYn
 */

#ifndef PROJECT2BFUNCTIONS_H_
#define PROJECT2BFUNCTIONS_H_

void burglarAnimation(void)
{
	uint8_t i = 0;
	uint8_t j = 0;

	oled_clearScreen(OLED_COLOR_BLACK);				//clearing screen
	oled_circle(48,26,26,OLED_COLOR_WHITE);			//head
	oled_line(27,10,68,10,OLED_COLOR_WHITE);		//top eye line
	oled_line(23,29,73,29,OLED_COLOR_WHITE);		//bottom eye line
	oled_circle(24,63,17,OLED_COLOR_WHITE);			//left shoulder
	oled_circle(72,63,17,OLED_COLOR_WHITE);			//right shoulder
	oled_fillRect(35,52,60,64,OLED_COLOR_BLACK);	//clearing the inner curve
	oled_fillRect(17,54,78,56,OLED_COLOR_WHITE);	//top shirt line
	oled_fillRect(12,61,82,64,OLED_COLOR_WHITE);	//bottom shirt line

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
		pca9532_setLeds((37449<<i),(28086<<i));			//65,535 = 0xFFFF
		oled_circle(35,20,(8-i),OLED_COLOR_BLACK);	//clearing previous left eye
		oled_circle(60,20,(8-i),OLED_COLOR_BLACK);	//clearing previous right eye

		oled_circle(35,20,(7-i),OLED_COLOR_WHITE);	//make left eye go smaller
		oled_circle(60,20,(7-i),OLED_COLOR_WHITE);	//make right eye go smaller
		oled_line((44-i),40,(52+i),40,OLED_COLOR_WHITE);	//make mouth go bigger
		timeDelay(150);
	}
	for (j=0; j<7;j++)
	{
		pca9532_setLeds(0,0xFFFF);						//clear 16 led
		switch (j)
		{
			case 1:	GPIO_ClearValue( 2, (1<<0) );
				break;
			case 2:
				break;
			case 3:	pca9532_setLeds(0x0001,0);
				break;
			case 4:	pca9532_setLeds(0x0002,0x0001);
					GPIO_SetValue( 2, (1<<0) );
				break;
			case 5:	pca9532_setLeds(0x0004,0x0002);
				break;
			case 6:	pca9532_setLeds(0x0009,0x0003);
					GPIO_ClearValue( 2, (1<<0) );
				break;
		}
		pca9532_setLeds((37449<<j),(28086<<j));			//65,535 = 0xFFFF
		oled_circle(35,20,(0+j),OLED_COLOR_BLACK);	//clearing previous left eye
		oled_circle(60,20,(0+j),OLED_COLOR_BLACK);	//clearing previous right eye
		oled_line((37+j),40,(59-j),40,OLED_COLOR_BLACK);	//clearing the previous mouth

		oled_circle(35,20,(1+j),OLED_COLOR_WHITE);	//make left eye go bigger
		oled_circle(60,20,(1+j),OLED_COLOR_WHITE);	//make right eye go bigger
		oled_line((38+j),40,(58-j),40,OLED_COLOR_WHITE);	//make mouth go smaller
		timeDelay(150);
	}
}
						/*	oled_putString(15,44,&fireAlarm,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
							mode = console_fire_alarm_countdown;
							int ascii = 57;
							while ((ascii >= 48)	&& (mode == console_fire_alarm_countdown))	//while count down mode
							{
								 led7seg_setChar(ascii, FALSE);
								 //timeDelay(1000);
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
								GPIO_SetValue(2,(1<<0));	//turn on the RGB RED LED
								if (mode == console_fire_alarm_ringing)		//to minimise remaining alarm tone being played when it should be terminated
								{
									playSong(burglarAlarmTone);
								}
								burglarAnimation();
								pca9532_setLeds(0,0xFFFF);	//turning off LED 4 to 19 or OFF ALL Lights
								GPIO_ClearValue(2,1);		//turn off the RGB RED LED
								timeDelay(500);

								oled_clearScreen(OLED_COLOR_BLACK);
								//"2nd Cycle" of the Intruder part
								pca9532_setLeds(0xFFFF,0);	//turning on LED 4 to 19 or ON ALL Lights
								GPIO_SetValue(2,1);			//turn on the RGB RED LED
								sprintf(storePrint,"INTRUDER !!");
								oled_putString(20,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
								playSong(burglarAlarmTone);

								oled_clearScreen(OLED_COLOR_BLACK);
								pca9532_setLeds(0,0xFFFF);		//turning off LED 4 to 19 or OFF ALL Lights
								GPIO_ClearValue(2,(1<<0));		//turn off the RGB RED LED
								timeDelay(500);
								oled_putString(20,33,&storePrint,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
							}*/


/*
	int *interruptSetEnableRegister = 0xE000E100;
	*interruptSetEnableRegister = (1<<8);			//set enable UART3 interrupt
	int *interruptClearPendingRegister = 0xE000E280;
	*interruptClearPendingRegister = (1<<8);		//clear UART3 interrupt pending
	LPC_GPIOINT->IO0IntEnF |= 1<<1;		//Allow UART3 Receiver Interrupt to Occur
	NVIC_EnableIRQ(UART3_IRQn);			//Enable EINT3 Interrupt Handler
*/
/*
void UART3_IRQHandler(void)
{
		uint8_t singleInput = 0;
		uint32_t length = 0;
		uint8_t message[64];
		int *disableUART3interrupt = 0xE000E180;

	printf("in UART3 interrupt handler !! WOOHOOOO~");
	UART_Receive(LPC_UART3, &singleInput, 1, BLOCKING);	//LPC Receiving from the terminal, ie need handshaking
	if (singleInput != '\r')
	{
		length++;
		message[length-1] = singleInput;
	}
	else	//enter is being pressed
	{
		message[length]=0;									//string terminator of ASCII value 0, to terminate the data, such that it becomes a string with \0 by adding the 0 behind
		sprintf(storePrint,"%s\r\n", message);				//storing the print in an array
		UART_SendString(LPC_UART3, &storePrint);			//Sending to the PC console tera terminal for display
		printf("--%s--\n", message);
		length = 0;
	}
	if(strcmp(message,"alert") == 0)
	{
		mode = monitoring;
	}
	*disableUART3interrupt = 1;				//clear the status flag
	NVIC_ClearPendingIRQ(UART3_IRQn);		//clear the pending status
	LPC_GPIOINT->IO0IntClr = 1<<1;			//end off the interrupt
}
*/

#endif /* PROJECT2BFUNCTIONS_H_ */
