/*
 * MultiFunction_Shield.c file for Multi_Function_Shield Library
 *
 * Created: 29.07.2018 17:00:00
 * Author : User007
 */ 

#include "MultiFunction_Shield.h"
#include "tick.h"

// ��������� ������������ ���������� � ������� (���������� - � MultiFunction_Shield.h)
// ��� ��� � ������������ ���������� - volatile
// ����������, ������� ������ ��������� ���� ��������� ����� �������� ������� - static

//-----------------------------------------------------------------------------------------------------------------------------------------------------
// ��������� ���������� ����������, � ������������� ������ ���� ��������� � �������������� extern � MultiFunction_Shield.h

uint8_t  mode = 1;
uint16_t buzzer_delay = 0;
uint16_t adc_delay = 0;
uint16_t rc5_delay = 0;
uint16_t temperature_delay = 0;
volatile uint8_t adc_result = 0;

#ifdef	USE_INTERRUPT_4_TSOP								// ���� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
volatile    uint16_t rc5_code = 0;							// ������� ���������� ���������� ��� ������������ ���� RC5
#endif

//-----------------------------------------------------------------------------------------------------------------------------------------------------
// ��������� ���������� ������������ ������������� ������ ������������� ������, � �� ������ ����� ����� ���������

static const uint8_t digit2segments[] =						// ��������� ������ �������� �� 0 �� 0x0F
{
	0xC0, // 0
	0xF9, // 1
	0xA4, // 2
	0xB0, // 3
	0x99, // 4
	0x92, // 5
	0x82, // 6
	0xF8, // 7
	0x80, // 8
	0x90, // 9
	0x88, // A
	0x83, // B
	0xC6, // C
	0xA1, // D
	0x86, // E
	0x8E, // F
	0xFF, // blank											// � �������� ������ ������ �������� � ��������� ������ "Err"
	0x86, // E
	0xAF, // r
	0xAF  // r
};

volatile uint8_t value2digits[4] = {0,0,0,0};				// ��������� ������ ������� ����������, ������� ����� ���������� ��� ��������� ����� ��������

//=====================================================================================================================================================
// ��������� ��������� ��������������� �������
void Write_74HC595(uint8_t byte);							// ������ 1 ����� � 74HC595
void Shield_display_digit(uint8_t addr, uint8_t digit);		// ����� 1 ������� �� ���������
void Shield_display_value(void);							// ����� �������� �� ���������
//=====================================================================================================================================================
// ������ ��������� ������� ������
//=====================================================================================================================================================
void Switch_Mode(void)										// ����� �������� ��� ������ ������
{
	switch(mode)
	{
		case 1:
		{
			OUTS_PORT_8_13 |= _BV(LED_3)|_BV(LED_2);
			OUTS_PORT_8_13 &= ~_BV(LED_1);
            adc_delay = t_ms;
			break;
		}
		case 2:
		{
			OUTS_PORT_8_13 |= _BV(LED_3)|_BV(LED_1);
			OUTS_PORT_8_13 &= ~_BV(LED_2);			
            rc5_delay = t_ms;
            Shield_set_display_value(0);
			break;
		}
		case 3:
		{
			OUTS_PORT_8_13 |= _BV(LED_2)|_BV(LED_1);
			OUTS_PORT_8_13 &= ~_BV(LED_3);
            temperature_delay = t_ms;
			break;
		}
		default: mode = 1;       
	}
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void Key_Press(void)										// ����� ������
{		
    static int16_t key_delay = 0;

    if (t_ms - key_delay + 0xFF00 >= 0xFF00)
    {
    	if (BUTTON_1 == 0)									
    	{
    		mode--;
    		if (mode == 0) mode = NUM_OF_MODES;
    		Buzer_Beep(SHORT_BEEP);
    		Switch_Mode();
			key_delay += 200;
    	}
    	else if (BUTTON_2 == 0)
    	{		
            
    	}
    	else if (BUTTON_3 == 0)
    	{		
    		mode++;
    		if (mode > NUM_OF_MODES) mode = 1;
    		Buzer_Beep(SHORT_BEEP);
    		Switch_Mode();
			key_delay += 200;
    	}

		else key_delay += 50;                               // ���� ��� ������� ������ ��������� ����� ����� 50 ��
    }
}
//=====================================================================================================================================================
// ������ �������������. �������� ������������� ������� ������������ ����� ���������� MultiFunction_Shield. ��� ������������� ����� ��������� ������� �������������
//=====================================================================================================================================================
void Init_Multi_Function_Shield(void)
{
// ������������� ������
	OUTS_DDR_0_7 = 1<<BUZZER|1<<SPI_CLK_PIN|1<<SPI_LATCH_PIN;// BUZZER, SPI_CLK_PIN and SPI_LATCH_PIN as output
	OUTS_PORT_0_7 = 1<<BUZZER;								// BUZZER off
	OUTS_DDR_8_13 = 1<<LED_4|1<<LED_3|1<<LED_2|1<<LED_1|1<<SPI_DATA_PIN;// LEDs and SPI_DATA_PIN as outputs
	OUTS_PORT_8_13 = 1<<LED_3|1<<LED_2|1<<LED_1;			// LEDs 1-3 is off
	ANALOG_PORT = _BV(BUTTON_3_PIN)|_BV(BUTTON_2_PIN)|_BV(BUTTON_1_PIN);// Enable pull-ups on Buttons
// ������������� UART
//-----------------------------------------------------------------------------------------------------------------------------------------------------
	UBRRL = LO(bauddivider);								
	UBRRH = HI(bauddivider);
	UCSRA = 0;
	UCSRB = 0<<RXCIE|0<<TXCIE|0<<UDRIE|1<<RXEN|1<<TXEN;		// ���������� UDRIE ����� ������� �� ���������, ����� ���������� ����� ������ � ��� ����������
	UCSRC = 1<<URSEL|1<<UCSZ1|1<<UCSZ0;
// ��������� ���
//-----------------------------------------------------------------------------------------------------------------------------------------------------
	ADMUX = 0<<REFS1|1<<REFS1|1<<ADLAR|0<<MUX3|0<<MUX2|0<<MUX1|0<<MUX0;	// AVCC � �������� ���, ������������ �� ������ ���� (8 ������� ��� ����������), 0 ����� ���
	ADCSRA = 1<<ADEN|0<<ADSC|0<<ADFR|1<<ADIF|1<<ADIE|0<<ADPS2|0<<ADPS1|0<<ADPS0;// ADC Enable, no start conversion, no ADC Free Running Select, ADC Interrupt enable, ADC Prescaler = 2
// ��������� ���������������� � ������� ������
//-----------------------------------------------------------------------------------------------------------------------------------------------------
	MCUCR = 1<<SE|0<<SM2|0<<SM1|1<<SM0|1<<ISC01;			// Sleep Enable, Sleep Mode - ADC Noise Reduction, The falling edge of INT0 generates an interrupt request
//-----------------------------------------------------------------------------------------------------------------------------------------------------
/*	#ifdef	USE_INTERRUPT_4_TSOP							// ���� ���������� ������� ���������� INT0 ��� ��������� ���� RC5 - ��� ����� ���� ���������� ���������� �� �������
	GIFR = 1<<INTF0;										// Clear External Interrupt Flag 0
	GICR = 1<<INT0;											// External Interrupt Request 0 Enable
	#endif */
}
//=====================================================================================================================================================
// ������ ������ � ����������� ����� 74HC595 (2 ��, ����������� ���������������)
//=====================================================================================================================================================
void Write_74HC595(uint8_t byte)							// ������ 1 ����� � 74HC595
{    
	for (uint8_t i = 0; i < 8; i++, byte <<= 1)
	{        
		if(byte & 0x80) SPI_DATA_HIGH();		
		else			SPI_DATA_LOW();		
		asm("nop");
		SPI_CLK_HIGH();
		asm("nop");
		SPI_CLK_LOW();  
	}
	asm("nop");
	SPI_LATCH_HIGH(); 
	asm("nop");
	SPI_LATCH_LOW();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void Shield_set_display_value(uint16_t value)				// ��������� �������� ��� ������ �� ���������
{
	for (uint8_t i = 0; i<4; i++)
	{
		value2digits[i] = 0;								// �������� ������, �����, �������, � �������
	}

	while (value >= 1000)									// ��������� ������
	{
		value -= 1000;
		value2digits[0]++;
	} 	
	while (value >= 100)									// �����
	{
		value -= 100;
		value2digits[1]++;
	} 
	while (value >= 10)										// �������
	{
		value -= 10;
		value2digits[2]++;
	} 
	while (value > 0)										// � �������
	{
		value -= 1;
		value2digits[3]++;
	}
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void Shield_display_digit(uint8_t addr, uint8_t digit)		// ����� 1 ������� �� ���������
{
	Write_74HC595(digit < sizeof(digit2segments) ? digit2segments[digit] : 0xFF);// ���� ������ � �������� 0-0x0F - ���������� ����������, ����� - ����� (���.1)
	Write_74HC595(1 << addr);								// � �������� ������ ������
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void Shield_display_value(void)								// ����� �������� �� ���������
{
	static uint8_t i = 0;									// ����������, ���������� ��� static �� �������� ����� ������ �� �������, � ��������� ���� ��������
    static int16_t display_delay = 0;

    if ((int16_t)(t_ms - display_delay) >= 0)               // ���� ������ ����� ���������� ��������� ������ ����������
    {
        Shield_display_digit(i,value2digits[i]);		    // ���������� ������ 7-���������� ��� � ������ �������
        i++;                                                // �������� ��������� ������ ����������
        i &= 0x03;											// �� ���� "i" ����� ������ 3, ������� ������� ������� (���� �������� ��� 0-1-2-3)
		display_delay += 5;                                 // ��������� �������� 5ms �� ��������� ����� ������� (������� ���������� 1 ������� �������� 50 ��)       
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void Shield_display_Err(void)								// ����� �������� ������ �� ���������
{
	for (uint8_t i = 0; i<4; i++)
	{
		value2digits[i] = i+16;                             // ��������� �������� ��� ������� ������� ������
	}
}
//=====================================================================================================================================================
ISR (ADC_vect)												// ���������� �� ��������� �������������� ���
{
	adc_result = ADCH;										// ���������� �� ��������� ��������������. ������� ��������� ��������������
}
//=====================================================================================================================================================
// ������ ��������� ���� RC5. � ����������� �� �������� �������� ���������� ��������� ����� �������������� � ����������� �������� ���������� INT0, ���� ���������� (������������� ������� ������� ������)
//=====================================================================================================================================================
#ifdef	USE_INTERRUPT_4_TSOP								// ���� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
ISR (INT0_vect)
{
	while (TSOP_PIN == 0);									// ���� ��������� ���������� ��������
	_delay_us(START_DELAY);									// �������� ������ ��������		
	for (uint8_t i=0; i<CODE_LEN; i++)						// ���������� ���� ������ ����� ����
	{
		rc5_code <<= 1;										// �������� ��� ����� 
		if (TSOP_PIN == 0)									// ���� �� ������ ��������� 0,
		rc5_code |= 1;										// ��� �������� ����� ���.1
		_delay_us(BIT_DELAY);								// ���� ��������� ������ ������
	}
	GIFR = 1<<INTF0;										// Clear External Interrupt Flag 0
	GICR = 0<<INT0;											// External Interrupt Request 0 Disable
	rc5_delay = t_ms + 250;                                 // ������� �������� �� ���������� ���������� �������� ����������
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#else														// ���� ���������� ������� ��� ��������� ���� RC5
uint16_t Get_RC5_code(void)									
{
	uint16_t code = 0;

	if (TSOP_PIN != 0)										// ������ ��� ���������� ��� ����������� ������
	{
		while (TSOP_PIN != 0)								// ���� ������ ���������� ��������
		{          
            t0_update();
            Blink_Led_4();
            Shield_display_value();						    // ����� �������� �� ���������
            Buzer_OFF();
            Key_Press();
			if (mode != TSOP_MODE) return code;				// ���� ����� ��� �������, �� �����
		}
		while (TSOP_PIN == 0);								// ���� ��������� ���������� ��������
		cli();												// ��������� ���������� ���� �������� ��������
		_delay_us(START_DELAY);								// �������� ������ ��������
		for (uint8_t i=0; i<CODE_LEN; i++)					// ���������� ���� ������ ����� ����
		{
			code <<= 1;										// �������� ��� ����� 
			if (TSOP_PIN == 0)								// ���� �� ������ ��������� 0,
			code |= 1;										// ��� �������� ����� ���.1		
			_delay_us(BIT_DELAY);							// ���� ��������� ������ ������
		}
		sei();												// ���������� ���� RC5 ���������, ��������� ����������
		rc5_delay = t_ms + 250;                             // ������� �������� �� ���������� ���������� �������� ����������
	}
	return code;											// ���������� ���������� ���
}
#endif
//=====================================================================================================================================================
void Buzer_Beep(beep_delay)									// �������� ������ ������������ X ��
{
	BUZZER_ON();
	buzzer_delay = t_ms + beep_delay;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void Buzer_OFF(void)                                        // ���������� ��������� �������
{
    if ((BUZZER_IS_ON) && (t_ms - buzzer_delay + 0xFF00 >= 0xFF00))
    {
        BUZZER_OFF();
    }
}
//=====================================================================================================================================================
void Send_Byte(uint8_t byte)								// �������� ������ ����� �� UART
{
	while (!(UCSRA & (1<<UDRE))){};							// ���� ����� ���������� UART
	UDR = byte;												// �������� ���� � ������� �����������
}
//=====================================================================================================================================================	
// ������ ������ � ����� 1-wire
//=====================================================================================================================================================	
uint8_t Reset_1_wire(void)									// ������������� ������ �� ���� 1-wire (Reset-Presence)
{
	ONE_WIRE_PIN_LOW();										// �������� ����� � 0
	_delay_us(480);											// ���������� ����� � ������ ��������� �� ���������� 480 ���
	ONE_WIRE_PIN_HIGH();									// ��������� �����
	uint8_t i=38;											// �������� ������� �� 60 ���
	for (; i>0; i--)							
	{
		_delay_us(1);											
		if (ONE_WIRE_LINE != 0) break;						// ����� ���������, ����������	
		else if (i==0) return 0;							// �������� ���������, ����� �� ��������� - ������������� �� ����� 
	}
	for (; i>0; i--)										// ���������� ������ �������� 60 ���
	{
		_delay_us(1);											
		if (ONE_WIRE_LINE == 0) break;						// ������ Presence ���������
		else if (i==0) return 0;							// �������� ���������, ������ Presence �� ���������
	}	
	_delay_us(55);											// ���� 55 ��� (Presence>=60)
	if (ONE_WIRE_LINE != 0) return 0;						// ��������� �����. ���� ������� - ����� ����������
	for (i=118; i>0; i--)									// �������� ������� �� 190 ���
	{
		_delay_us(1);											
		if (ONE_WIRE_LINE != 0) break;						// ����� ��������� - ��������� ����� Presence 						
		else if (i==0) return 0;							// �������� ���������, ����� �� ���������, ������������� �� �����
	}
	return 1;												// ���������� ����� ������������� ��������� ������
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void Send_1_wire_byte(uint8_t byte)							// �������� 1 ����� �� ���� 1-wire
{
	for (uint8_t i=0; i<8; i++)								// ���������� ���� �������� 1 �����
	{
		if((byte & (1<<i)) == 1<<i)							// Send 1
		{
			ONE_WIRE_PIN_LOW();								// �������� ����� � 0
			_delay_us(1);		
			ONE_WIRE_PIN_HIGH();							// ��������� �����
			_delay_us(60);									// ���� 60us	
		}
		else												// Send 0
		{
			ONE_WIRE_PIN_LOW();								// �������� ����� � 0		
			_delay_us(60);									// ���� 60us
			ONE_WIRE_PIN_HIGH();							// ��������� �����
			_delay_us(1);
		}
	}
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t One_wire_receive_byte(void)							// ����� 1 ����� �� ���� 1-wire
{	
	uint8_t one_wire_byte = 0;
	for (uint8_t i=0; i<8; i++)								// ���������� ���� �������� 1 �����
	{
		ONE_WIRE_PIN_LOW();									// �������� ����� � 0		
		_delay_us(1);
		ONE_WIRE_PIN_HIGH();								// ��������� �����
		_delay_us(12);										// ���� 12us
		if (ONE_WIRE_LINE == 0)
			one_wire_byte &=~(1<<i);
		else one_wire_byte |= 1<<i;
		_delay_us(47);										// ���� 47us �� ����� ����-����� (60us)
	}
	return one_wire_byte;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t Read_Temperature(void)								// ������ ����������� (������ ����� �����, ��� CRC)
{
	uint8_t temperature = 0;
	temperature = One_wire_receive_byte()>>4;				// ������ ������� ���� �����������, ��������� ����� ����� � ������� �������
	return (temperature | (One_wire_receive_byte() << 4));	// ������ ������� ���� �����������, ��������� �������� ����� � ������� �������, ��������� ������� 2 �������� ����, � ���������� ���������
}
//=====================================================================================================================================================
