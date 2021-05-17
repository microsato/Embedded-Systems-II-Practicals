/*
 * Prac4.cpp
 * 
 * Originall written by Stefan SchrÃ¶der and Dillion Heald 
 * 
 * Adapted for EEE3096S 2019 by Keegan Crankshaw
 * 
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "Prac4.h"

using namespace std;

bool playing = true; // should be set false when paused
bool stopped = false; // If set to true, program should close
unsigned char buffer[2][BUFFER_SIZE][2];
int buffer_location = 0;
bool bufferReading = 0; //using this to switch between column 0 and 1 - the first column
bool threadReady = false; //using this to finish writing the first column at the start of the song, before the column is played
int lastInt;
int interruptTime;

// Configure interrupts 
void play_pause_isr(void){
	long interruptTime=millis();
	if(interruptTime-lastInt>200){
	
		if (playing == false){
			playing = true;    
		} else{
			playing = false;
		}
	}
lastInt = interruptTime;
}

void stop_isr(void){
	long interruptTime=millis();
	if(interruptTime-lastInt>200){
		stopped = true;
	}
lastInt = interruptTime;
}

/*
 * Setup Function. Called once 
 */
int setup_gpio(void){
    printf("Setting up\n");
    //Set up wiring Pi
    wiringPiSetup();
    wiringPiSPISetup(SPI_CHAN,SPI_SPEED);
    //setting up the buttons

    pinMode(PLAY_BUTTON, INPUT);
    pinMode(STOP_BUTTON, INPUT);

    pullUpDnControl(PLAY_BUTTON, PUD_UP);
    pullUpDnControl(STOP_BUTTON, PUD_UP);
    printf("Setup complete\n");
    return 0;
}

void *playThread(void *threadargs){
    // If the thread isn't ready, don't do anything
    while(!threadReady)
        continue;
    
    //play if the stopped flag is false
    while(!stopped & playing){
        //Write the buffer out to SPI
        wiringPiSPIDataRW (SPI_CHAN, buffer[bufferReading][buffer_location], 2);
		
                buffer_location++;
        if(buffer_location >= BUFFER_SIZE) {
            buffer_location = 0;
            bufferReading = !bufferReading; // switches column one it finishes one column
        }
    }
    
    pthread_exit(NULL);
}

int main(){
    // Call the setup GPIO function
	if(setup_gpio()==-1){
        return 0;
    }
        
    
	pthread_attr_t tattr;
    	pthread_t thread_id;
    	int newprio = 99;
    	sched_param param;
    
    	pthread_attr_init (&tattr);
    	pthread_attr_getschedparam (&tattr, &param); /* safe to get existing scheduling param */
    	param.sched_priority = newprio; /* set the priority; others are unchanged */
    	pthread_attr_setschedparam (&tattr, &param); /* setting the new scheduling param */
    	pthread_create(&thread_id, &tattr, playThread, (void *)1); /* with new priority specified */
    
    // Open the file
    char ch;
    FILE *filePointer;
    printf("%s\n", FILENAME);
    filePointer = fopen(FILENAME, "r"); // read mode

    if (filePointer == NULL) {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }

    int counter = 0;
    int bufferWriting = 0;

    // Have a loop to read from the file
	 while((ch = fgetc(filePointer)) != EOF){
        while(threadReady && bufferWriting==bufferReading && counter==0){
            continue;
        }
        //Set config bits for first 8 bit packet and OR with upper bits
        buffer[bufferWriting][counter][0] =0b00110000 | (ch>>6); //TODO
        //Set next 8 bit packet
        buffer[bufferWriting][counter][1] =0b11111100 & (ch<<2); //TODO

        counter++;
        if(counter >= BUFFER_SIZE+1){
            if(!threadReady){
                threadReady = true;
            }

            counter = 0;
            bufferWriting = (bufferWriting+1)%2;
        }

    }
     
    // Close the file
    fclose(filePointer);
    printf("Complete reading"); 
	 
    //Join and exit the playthread
	pthread_join(thread_id, NULL); 
    pthread_exit(NULL);
	
    return 0;
}
