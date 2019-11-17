
// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// button stuff
int buttonPinPause = 4;

int bounceDelay = 400;
int rotaryBounceDelay = 40;

// rotary encoder stuff
volatile boolean TurnDetected;	// need volatile for Interrupts
volatile boolean rotationdirection;	// CW or CCW rotation
const int rotatryClock=2;	 // Generating interrupts using CLK signal
const int rotatryData=5;		// Reading Data signal
// rotary switch as the confirm button
const int buttonPinConfirm=6;		// Reading Push Button switch



// gobal variables and stuff
int maxPlayerCount = 6;
int playerCount = 4;
String playerCountString = "4";

int maxGameTime = 60;
int minGameTime = 10;
int startingGameTime = 30;
long startingMilis = 30 * 60 * 1000;
String startingTimeString = "30";

String displayTimeString= "0";
long lastTick = 0;


long playerTimes[6] = {
	startingMilis,
	startingMilis,
	startingMilis,
	startingMilis,
	startingMilis,
	startingMilis};

int currentPlayer = 0;
int nonZeroIndexedCurrentPlayer = 1;
String currentPlayerString = "1";

/*
		Valid states
				- C : setting player count
				- T : sets amount of time per player
				- G : in game
				- K : player knocked out
				- P : timer paused
				- E : Error
*/
char GAME_STATE = 'C';
String ERROR_STRING = "";




void setup() {
	// set up the LCD's number of columns and rows:
	lcd.begin(16, 2);
	pinMode(buttonPinPause, INPUT_PULLUP);
	// Print a message to the LCD.
	pinMode(rotatryClock,INPUT);
	pinMode(rotatryData,INPUT);
	digitalWrite(buttonPinConfirm, HIGH); // Pull-Up resistor for switch
	attachInterrupt (0,isr,FALLING); // interrupt 0 always connected to pin 2 on Arduino UNO
}

void loop() {

	// draw functions
	switch(GAME_STATE){
		case 'C':
			displaySetPlayerCount();
			break;
		case 'T' :
			displaySetGameTime();
			break;
		case 'G' :
			displayInGame();
			break;
		case 'K' :
			displayKO();
			break;
		case 'P' :
			displayPaused();
			break;
		default :
			// should never get here, but should show that we did get here
			ERROR_STRING = "DrawState error";
			displayError();
			break;
	}

	// change game states
	if (digitalRead(buttonPinConfirm) == LOW){
		delay(bounceDelay);
		switch(GAME_STATE){
			case 'C':
				lcd.clear();
				GAME_STATE = 'T';
				break;
			case 'T':
				lcd.clear();
				// initialize game times
				for(int i = 0; i < 6; i++){
					playerTimes[i] = startingGameTime * 60000;
				}
				lastTick = millis();
				GAME_STATE = 'G';
				break;
			case 'K':
				// wait for a click to move to next player after a knock out.
				GAME_STATE = 'G';
			case 'G':
				// progress to next player
				lcd.clear();
				moveToNextPlayer();
				break;
			case 'E':
				// shouldn't come up but just incase, move to pause if there is an error
				GAME_STATE = 'P';
				break;
			case 'P' :
				// let either button unpause
				GAME_STATE = 'G';
				break;
			default :
				ERROR_STRING = "ConfirmButt ERR";
				GAME_STATE = "E";
				break;
		}
	}
	// pause timer
	if (digitalRead(buttonPinPause) == LOW){
		delay(bounceDelay);
		lcd.clear();
		if(GAME_STATE == 'P'){
			GAME_STATE = 'G';
		}else{
			GAME_STATE = 'P';
		}
	}
	// detect turns of the rotary encoder
	if (TurnDetected){
		if (rotationdirection) {
			// positive rotation
			switch(GAME_STATE){
				case 'C' :
					if(playerCount < maxPlayerCount){
						playerCount ++;
						playerCountString = playerCount;
					}
					break;
				case 'T':
					if(startingGameTime < maxGameTime){
						startingGameTime ++;
						startingTimeString = startingGameTime;
					}
				default :
				break;
			}
		} else {
		 // negative rotation
	        switch (GAME_STATE){
				case 'C' :
					if(playerCount >2){
						playerCount --;
						playerCountString = String(playerCount);
					}
					break;
				case 'T':
					if(startingGameTime > minGameTime){
						startingGameTime --;
						startingTimeString = startingGameTime;
					}
					break;
				default :
					// ignore this
					break;
	        }
		}
		TurnDetected = false;	// do NOT repeat IF loop until new rotation detected
	}

}

void displaySetPlayerCount(){
	lcd.setCursor(0, 0);
	lcd.print("Set player count");
	lcd.setCursor(0, 1);
	lcd.print(playerCountString + " Players ");
}

/*
		screen for setting amount of time for each player
*/
void displaySetGameTime(){
	lcd.setCursor(0, 0);
	lcd.print("Set player time");
	lcd.setCursor(0, 1);
	lcd.print(startingTimeString + " mins ");
}

/*
	screen showing remaining time for current player
	Also updates timers and time remaining.
*/
void displayInGame(){
	long thisPlayerRemainingTime = playerTimes[currentPlayer];
	// update the players time
	long now = millis();
	thisPlayerRemainingTime = thisPlayerRemainingTime - (now - lastTick);
	if(thisPlayerRemainingTime <= 0){
		GAME_STATE = 'K';
	} else{
		long mins = thisPlayerRemainingTime / 60000;
		long seconds = thisPlayerRemainingTime % 60000;
		seconds = seconds / 1000L;
		char timeDisplay[16];
		sprintf(timeDisplay, "%2ld:%2ld", mins, seconds);
		lcd.setCursor(0, 0);
		lcd.print("Player " + currentPlayerString);
		lcd.setCursor(0, 1);
		lcd.print(timeDisplay);
	}
	// set the remaining time
	playerTimes[currentPlayer] = thisPlayerRemainingTime;
	lastTick = now;
}

/*
	game paused screen
*/
void displayPaused(){
	lcd.setCursor(0, 0);
	lcd.print("Player " + currentPlayerString);
	lcd.setCursor(0, 1);
	lcd.print("Paused");
	lastTick = millis();
}

/*
	screen shown when a player is knocked out/out of time
*/
void displayKO(){
	lcd.setCursor(0, 0);
	lcd.print("Player " + currentPlayerString + " KO'd");
	lcd.setCursor(0, 1);
	lcd.print("Click to continue");
	lastTick = millis();
}

/*
	progress currentPlayer to next valid player
*/
void moveToNextPlayer(){
	currentPlayer ++;
	if(currentPlayer >= playerCount){
		currentPlayer = 0;
	}
	while(playerTimes[currentPlayer] <= 0){
		currentPlayer ++;
		if(currentPlayer >= playerCount){
			currentPlayer = 0;
		}
	}
	nonZeroIndexedCurrentPlayer = currentPlayer + 1;
	currentPlayerString = nonZeroIndexedCurrentPlayer;
}

/*
	screen for displaying errors
	shouldn't happen but just incase
*/
void displayError(){
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(ERROR_STRING);
	lcd.setCursor(0,1);
	lcd.print("Click to continue");
	lastTick = millis();
}

/*
	 interrupt function for the rotary encoder
*/
void isr ()	{
	delay(4);	// delay for Debouncing
	if (digitalRead(rotatryClock))
		rotationdirection = digitalRead(rotatryData);
	else
		rotationdirection = !digitalRead(rotatryData);
	TurnDetected = true;
}
