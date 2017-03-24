#include "pitches.h" // Thanks to https://www.arduino.cc/en/Tutorial/toneMelody

#define NUMBUTTONS 7
#define NUMLEDS 7
#define NUMCRAZYLEDS 2
#define DEBOUNCE_DELAY 50
#define SPEAKER 3

// Universal Variables
enum GAME_STATE {BOOTING, MENU, INITIALIZING, PLAYING, GAME_OVER};
GAME_STATE gameState = BOOTING;
void (* doPlayCurrentGame)();
void (* doInitializeCurrentGame)();
byte switches[] = {A0, A1, A2, A3, A4, A5, 2};
unsigned long switchLastDebounceTime[] = { millis(), millis() ,millis(), millis(), millis(), millis(), millis()};
bool isDebouncing[] = {false, false, false, false, false, false, false};
byte switchState[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
byte LEDs[] = {7, 8, 9, 10, 11, 12, 13};
byte LEDState[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW};
byte crazyLEDs[] = {4,5};
unsigned long gameOver_resetTime = 0;
unsigned int buttonNotes[] = {NOTE_C6, NOTE_D6, NOTE_E6, NOTE_F6, NOTE_G6, NOTE_A6, NOTE_AS6};
bool isWinner = true;

// MENU variables
unsigned long menu_lightTimer = millis();
byte menu_currentLightIndex = 6; 

// EVEN ODD Game variables
unsigned int eo_Order[] = {1, 3, 5, 0, 2, 4, 6};
unsigned int eo_nextButtonIndex = 0;

// TORNADO CHASER Game variables
byte tc_currentLightIndex = 0;
unsigned long tc_lightTimer = millis();
byte tc_chosenLights[NUMLEDS] = {};
byte tc_currentLevel = 1;

// PSYMAN Game variables
bool ps_isRevealing = true;
unsigned int ps_sequenceQueue[20] = {};
byte ps_currentUserGuess = 0;
byte ps_sequenceLength = 0;

// SONGS
unsigned int song_victoryNotes[] = {NOTE_C4, 0, NOTE_C4, NOTE_C4, NOTE_G5};
unsigned int song_victoryDurations[] = {8, 8, 8, 8, 2};

unsigned int song_bootingNotes[] = {NOTE_C7, NOTE_G6, NOTE_E7, NOTE_C7};
unsigned int song_bootingDurations[] = {16, 16, 16, 8};


// Forward Declarations
void playBeep(); // Plays a benign "beep" from the speaker
void playX(); // Plays an unpleasant "BRRPT" from the speaker
void playBootingMusic(); // Default Ditty used when done booting or selecting a game from the menu
void playVictoryMusic(); // Plays music indicating victory
void playFailureMusic(); // Plays music indicating failure
bool buttonPressedNow(byte); // Takes care of button debouncing;
                             //returns true if this is the first frame a button has been pressed

void doBooting(); // GAME STATE FUNCTION used when the Arduino is booting from power-on
void doMenu(); // GAME STATE FUNCTION used when in Menu state (after BOOTING or after GAME_OVER)

void initializeEvenOdd(); // Uused for INITIALIZING the Even-Odd game
void playEvenOdd(); // Used for PLAYING the Even-Odd game
void initializeBuddies(); // Used for INITIALIZING the Buddies game
void playBuddies(); // Used for PLAYING the Buddies game
void initializeTornadoChaser(); // Used for INITIALIZING the Tornado Chaser game
void playTornadoChaser(); // Used for PLAYER the Tornado Chaser game
bool tornadoChaser_isValidLight(); // Used by the Tornado Chaser game
void initializePsyman(); // Used for INITIALIZING the Psyman game
void playPsyman(); // Used for PLAYER the Psyman game
void initializeFreePlay(); // Used for INITIALIZING the Free Play "game"
void playFreePlay(); // Used for PLAYING the Free Play "game"
void initializeBinaryCounting(); // Used for INITIALIZING the Binary Counting game
void playBinaryCounting(); // Used for PLAYER the Binary Counting game
bool binaryCounting_canToggleLight(byte);
void initializeHardMode();  // Used for INITIALIZING the Hard Mode game
void playHardMode(); // Used for PLAYING the Hard Mode game
bool hardMode_canToggleLight(byte _whichButton, bool _isRecursing = false); // Used by the Hard Mode game

void doGameOver(); // GAME STATE MODE after PLAYING is finished; then goes back to MENU state



// SETUP

void setup() {
  gameState = BOOTING;

  randomSeed(analogRead(0));
  
  for(int i = 0; i < NUMBUTTONS; ++i)
  {
    pinMode(switches[i], INPUT_PULLUP);
  }

  for(int i = 0; i < NUMLEDS; ++i)
  {
    pinMode(LEDs[i], OUTPUT);
    digitalWrite(LEDs[i], LEDState[i]);
  }

  for(int i = 0; i < NUMCRAZYLEDS; ++i)
  {
    pinMode(crazyLEDs[i], OUTPUT);
  }
}



// LOOP

void loop() {
  switch (gameState)
  {
    case BOOTING:
    {
      doBooting();
    }
    break;
    case MENU:
    {
      doMenu();
    }
    break;
    case INITIALIZING:
    {
      doInitializeCurrentGame();
    }
    break;
    case PLAYING:
    {
      doPlayCurrentGame();
    }
    break;
    case GAME_OVER:
    {
      doGameOver();
    }
    break;
  }

}







void playBeep()
{
    tone(SPEAKER, NOTE_C4, 1000/32);
    //delay((1000/32)* 1.3);
    noTone(SPEAKER);
}

void playX()
{
    tone(SPEAKER, NOTE_B2, 1000/32);
    delay((1000/32)* 1.3);
    noTone(SPEAKER);
}

void playBootingMusic()
{
  for (int thisNote = 0; thisNote < 4; thisNote++) {
    int noteDuration = 1000 / song_bootingDurations[thisNote];
    tone(SPEAKER, song_bootingNotes[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    noTone(SPEAKER);
  }
}

void playVictoryMusic()
{
  for (int thisNote = 0; thisNote < 5; thisNote++) {
    int noteDuration = 1000 / song_victoryDurations[thisNote];
    tone(SPEAKER, song_victoryNotes[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    noTone(SPEAKER);
  } 
}

void playFailureMusic()
{
    tone(SPEAKER, NOTE_G4, 1000/8);
    delay((1000/8)* 1.3);
    delay((1000/8)* 1.3);  // Delay again, no sound this time
    tone(SPEAKER, NOTE_CS4, 1000/1);
    delay((1000/1)* 1.3);
    noTone(SPEAKER); 
}

// This takes care of all button debouncing (so it must be called every frame)
// It also returns true if this is the first loop that the debounced button has been pressed
bool buttonPressedNow(byte _whichButton)
{
  if((digitalRead(switches[_whichButton]) != switchState[_whichButton]) && (!isDebouncing[_whichButton]))
  {
    switchLastDebounceTime[_whichButton] = millis() + DEBOUNCE_DELAY;
    isDebouncing[_whichButton] = true;
  }

  if(isDebouncing[_whichButton] && (switchLastDebounceTime[_whichButton] < millis()))
  {
    isDebouncing[_whichButton] = false;
    switchState[_whichButton] = digitalRead(switches[_whichButton]);
    if(LOW == switchState[_whichButton])
    {
      return true;
    }
  }

  return false;
}



void doBooting()
{
  playBootingMusic();

  gameState = MENU;
}

void doMenu()
{
  // Cycle lights to prompt user to choose a game
  if(millis() > menu_lightTimer)
  {
    if(6 < ++menu_currentLightIndex)
    {
      menu_currentLightIndex = 0;
    }

    // Reset All lights
    for(int i = 0; i < NUMLEDS; ++i)
    {
      digitalWrite(LEDs[i], LOW);
    }

    digitalWrite(LEDs[menu_currentLightIndex], HIGH);
    menu_lightTimer = millis() + 200;
  }


  // Check for chosen game
  for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
  {
    if(buttonPressedNow(buttonIndex))
    {
      // Reset All lights
      for(int i = 0; i < NUMLEDS; ++i)
      {
        LEDState[i] = LOW;
        digitalWrite(LEDs[i], LEDState[i]);
      }

      switch (buttonIndex)
      {
        case 1:
        {
          doInitializeCurrentGame = initializeBuddies;
          doPlayCurrentGame = playBuddies;
        }
        break;
        case 2:
        {
          doInitializeCurrentGame = initializeTornadoChaser;
          doPlayCurrentGame = playTornadoChaser;
        }
        break;
        case 3:
        {
          doInitializeCurrentGame = initializePsyman;
          doPlayCurrentGame = playPsyman;
        }
        break;
        case 4:
        {
          doInitializeCurrentGame = initializeFreePlay;
          doPlayCurrentGame = playFreePlay;
        }
        break;
        case 5:
        {
          doInitializeCurrentGame = initializeBinaryCounting;
          doPlayCurrentGame = playBinaryCounting;
        }
        break;
        case 6:
        {
          doInitializeCurrentGame = initializeHardMode;
          doPlayCurrentGame = playHardMode;
        }
        break;
        case 0:
        default:
        {
          doInitializeCurrentGame = initializeEvenOdd;
          doPlayCurrentGame = playEvenOdd;
        }
        break;
      }


      digitalWrite(LEDs[buttonIndex], HIGH);

      // Play menu chosen song (booting music for now)
      playBootingMusic();

      // Reset All lights
      for(int i = 0; i < NUMLEDS; ++i)
      {
        LEDState[i] = LOW;
        digitalWrite(LEDs[i], LEDState[i]);
      }

      playBeep();

      gameState = INITIALIZING;
    }
  }
}

void initializeEvenOdd()
{
  eo_nextButtonIndex = 0;
  gameState = PLAYING;
}

void playEvenOdd()
{
  for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
  {
    if(buttonPressedNow(buttonIndex))
    {
      if(eo_Order[eo_nextButtonIndex] == buttonIndex)
      {
        playBeep();
        LEDState[buttonIndex] = !LEDState[buttonIndex];
        digitalWrite(LEDs[buttonIndex], LEDState[buttonIndex]);
        if(7 <= ++eo_nextButtonIndex)
        {
              gameState = GAME_OVER;
        }
      }
      else  // Wrong Button!
      {
        playX();
        eo_nextButtonIndex = 0;
        for(int i = 0; i < NUMLEDS; ++i)
        {
          LEDState[i] = LOW;
          digitalWrite(LEDs[i], LEDState[i]);
        }
      }
    }
  }
}

void initializeBuddies()
{
  gameState = PLAYING;
}

void playBuddies()
{
  for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
  {
    if(buttonPressedNow(buttonIndex))
    {
      playBeep();
      if(buttonIndex - 1 >= 0)
      {
        LEDState[buttonIndex - 1] = !LEDState[buttonIndex - 1];
        digitalWrite(LEDs[buttonIndex - 1], LEDState[buttonIndex - 1]);
      }
        
      if(buttonIndex - 1 <= 6)
      {
        LEDState[buttonIndex + 1] = !LEDState[buttonIndex + 1];
        digitalWrite(LEDs[buttonIndex + 1], LEDState[buttonIndex + 1]);
      }

      if(LEDState[0] && LEDState[1] && LEDState[2] && LEDState[3] && LEDState[4] && LEDState[5] && LEDState[6])
      {
        gameState = GAME_OVER;
      }
    }
  }
}

void initializeTornadoChaser()
{
  tc_currentLevel = 1;
  
  for(byte i = 0; i < NUMLEDS; ++i)
  {
    tc_chosenLights[i] = 7;
  }
  tc_lightTimer = millis() + 1000/tc_currentLevel;
  tc_currentLightIndex = 0;
  digitalWrite(LEDs[tc_currentLightIndex], HIGH);
  gameState = PLAYING;
}


void playTornadoChaser()
{
  // Cycle lights
  if(millis() > tc_lightTimer)
  {
    do
    {
      if(6 < ++tc_currentLightIndex)
      {
        tc_currentLightIndex = 0;
      }
    } while (!tornadoChaser_isValidLight());


    // Reset All LEDs
    for(int i = 0; i < NUMLEDS; ++i)
    {
      digitalWrite(LEDs[i], LOW);
    }

    // Light up LEDs
    digitalWrite(LEDs[tc_currentLightIndex], HIGH);
    for(byte i = 0; i < NUMLEDS; ++i)
    {
      if(7 > tc_chosenLights[i])
      {
        digitalWrite(LEDs[tc_chosenLights[i]], HIGH);
      }
    }

    
    tc_lightTimer = millis() + 1000/tc_currentLevel;
  }


  // If they hit a button, see if it was correct
  for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
  {
    if(buttonPressedNow(buttonIndex))
    {
      if(tc_currentLightIndex == buttonIndex)
      {
        playBeep();
        tc_chosenLights[tc_currentLevel - 1] = tc_currentLightIndex;
        if(6 < ++tc_currentLevel)
        {
          // Light up all LEDs
          for(int i = 0; i < NUMLEDS; ++i)
          {
            digitalWrite(LEDs[i], HIGH);
          }
          
          gameState = GAME_OVER;
        }
        
      }
      else
      {
        playX();
        // Reset All LEDs
        for(int i = 0; i < NUMLEDS; ++i)
        {
          digitalWrite(LEDs[i], LOW);
        }
        playX();
        initializeTornadoChaser();
      }
      
    }
  }
}

bool tornadoChaser_isValidLight()
{
  for(byte i = 0; i < NUMLEDS; ++i)
  {
    if(tc_chosenLights[i] == tc_currentLightIndex)
    {
      return false;
    }
  }
  return true;
}

void initializePsyman()
{
  for(int i = 0; i < NUMLEDS; ++i)
  {
    LEDState[i] = LOW;
  }
  
  ps_isRevealing = true;
  ps_sequenceQueue[0] = random(7);
  ps_sequenceLength = 0;
  ps_currentUserGuess = 0;

  delay(1000);
  
  gameState = PLAYING;
}

void playPsyman()
{
  if(ps_isRevealing)  // Show the player the next steps
  {
    // Pick a new number
    ps_sequenceQueue[ps_sequenceLength] = random(7);
    ps_sequenceLength++;

    // Display all numbers in order
    for(byte i = 0; i < ps_sequenceLength; ++i)
    {
      digitalWrite(LEDs[ps_sequenceQueue[i]], HIGH);
      tone(SPEAKER, buttonNotes[ps_sequenceQueue[i]], 1000/32);
      //delay((1000/32)* 1.3);
    
      delay(1000);
      noTone(SPEAKER);
      digitalWrite(LEDs[ps_sequenceQueue[i]], LOW);
      delay(100);
    }

    playBeep();

    ps_isRevealing = false;
  }
  else // Wait for the player's input
  {
    for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
    {
      if(buttonPressedNow(buttonIndex))
      {
        if(buttonIndex == ps_sequenceQueue[ps_currentUserGuess])
        {
          // Light the LED and play the tone
          digitalWrite(LEDs[buttonIndex], HIGH);
          tone(SPEAKER, buttonNotes[buttonIndex], 1000/32);
          delay((1000/32)* 1.3);
          noTone(SPEAKER);
          digitalWrite(LEDs[buttonIndex], LOW);
          delay(100);
          
          if(++ps_currentUserGuess >= ps_sequenceLength)
          {

            
            if(10 <= ps_currentUserGuess)  // If the user has gotten enough right, they win!
            {
              isWinner = true;
              gameState = GAME_OVER; 
            }
            else // otherwise, continue to add another tone
            {
              ps_currentUserGuess = 0;
              delay(1000);
              ps_isRevealing = true;
            }
          }
        }
        else
        {
          isWinner = false;
          gameState = GAME_OVER; 
        }
      }
    }
  }
}

void initializeFreePlay()
{
  gameState = PLAYING;
}

void playFreePlay()
{
  for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
  {
    if(buttonPressedNow(buttonIndex))
    {
      // Light the LED and play the tone
      digitalWrite(LEDs[buttonIndex], HIGH);
      tone(SPEAKER, buttonNotes[buttonIndex], 1000/32);
      delay((1000/32)* 1.3);
      noTone(SPEAKER);
      digitalWrite(LEDs[buttonIndex], LOW);
      delay(100);
    }
  }

  if(digitalRead(switches[0]) == LOW && digitalRead(switches[NUMBUTTONS - 1]) == LOW)
  {
    gameState = GAME_OVER;
  }
}

void initializeBinaryCounting()
{
  for(int i = 0; i < NUMBUTTONS; ++i)
  {
    isDebouncing[i] = false;
    switchState[i] = HIGH;
  }
  
  for(int i = 0; i < NUMLEDS; ++i)
  {
    LEDState[i] = LOW;
  }
  
  gameState = PLAYING;
}

void playBinaryCounting()
{
  for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
  {
    if(buttonPressedNow(buttonIndex) && (binaryCounting_canToggleLight(buttonIndex)) )
    {
      LEDState[buttonIndex] = !LEDState[buttonIndex];
      digitalWrite(LEDs[buttonIndex], LEDState[buttonIndex]);
      // Kill lights to the right
      byte rightLight = buttonIndex;
      while(7 > ++rightLight)
      {
        LEDState[rightLight] = LOW;
        digitalWrite(LEDs[rightLight], LEDState[rightLight]);
      }

      
      playBeep();
      
      // Check for win
      if(PLAYING == gameState && LEDState[0] && LEDState[1]&& LEDState[2]&& LEDState[3]&& LEDState[4]&& LEDState[5]&& LEDState[6])
      {
        gameState = GAME_OVER;
      }
    }
  } 
}

bool binaryCounting_canToggleLight(byte _whichButton)
{
  // If we've run out of buttons to check, we can toggle the light!
  if(++_whichButton > 6)
    return true;

  if(!LEDState[_whichButton])
  {
    return false;
  }

  return binaryCounting_canToggleLight(_whichButton);
}

void initializeHardMode()
{
  for(int i = 0; i < NUMBUTTONS; ++i)
  {
    isDebouncing[i] = false;
    switchState[i] = HIGH;
  }
  
  for(int i = 0; i < NUMLEDS; ++i)
  {
    LEDState[i] = LOW;
  }
  
  gameState = PLAYING;
}

void playHardMode()
{
  for(byte buttonIndex = 0; buttonIndex < NUMBUTTONS; ++buttonIndex)
  {
    if(buttonPressedNow(buttonIndex) && (hardMode_canToggleLight(buttonIndex)) )
    {
      LEDState[buttonIndex] = !LEDState[buttonIndex];
      digitalWrite(LEDs[buttonIndex], LEDState[buttonIndex]);
      playBeep();
      
      // Check for win
      if(PLAYING == gameState && LEDState[0] && LEDState[1]&& LEDState[2]&& LEDState[3]&& LEDState[4]&& LEDState[5]&& LEDState[6])
      {
        gameState = GAME_OVER;
      }
    }
  } 
}

bool hardMode_canToggleLight(byte _whichButton, bool _isRecursing)
{
  // If we've run out of buttons to check, we can toggle the light!
  if(0 >= _whichButton--)
    return true;

  // This is probably a little confusing, so I'll explain:
  // Case 1: In order to toggle a light in HARD_MODE, the light directly to the left must be off.
  // When this funtion is initially called, the default value of "_isRecursing" is used (false).
  // So if this is the first call (_isRecursing == false), the light must be off.  If not (LEDState[]== true),
  // Then we cannot toggle the button (since false != true).
  // Case 2: All other lights to the left, besides the first, must be on.  When this funciton is called again,
  // (_isRecursing == true).  So if the LED is off (LEDState[] == false) then we also cannot toggle the button
  // since (true != false)
  if(_isRecursing != LEDState[_whichButton])
    return false;

  return hardMode_canToggleLight(_whichButton, true);
}

void doGameOver()
{
  if(!isWinner)
  {
    isWinner = true; // The default
    playFailureMusic();
    menu_currentLightIndex = 6;
    gameState = MENU;
    return;
  }
  
  if(0 == gameOver_resetTime)
  {
    for(int i = 0; i < NUMCRAZYLEDS; ++i)
    {
      digitalWrite(crazyLEDs[i], HIGH);
    }

    gameOver_resetTime = millis() + 10000;
    playVictoryMusic();
  }
  else if(millis() > gameOver_resetTime)
  {
    gameOver_resetTime = 0;

    for(int i = 0; i < NUMLEDS; ++i)
    {
      digitalWrite(LEDs[i], LOW);
    }
  
    for(int i = 0; i < NUMCRAZYLEDS; ++i)
    {
      digitalWrite(crazyLEDs[i], LOW);
    }

    menu_currentLightIndex = 6;
    gameState = MENU;
  }
}
