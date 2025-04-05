#include <avr/io.h>
#include "UART.h"
#define NULL ((void*)0)
#define F_CPU 16000000UL

// Initialization


// Prototypes
// Initialization
void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop);

// Send
void UART_puts(uint8_t com, char *str);
void UART_putchar(uint8_t com, char data);

// Received
uint8_t UART_available(uint8_t com);
char UART_getchar(uint8_t com );
void UART_gets(uint8_t com, char *str);

// Escape sequences
void UART_clrscr( uint8_t com );
void UART_setColor(uint8_t com, uint8_t color);
void UART_gotoxy(uint8_t com, uint8_t x, uint8_t y);

// Utils
void itoa(uint16_t number, char* str, uint8_t base);
uint16_t atoi(char *str);



typedef struct {
    uint8_t UCSRA;
    uint8_t UCSRB;
    uint8_t UCSRC;
    uint8_t res;
    uint16_t UBRR;
} UART_regs;

#define UART0_BASE ((UART_regs *)0xC0)
#define UART1_BASE ((UART_regs *)0xC8)
#define UART2_BASE ((UART_regs *)0xD0)
#define UART3_BASE ((UART_regs *)0x130)

UART_regs *uart_offset[] = {
    UART0_BASE,
    UART1_BASE,
    UART2_BASE,
    UART3_BASE
};
const uint8_t UCSZ2[] = { UCSZ02, UCSZ12, UCSZ22, UCSZ32 };
const uint8_t UCSZ1[] = { UCSZ01, UCSZ11, UCSZ21, UCSZ31 };
const uint8_t UCSZ0[] = { UCSZ00, UCSZ10, UCSZ20, UCSZ30 };
const uint8_t USBS[]  = { USBS0, USBS1, USBS2, USBS3 };
const uint8_t UPM0[]  = { UPM00, UPM10, UPM20, UPM30 };
const uint8_t UPM1[]  = { UPM01, UPM11, UPM21, UPM31 };
const uint8_t UDRE[] = { UDRE0, UDRE1, UDRE2, UDRE3 };
const uint8_t RXC[] = { RXC0, RXC1, RXC2, RXC3 };

void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop) {
    if (com > 3) return;  // Verifica que la UART esté dentro del rango

    UART_regs *myuart = uart_offset[com];

    // Configurar la velocidad (baud rate)
    uint16_t ubrr = (F_CPU / (16 * baudrate)) - 1;  // UBRR = F_CPU / (16 * baudrate) - 1
    myuart->UBRR = ubrr;

    // Limpiar los bits de tamaño de datos
   myuart->UCSRB &= ~(1 << UCSZ2[com]);
    myuart->UCSRC &= ~((1 << UCSZ1[com]) | (1 << UCSZ0[com]));

    if (size == 6) {
        myuart->UCSRC |= (1 << UCSZ0[com]);         // 6 bits
    } 
    else if (size == 7) {
        myuart->UCSRC |= (1 << UCSZ1[com]);         // 7 bits
    } 
    else if (size == 8) {
        myuart->UCSRC |= (1 << UCSZ1[com]) | (1 << UCSZ0[com]);   // 8 bits
    }
  

    // Configurar paridad
    myuart->UCSRC &= ~((1 << UPM1[com]) | (1 << UPM0[com]));
    if (parity == 1) {           // Impar
        myuart->UCSRC |= (1 << UPM1[com]) | (1 << UPM0[com]);
    } 
    else if (parity == 2) {      // Par
        myuart->UCSRC |= (1 << UPM1[com]);
    }

    // Configurar los bits de stop
    if (stop == 1) {
        myuart->UCSRC |= (1 << USBS[com]);   // 2 bits de stop
    } else {
        myuart->UCSRC &= ~(1 << USBS[com]);  // 1 bit de stop
    }
}

void UART_puts(uint8_t com, char *str)
{
	 while (*str) {
		
        UART_putchar(com, *str++);
    }
	
}
void UART_putchar(uint8_t com, char data)
{
	if(com>3) return;
	UART_regs *myuart = uart_offset[com];
	while (!((myuart->UCSRA) & (1 << UDRE[com])));

	// Colocar el dato en el buffer de transmisión
	if (com == 0) UDR0 = data;
	else if (com == 1) UDR1 = data;
	else if (com == 2) UDR2 = data;
	else if (com == 3) UDR3 = data;
}

uint8_t UART_available(uint8_t com)
{
	if(com>3)return 0;
	UART_regs *myuart = uart_offset[com];
	if(myuart->UCSRA &(1<<RXC[com]))// si hay 1 hay datos disponoble
	{
		return 1;
	}
	else
	{
		return 0;
	}
	
}
char UART_getchar(uint8_t com )
{
		if(com>3)return 0xFF;
	UART_regs *myuart = uart_offset[com];
	  while (!(myuart->UCSRA & (1 << RXC[com])||(com==3)));
		if (com == 0) return UDR0;//retorno el contenido del registro
	else if (com == 1) return UDR1;
	else if (com == 2) return UDR2;
	else if (com == 3) return UDR3;

	return -1;  
}
void UART_gets(uint8_t com, char *str)
{
    if (com > 3 || str == NULL) return;
    
    char c;
    uint16_t i = 0;

    while (1) {
       // while (!UART_available(com));  // Espera hasta que haya un carácter disponible
        
        c = UART_getchar(com);  // Recibir carácter
		if(com==3)
		{
			UART_putchar(0,'#');
			UART_putchar(0,c);
		}
		
		

        if (c == '\n' || c == '\r') {  
            str[i] = '\0';  // Terminar cadena
            break;
        }
        else if (c == '\b' || c == 127) {  // Retroceso 
            if (i > 0) {
                i--;
                UART_puts(com, "\b \b");  // Borra el carácter en la pantalla
            }
        }
        else if (i < 19) {  // Evitar desbordamiento
            str[i++] = c;
            UART_putchar(com, c); 
        }
    }
}

void UART_clrscr( uint8_t com )
{
	 if (com > 3) return;  

    
    UART_puts(com, "\x1B[2J\x1B[H");

}
void UART_setColor(uint8_t com, uint8_t color)
{
    if (com > 3) return;

    char buffer[10];
    uint8_t i = 0;

    buffer[i++] = 0x1B;  
    buffer[i++] = '[';

    
    if (color >= 10) {  
        buffer[i++] = '0' + (color / 10); 
    }
    buffer[i++] = '0' + (color % 10);  

    buffer[i++] = 'm';
    buffer[i] = '\0';

    UART_puts(com, buffer);
}
void UART_gotoxy(uint8_t com, uint8_t x, uint8_t y)
{
	 if (com > 3) return;  

    char buffer[10];  
    uint8_t i = 0;

    
    buffer[i++] = 0x1B;  
    buffer[i++] = '[';    

 
    if (y < 10) {
        buffer[i++] = '0' + y;  
    } else {
        buffer[i++] = '0' + (y / 10);  
        buffer[i++] = '0' + (y % 10);  
    }

    buffer[i++] = ';';  

   
    if (x < 10) {
        buffer[i++] = '0' + x;  
    } else {
        buffer[i++] = '0' + (x / 10);  
        buffer[i++] = '0' + (x % 10);  
    }

    buffer[i++] = 'H'; 

    buffer[i] = '\0';  

    
    UART_puts(com, buffer);
}

void itoa(uint16_t number, char* str, uint8_t base)
{
	uint8_t i = 0;
    uint8_t temp;
	if(base==10)
	{
		if (number == 0) {
            str[i++] = '0';  
        } else {
            while (number > 0) {
                temp = number % 10;
                str[i++] = temp + '0'; 
                number /= 10;
            }
        }
	}
	if(base == 16)
	{
		if (number == 0) {
            str[i++] = '0';  
        } else {
            while (number > 0) {
                temp = number % 16;
                str[i++] = (temp < 10) ? (temp + '0') : (temp - 10 + 'A');  
                number /= 16;
            }
        }
	}
	if(base == 2)
	{
		if (number == 0) {
            str[i++] = '0';  
        } else {
            while (number > 0) {
                str[i++] = (number % 2) + '0';  
                number /= 2;
            }
        }
	}
	if(base == 8)
	{
		 if (number == 0) {
            str[i++] = '0';  
        } else {
            while (number > 0) {
                temp = number % 8;
                str[i++] = temp + '0';  
                number /= 8;
            }
        }
	}

	 str[i] = '\0';
    
    
    uint8_t start = 0;
    uint8_t end = i - 1;
    while (start < end) {
        
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}
uint16_t atoi(char *str)
{
	 uint16_t result = 0;
    uint8_t i = 0;
    while (str[i] != '\0') { 
        if (str[i] >= '0' && str[i] <= '9') { 
            result = result * 10 + (str[i] - '0');
        } else { 
            break;  
        }
        i++;
    }
    
    return result;
}
int main( void )
{
	char cad[20];
	char cadUart3[20];
	uint16_t num;

	UART_Ini(0,12345,8,1,2);
	UART_Ini(2,115200,8,0,1);
	UART_Ini(3,115200,8,0,1);
	while(1)
	{
		UART_getchar(0);
		UART_clrscr(0);

		UART_gotoxy(0,2,2);
		UART_setColor(0,YELLOW);
		UART_puts(0,"Introduce un numero:");
		
		UART_gotoxy(0,22,2);
		UART_setColor(0,GREEN);
		UART_gets(0,cad);
		// -------------------------------------------
		// Cycle through UART2->UART3
		UART_puts(2,cad);
		UART_puts(2,"\r");
		//UART_gets(3,cadUart3);
		UART_gotoxy(0,5,3);
		UART_puts(0,cadUart3);
		// -------------------------------------------
		num = atoi(cad);
		itoa(num,cad,16);
		
		UART_gotoxy(0,5,4);
		UART_setColor(0,BLUE);
		UART_puts(0,"Hex: ");
		UART_puts(0,cad);
		itoa(num,cad,2);
		
		UART_gotoxy(0,5,5);
		UART_puts(0,"Bin: ");
		UART_puts(0,cad);
	}
}
