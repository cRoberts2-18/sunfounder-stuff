#include <stdio.h>
#include <wiringPi.h>
#include <pcf8591.h>
#include <math.h>

#define		PCF     120
#define		DOpin	0
#define uchar	unsigned char

int AIN0 = PCF + 0;
int AIN1 = PCF + 1;
int AIN2 = PCF + 2;

char *state[7] = {"home", "up", "down", "left", "right", "pressed"};

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
		while(status2==0){
		analogVal = analogRead(PCF + 0);
		Vr = 5 * (double)(analogVal) / 255;
		Rt = 10000 * (double)(Vr) / (5 - (double)(Vr));
		temp = 1 / (((log(Rt/10000)) / 3950)+(1 / (273.15 + 25)));
		temp = temp - 273.15;
		printf("Current temperature : %lf\n", temp);
		
		// For a threshold, uncomment one of the code for
		// which module you use. DONOT UNCOMMENT BOTH!
		//---------------------------------------------
		// 1. For Analog Temperature module(with DO)
		tmp = digitalRead(DOpin);

		if (tmp != status)
		{
			PrintTemp(tmp);
			status = tmp;
		}

		delay (200);
		
		analogVal = analogRead(PCF + 0);
		printf("%d\n", analogVal);

		tmp = digitalRead(DOpin);

		if (tmp != status)
		{
			PrintRain(tmp);
			status = tmp;
		}

		delay (200);
		}
	}
	
		
	
	
	return 0;
}

