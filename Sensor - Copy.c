#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <pcf8591.h>
#include <math.h>
#include <wiringPiI2C.h>
#include <string.h>
#include <time.h>


#define		PCF     120
#define		DOpin	0
#define uchar	unsigned char

FILE *file;

int AIN0 = PCF + 0;
int AIN1 = PCF + 1;
int AIN2 = PCF + 2;

int LCDAddr = 0x27;
int BLEN = 1;
int fd;


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

int main(){
	
	unsigned char analogVal, analogVal1;
	double Vr, Rt, temperature=1, temp=1, maxTemp=0, minTemp=9999999, hourlyAvg=0;
	int tmp, status, tmp1, status1,tmp2,status2,  rain;
	int firstPass = 1;
	int buggedValue=0;
	int buggedValue1=0;
	int loopnum=0;
	int timer=0;
	int counter=0;

	if(wiringPiSetup() == -1){
		printf("setup wiringPi failed !");
		return 1;
	}
	// Setup pcf8591 on base pin 120, and address 0x48
	pcf8591Setup(PCF, 0x48);

	pinMode(DOpin, INPUT);

	fd = wiringPiI2CSetup(LCDAddr);
	init();

	clear();

	status = 0;
	status2=0;
	tmp2=0;
	while(1) // loop forever
	{
	loopnum=loopnum%40;
		if(loopnum==0){
			clear();
			write(0, 0, "Current Temp:   ");
			}
		if(loopnum==10){
			clear();
			write(0, 0, "Max Temp:       ");
			}
		if(loopnum==20){
			clear();
			write(0, 0, "Min Temp:       ");
			}
		if(loopnum==30){
			clear();
			write(0, 0, "Rainfall:       ");
			}


			analogVal = analogRead(PCF + 0);
			Vr = 5 * (double)(analogVal) / 255;
			Rt = 10000 * (double)(Vr) / (5 - (double)(Vr));
			temp = 1 / (((log(Rt/10000)) / 3950)+(1 / (273.15 + 25)));
			temp = temp - 273.15;
			char tempstr[15];
			char templcd[]="";
			snprintf(tempstr, 50, "%f", temp);
			strcat(templcd,tempstr);
			strcat(templcd,"           ");
			if(loopnum<10){
				write(0,1,templcd);
			}

			if(round(temp*100)!=round(temperature*100)){
				temperature=temp;
				if(temp>maxTemp){
					maxTemp=temp;
					printf("New max %f\n", maxTemp);
					if(buggedValue==0){
						buggedValue=1;
					}
				}
				if(buggedValue==1){
					maxTemp=0;
					buggedValue=2;
				}


				if(temp<minTemp){
					minTemp=temp;
					//snprintf(minlcd, 15, "%d", temp);
					printf("New min %f\n", temp);
					if(buggedValue1==0){
						buggedValue1=1;
					}
				}
				if(buggedValue1==1){
					minTemp=999;
					buggedValue1=2;
				}
			}

			char mintmp[15];
			char minlcd[]="";
			snprintf(mintmp, 50 , "%f", minTemp);
			strcat(minlcd,mintmp);
			strcat(minlcd,"               ");
			if(loopnum>=20&&loopnum<30){
				write(0, 1, minlcd);
			}

			char maxtmp[15];
			char maxlcd[]="";
			snprintf(maxtmp, 50, "%f",maxTemp);
			strcat(maxlcd,maxtmp);
			strcat(maxlcd,"                 ");
			if(loopnum>=10&&loopnum<20){
				write(0, 1, maxlcd);
			}


			tmp = digitalRead(DOpin);

			if (tmp != status){
				PrintTemp(tmp);
				status = tmp;
			}


			analogVal = analogRead(PCF + 0);

			char raintemp[15];
			char rainlcd[]="";
			snprintf(raintemp, 15, "%d", rain);
			strcat(rainlcd,raintemp);
			strcat(rainlcd,"               ");
			if(loopnum>=30){
				write(0,1,rainlcd);
			}
			if(analogVal!=rain){
				rain=analogVal;
			}


			tmp = digitalRead(DOpin);

			if (tmp != status)
			{
				PrintRain(tmp);
				status = tmp;
			}
			if(timer==0){
				file=fopen("Readings.csv","w");
					fprintf(file,"Temperature, Max Temperature, Min Temperature, Rain, Reading Time\n");
					fclose(file);
			}
			if(time%60==0){
				hourlyAvg+=temp;
				counter+=1;
			}
			if(timer%3600==0){
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);
				int avgTemp	= hourlyAvg/counter;
				file = fopen("Readings.csv","a");
				fprintf(file,"%f,%f,%f,%d,%d-%02d-%02d %02d:%02d:%02d\n",avgTemp,maxTemp,minTemp,rain,tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				fclose(file);
				hourlyAvg=0;
				maxTemp=0;
				minTemp=999;
				counter=0;
			}
			if(timer==(3600*24)){
			exit(1);
			}

			timer+=1;
			loopnum+=1;
			delay(1000);
	}

	return 0;
}
