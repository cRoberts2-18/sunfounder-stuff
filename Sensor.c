#include <stdio.h>
#include <wiringPi.h>
#include <pcf8591.h>
#include <math.h>
#include <wiringPiI2C.h>
#include <string.h>

#define		PCF     120
#define		DOpin	0
#define uchar	unsigned char

int AIN0 = PCF + 0;
int AIN1 = PCF + 1;
int AIN2 = PCF + 2;

int LCDAddr = 0x27;
int BLEN = 1;
int fd;

char *state[7] = {"home", "up", "down", "left", "right", "pressed"};
void write_word(int data){
	int temp = data;
	if ( BLEN == 1 )
		temp |= 0x08;
	else
		temp &= 0xF7;
	wiringPiI2CWrite(fd, temp);
}

void send_command(int comm){
	int buf;
	// Send bit7-4 firstly
	buf = comm & 0xF0;
	buf |= 0x04;			// RS = 0, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);

	// Send bit3-0 secondly
	buf = (comm & 0x0F) << 4;
	buf |= 0x04;			// RS = 0, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);
}

void send_data(int data){
	int buf;
	// Send bit7-4 firstly
	buf = data & 0xF0;
	buf |= 0x05;			// RS = 1, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);

	// Send bit3-0 secondly
	buf = (data & 0x0F) << 4;
	buf |= 0x05;			// RS = 1, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);
}

void init(){
	send_command(0x33);	// Must initialize to 8-line mode at first
	delay(5);
	send_command(0x32);	// Then initialize to 4-line mode
	delay(5);
	send_command(0x28);	// 2 Lines & 5*7 dots
	delay(5);
	send_command(0x0C);	// Enable display without cursor
	delay(5);
	send_command(0x01);	// Clear Screen
	wiringPiI2CWrite(fd, 0x08);
}

void clear(){
	send_command(0x01);	//clear Screen
}

void write(int x, int y, char data[]){
	int addr, i;
	int tmp;
	if (x < 0)  x = 0;
	if (x > 15) x = 15;
	if (y < 0)  y = 0;
	if (y > 1)  y = 1;

	// Move cursor
	addr = 0x80 + 0x40 * y + x;
	send_command(addr);
	
	tmp = strlen(data);
	for (i = 0; i < tmp; i++){
		send_data(data[i]);
	}
}

int direction(){
	int x, y, b;
	int tmp=0;
	x = analogRead(AIN1);
	y = analogRead(AIN0);
	b = analogRead(AIN2);
	if (y <= 30)
		tmp = 1;		// up
	if (y >= 225)
		tmp = 2;		// down
	
	if (x >= 225)
		tmp = 3;		// left
	if (x <= 30)
		tmp = 4;		// right

	if (b <= 30)
		tmp = 5;		// button preesd
	if (x-125<15 && x-125>-15 && y-125<15 && y-125>-15 && b >= 60)
		tmp = 0;		// home position
	
	return tmp;
}


void PrintTemp(int x)
{
	switch(x)
	{
		case 0:
			printf("\n************\n"  );
			printf(  "* Too Hot! *\n"  );
			printf(  "************\n\n");
		break;
		case 1:
			printf("\n***********\n"  );
			printf(  "* Better~ *\n"  );
			printf(  "***********\n\n");
		break;
		default:
			printf("\n**********************\n"  );
			printf(  "* Print value error. *\n"  );
			printf(  "**********************\n\n");
		break;
	}
}

void PrintRain(int x)
{
	switch(x)
	{
		case 1:
			printf("\n***************\n"  );
			printf(  "* Not Raining *\n"  );
			printf(  "***************\n\n");
		break;
		case 0:
			printf("\n*************\n"  );
			printf(  "* Raining!! *\n"  );
			printf(  "*************\n\n");
		break;
		default:
			printf("\n**********************\n"  );
			printf(  "* Print value error. *\n"  );
			printf(  "**********************\n\n");
		break;
	}
}

int main()
{
	unsigned char analogVal, analogVal1;
	double Vr, Rt, temp;
	int tmp, status, tmp1, status1,tmp2,status2;
	int pass = 0;
	if(wiringPiSetup() == -1){
		printf("setup wiringPi failed !");
		return 1;
	}
	// Setup pcf8591 on base pin 120, and address 0x48
	pcf8591Setup(PCF, 0x48);

	pinMode(DOpin, INPUT);

	status = 0;
	status2=0;
	tmp2=0;
	while(1) // loop forever
	{
		
		tmp2 = direction();
			if (tmp2 != status2)
			{
				printf("%s\n", state[tmp2]);
				status2 = tmp2;
			}
		
		while(status2!=0){
			pass=1;
			tmp2 = direction();
			if (tmp2 != status2)
			{
				printf("%s\n", state[tmp2]);
				status2 = tmp2;
				
			}
		}
		
			analogVal = analogRead(PCF + 0);
			Vr = 5 * (double)(analogVal) / 255;
			Rt = 10000 * (double)(Vr) / (5 - (double)(Vr));
			temp = 1 / (((log(Rt/10000)) / 3950)+(1 / (273.15 + 25)));
			temp = temp - 273.15;
			if(pass==0){
				printf("Current temperature : %lf\n", temp);
			}
			// For a threshold, uncomment one of the code for
			// which module you use. DONOT UNCOMMENT BOTH!
			//---------------------------------------------
			// 1. For Analog Temperature module(with DO)
			tmp = digitalRead(DOpin);

			if (tmp != status && pass==00)
			{
				PrintTemp(tmp);
				status = tmp;
			}

			delay (200);

			analogVal = analogRead(PCF + 0);
			if(pass==0){
				printf("%d\n", analogVal);
			}
			tmp = digitalRead(DOpin);

			if (tmp != status && pass==0)
			{
				PrintRain(tmp);
				status = tmp;
			}
			
		
			fd = wiringPiI2CSetup(LCDAddr);
			init();
			write(0, 0, "Greetings!");
			write(1, 1, "From SunFounder");
			delay(2000);
		
			pass=0;
	}
	
		
	
	
	return 0;
}

