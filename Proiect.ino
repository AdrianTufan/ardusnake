

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display size in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED Pins Definition
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

// OLED initialisation using the Adafruit library
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// LCD
#include <LiquidCrystal.h>
const int RS = 12, EN = 8, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Analog Joystick
#define joystickX A0
#define joystickY A1
#define SW 7

// Lungimea maxima a snake-ului
#define SNAKE_MAX_LENGTH 1000
// Dimenisunea initiala a snake-ului cat si numarul de segmente adaugat la coada daca snake-ul reuseste sa manance fructul
#define SNAKE_INCREMENT_VALUE 3
// Scorul necesar pentru a castiga jocul
#define GAME_WON_SCORE 5

// Structura folosita pentru definirea pozitiilor de pe grid-ul de dimensiune 128 x 64 / Dimensiunea OLED-ului
typedef struct GridSegment
{
  byte x;
  byte y;
  
  GridSegment(byte x, byte y)
  {
    this->x = x;
    this->y = y;
  }

  GridSegment()
  {
    this->x = 255;
    this->y = 255;
  }
}GridSegment;

// Structura echivalenta cu GridSegment
// folosita pentru definirea capului si cozii snake-ului
typedef struct SnakeSegment
{
  byte x;
  byte y;
  
  SnakeSegment(byte x, byte y)
  {
    this->x = x;
    this->y = y;
  }

  SnakeSegment()
  {
    this->x = 255;
    this->y = 255;
  }
  
}SnakeSegment;

// Structura folosita pentru definirea snake-ului
typedef struct Snake
{
  // Coada snake-ului, unde tail[0] reprezinta capul
  SnakeSegment tail[SNAKE_MAX_LENGTH];
  // Lungimea curenta a snake-ului
  int sLength;
  // Directia de deplasare a snake-ului
  // Aceasta poate fi 0 pentru Nord, 1 pentru Sud, 2 pentru Vest si 3 pentru Est
  byte sDirection;
  // Scorul curent
  byte sScore;

  // Functie folosita pentru a shift-a coada snake-ului
  // in momentul in care sunt adaugate segmente noi
  void tailFollow()
  {
    for(int i = sLength; i >= 1; --i)
    {
      tail[i] = tail[i - 1];
    }
  }

  // Functie de deplasare spre Nord
  void moveUp()
  {
    tailFollow();
    tail[0] = {tail[0].x, tail[0].y - 1};
  }

  // Functie de deplasare spre Sud
  void moveDown()
  {
    tailFollow();
    tail[0] = {tail[0].x, tail[0].y + 1};
  }

  // Functie de deplasare spre Vest
  void moveLeft()
  {
    tailFollow();
    tail[0] = {tail[0].x - 1, tail[0].y};
  }

  // Functie de deplasare spre Est
  void moveRight()
  {
    tailFollow();
    tail[0] = {tail[0].x + 1, tail[0].y};
  }
  
}Snake;

// Structura folosita pentru definirea frructului
typedef struct Fruit
{
  // Fructul este definit de pozitia sa pre grid, avand 2 componente: pozitia pe axa x si pe axa y
  byte x;
  byte y;
  // Flag folosit pentru a stii daca fructul a fost generat sau nu
  bool spawned;
}Fruit;

// Structura folosita pentru agentul inteligent
// Aceasta este folosita pentru a proiecta 3 raze din capul snake-ului
// pentru a obtine informatii despre mediul inconjurator
typedef struct Rayscasts
{
  // Flag-uri care ne spun daca exista obiecte in calea snake-ului in cele 4 directii 
  bool N;
  bool S;
  bool W;
  bool E;
  // Daca acele obiecte exista, putem obtine si distana pana la ele
  int distanceToN;
  int distanceToS;
  int distanceToE;
  int distanceToW;
  // Lungimea default a uneia dintre raze
  int rayLength = 20;
  // Aceasta variabila este folosita pentru a-i spune snake-ului
  // carora obiecte sa le dea importanta
  // De exemplu daca un obiect se afla in Nord la distanta de 15 iar snake-ul se deplaseaza tot inspre Nord, snake-ul si-ar continua calea
  // In schimba daca acea distanta ar fi sub limita acceptata, snake-ul ar trebui sa-si schimbe directia pentru a evita colizunea cu acel obiect
  int rayThreshold = 5;
}Raycasts;

// Agent inteligent folosit pentru controlarea snake-ului
typedef struct SnakeAgent
{ 
    Snake* snake;
    // Pozitia goal-ului pe grid, in acest caz pozitia fructului
    GridSegment sGoal;
    Fruit* fruit;
    Raycasts rays;

    SnakeAgent()
    {
      snake = 0;
      sGoal = GridSegment(255, 255);
      fruit = 0;
    }
  
    SnakeAgent(Snake* snake, Fruit* fruit)
    {
      this->snake = snake;
      sGoal = GridSegment(fruit->x, fruit->y);
      this->fruit = fruit;
    }

    // Functie folosita pentru a seta flag-urile si distantele din structura Raycasts
    void checkForPossibleTailCollision()
    {
      SnakeSegment snakeHead = snake->tail[0];
      SnakeSegment tailSegment;
    
      rays.N = false;
      rays.S = false;
      rays.E = false;
      rays.W = false;
      
      rays.distanceToN = SCREEN_HEIGHT;
      rays.distanceToS = SCREEN_HEIGHT;
      rays.distanceToE = SCREEN_WIDTH;
      rays.distanceToW = SCREEN_WIDTH;
      
      switch(snake->sDirection)
      {
        // Daca snake-ul are directia de deplasare Nord, avem nevoie sa proiectam 3 raze in directiile Nord, Vest si Est
        case 0:
          for(int i = 1; i < snake->sLength; ++i)
          {
            for(int j = 1; j <= rays.rayLength; ++j)
            {
              if(snakeHead.y - j == snake->tail[i].y && snakeHead.x == snake->tail[i].x)
              {
                rays.N = true;
                rays.distanceToN = j;
              }
              if(snakeHead.x - j == snake->tail[i].x && snakeHead.y == snake->tail[i].y)
              {
                rays.W = true;
                rays.distanceToW = j;
              }
              if(snakeHead.x + j == snake->tail[i].x && snakeHead.y == snake->tail[i].y)
              {
                rays.E = true;
                rays.distanceToE = j;
              }
            }
          }

          // In cazul in care nu exista obiecte in caiile celor 3 raze, verificam daca totusi nu ne aflam pe marginea grid-ului
          if(rays.N == false)
            if(snakeHead.y - rays.rayLength <= 0)
            {
              rays.N = true;
              rays.distanceToN = snakeHead.y;
            }

          if(rays.W == false)
            if(SCREEN_WIDTH - rays.rayLength <= snakeHead.x)
            {
              rays.W = true;
              rays.distanceToN = SCREEN_WIDTH - snakeHead.x;
            }

          if(rays.E == false)
            if(SCREEN_WIDTH - rays.rayLength <= snakeHead.x)
            {
              rays.E = true;
              rays.distanceToN = SCREEN_WIDTH - snakeHead.x;
            }
          
          break;
       // Daca snake-ul are directia de deplasare Nord, avem nevoie sa proiectam 3 raze in directiile Sud, Vest si Est
        case 1:
          for(int i = 1; i < snake->sLength; ++i)
          {
            for(int j = 1; j <= rays.rayLength; ++j)
            {
              if(snakeHead.y + j == snake->tail[i].y && snakeHead.x == snake->tail[i].x)
              {
                rays.S = true;
                rays.distanceToS = j;
              }
              if(snakeHead.x - j == snake->tail[i].x && snakeHead.y == snake->tail[i].y) 
              {
                rays.W = true;
                rays.distanceToW = j;
              }
              if(snakeHead.x + j == snake->tail[i].x && snakeHead.y == snake->tail[i].y)
              {
                rays.E = true;
                rays.distanceToE = j;
              }
            }
          }

          if(rays.S == false)
            if(SCREEN_HEIGHT - rays.rayLength <= snakeHead.y)
            {
              rays.S = true;
              rays.distanceToN = SCREEN_HEIGHT - snakeHead.y;
            }

          if(rays.W == false)
            if(snakeHead.x - rays.rayLength <= 0)
            {
              rays.W = true;
              rays.distanceToN = snakeHead.x;
            }

          if(rays.E == false)
            if(SCREEN_WIDTH - rays.rayLength <= snakeHead.x)
            {
              rays.E = true;
              rays.distanceToN = SCREEN_WIDTH - snakeHead.x;
            }
          
        break;
        // Daca snake-ul are directia de deplasare Nord, avem nevoie sa proiectam 3 raze in directiile Nord, Sud si Vest
        case 2:
          for(int i = 1; i < snake->sLength; ++i)
          {
            for(int j = 1; j <= rays.rayLength; ++j)
            {
              if(snakeHead.y - j == snake->tail[i].y && snakeHead.x == snake->tail[i].x)
              {
                rays.N = true;
                rays.distanceToN = j;
              }
              if(snakeHead.y + j == snake->tail[i].y && snakeHead.x == snake->tail[i].x)
              {
                rays.S = true;
                rays.distanceToS = j;
              }
              if(snakeHead.x - j == snake->tail[i].x && snakeHead.y == snake->tail[i].y)
              {
                rays.W = true;
                rays.distanceToW = j;
              }
            }
          }

          if(rays.S == false)
            if(SCREEN_HEIGHT - rays.rayLength <= snakeHead.y)
            {
              rays.S = true;
              rays.distanceToN = SCREEN_HEIGHT - snakeHead.y;
            }

          if(rays.N == false)
            if(snakeHead.y - rays.rayLength <= 0)
            {
              rays.N = true;
              rays.distanceToN = snakeHead.y;
            }

          if(rays.W == false)
            if(snakeHead.x - rays.rayLength <= 0)
            {
              rays.W = true;
              rays.distanceToN = snakeHead.x;
            }
          
        break;
        // Daca snake-ul are directia de deplasare Nord, avem nevoie sa proiectam 3 raze in directiile Nord, Sud si Est
        case 3:
          for(int i = 1; i < snake->sLength; ++i)
          {
            for(int j = 1; j <= rays.rayLength; ++j)
            {
              if(snakeHead.y - j == snake->tail[i].y && snakeHead.x == snake->tail[i].x)
              {
                rays.N = true;
                rays.distanceToN = j;
              }
              if(snakeHead.y + j == snake->tail[i].y && snakeHead.x == snake->tail[i].x)
              {
                rays.S = true;
                rays.distanceToS = j;
              }
              if(snakeHead.x + j == snake->tail[i].x && snakeHead.y == snake->tail[i].y)
              {
                rays.E = true;
                rays.distanceToE = j;
              }
            }
          }

          if(rays.S == false)
            if(SCREEN_HEIGHT - rays.rayLength <= snakeHead.y)
            {
              rays.S = true;
              rays.distanceToN = SCREEN_HEIGHT - snakeHead.y;
            }

          if(rays.N == false)
            if(snakeHead.y - rays.rayLength <= 0)
            {
              rays.N = true;
              rays.distanceToN = snakeHead.y;
            }

          if(rays.E == false)
            if(SCREEN_WIDTH - rays.rayLength <= snakeHead.x)
            {
              rays.E = true;
              rays.distanceToN = SCREEN_WIDTH - snakeHead.x;
            }
          
        break;
      }
    }

    // Functia folosita pentru deplasarea snake-ului catre goal
    void moveTowardsGoal()
    {
      // Proiectam razele pentru a obtine informatiile necesare
      checkForPossibleTailCollision();
      SnakeSegment snakeHead = snake->tail[0];

      // Daca fructul inca nu a fost generat, aceasta functie nu trebuie sa mai faca nimic
      if (!fruit->spawned) return;
      // Daca fructul a fost generat inafara grid-ului, iesim din functie pana cand problema este remediata
      if(fruit->x == -1 && fruit->y == -1)  return;

      // Verificam directia curenta de deplasare a snake-ului cat si pozitia curenta a frucutlui pe grid
      // Pe baza acestor informatii cat si pe baza informatiilor obtinute din proiectarea razelor
      // aalegem calea "cea mai buna" pentru a atinge goal-ul
      switch(snake->sDirection)
      {
        case 0:
          if (sGoal.y < snakeHead.y && (rays.distanceToN > rays.rayThreshold || rays.N == false))
          {
              snake->moveUp();
              break;
          }
          else if(sGoal.x <= snakeHead.x && (rays.distanceToW > rays.rayThreshold || rays.W == false))
          {
              snake->moveLeft();
              snake->sDirection = 2;
              break;
          }
          else if(sGoal.x >= snakeHead.x && (rays.distanceToE > rays.rayThreshold || rays.E == false))
          {
              snake->moveRight();
              snake->sDirection = 3;
              break;
          }
          else
          {
            snake->moveUp();
            break;
          }
        case 1:
          if (sGoal.y > snakeHead.y && (rays.distanceToS > rays.rayThreshold || rays.S == false))
          {
              snake->moveDown();
              break;
          }
          else if(sGoal.x <= snakeHead.x && (rays.distanceToW > rays.rayThreshold || rays.W == false))
          {
              snake->moveLeft();
              snake->sDirection = 2;
              break;
          }
          else if(sGoal.x >= snakeHead.x && (rays.distanceToE > rays.rayThreshold || rays.E == false))
          {
              snake->moveRight();
              snake->sDirection = 3;
              break;
          }
          else
          {
            snake->moveDown();
            break;
          }
          break;
        case 2:
          if(sGoal.x < snakeHead.x && (rays.distanceToW > rays.rayThreshold || rays.W == false))
          {
              snake->moveLeft();
              break;
          }
          else if(sGoal.y >= snakeHead.y && (rays.distanceToS > rays.rayThreshold || rays.S == false))
          {
             snake->moveDown();
             snake->sDirection = 1;
             break;
          }
          else if(sGoal.y <= snakeHead.y && (rays.distanceToN > rays.rayThreshold || rays.N == false))
          {
              snake->moveUp();
              snake->sDirection = 0;
              break;
          }
          else
          {
            snake->moveLeft();
            break;
          }
          break;
        case 3:
          if(sGoal.x > snakeHead.x && (rays.distanceToE > rays.rayThreshold || rays.E == false))
          {
            snake->moveRight();
            break;
          }
          else if(sGoal.y <= snakeHead.y && (rays.distanceToN > rays.rayThreshold || rays.N == false))
          {
             snake->moveUp();
             snake->sDirection = 0;
             break;
          }
          else if(sGoal.y >= snakeHead.y && (rays.distanceToS > rays.rayThreshold || rays.S == false))
          {
            snake->moveDown();
            snake->sDirection = 1;
            break;
          }
          else
          {
            snake->moveRight();
            break;
          }
          break;
      }
      snake->tail[snake->sLength] = {-1, -1};
      return;
    }
}SnakeAgent;

 // Snake-ul, fructul si agentul inteligent 
 Snake snake;
 Fruit fruit;
 SnakeAgent snakeAgent;


 int xValue;
 int yValue;
 int pxValue;
 int pyValue;
 int epsilon;
 bool isAi = false;

 double timePassed;
 double timeStart;

void setup()
{
  pinMode(joystickX, INPUT);
  pinMode(joystickY, INPUT);
  pinMode(SW, INPUT_PULLUP);
  digitalWrite(SW, HIGH);

  Serial.begin(9600);
  lcd.begin(16, 2);
  randomSeed(analogRead(0));

  if(!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);  
  }

  startGameScreen();
}

void initGame()
{
  lcd.print("Score: ");
  lcd.setCursor(0, 1);
  lcd.print("User Mode");

  timeStart = 0.0;
  
  for(int i = 0; i < SNAKE_INCREMENT_VALUE; ++i)
  {
    SnakeSegment snakeSegment = {64 + i, 32};
    snake.tail[i] = snakeSegment;
  }

  for(int i = SNAKE_INCREMENT_VALUE; i < SNAKE_MAX_LENGTH; ++i)
  {
    SnakeSegment snakeSegment = {-1, -1};
    snake.tail[i] = snakeSegment;
  }

  snake.sLength = SNAKE_INCREMENT_VALUE;
  snake.sDirection = 3;
  snake.sScore = 0;

  fruit.x = 255;
  fruit.y = 255;
  fruit.spawned = false;
  
  epsilon = 200;

  snakeAgent.snake = &snake;
  snakeAgent.sGoal = GridSegment(fruit.x, fruit.y);
  snakeAgent.fruit = &fruit;

  updateLcd();
}

void drawSnake()
{
  display.display();

  display.clearDisplay();

  for(int i = 0; i < snake.sLength; ++i)
  {
    display.drawPixel(snake.tail[i].x, snake.tail[i].y, SSD1306_WHITE);
  }
  
  display.display();
}

void moveSnake()
{
  xValue = analogRead(joystickX) - 518;
  yValue = analogRead(joystickY) - 524;

  if (xValue < 0 - epsilon)
  {
    snake.sDirection = 0;
  }
  if (xValue > 0 + epsilon)
  {
    snake.sDirection = 1;
  }
  
  if (yValue > 0 + epsilon)
  {
    snake.sDirection = 2;
  }
    
  if (yValue < 0 - epsilon)
  {
    snake.sDirection = 3;
  }
    
  switch(snake.sDirection)
  {
    // Up
    case 0:
      snake.moveUp();
      break;
        
    // Down
    case 1: 
      snake.moveDown();
      break;
        
    // Left
    case 2:
      snake.moveLeft();
      break;
        
    // Right
    case 3:
      snake.moveRight();
      break;
  }
  
  snake.tail[snake.sLength] = {255, 255};
}

void addSnakeSegment(SnakeSegment* tail, int* sLength, byte * sDirection, int amount)
{
  switch(*sDirection)
  {
    // Up
    case 0:
      for(int i = 0; i < amount; ++i)
      {
        tail[*sLength + i] = SnakeSegment(tail[*sLength - (i + 1)].x, tail[*sLength - (i + 1)].y - 1);
        *sLength = *sLength + 1;
      }
      break;
        
    // Down
    case 1: 
      for(int i = 0; i < amount; ++i)
      {
        tail[*sLength + i] = SnakeSegment(tail[*sLength - (i + 1)].x, tail[*sLength - (i + 1)].y + 1);
        *sLength = *sLength + 1;
      }
      break;
        
    // Left
    case 2:
      for(int i = 0; i < amount; ++i)
      {
        tail[*sLength + i] = SnakeSegment(tail[*sLength - (i + 1)].x - 1, tail[*sLength - (i + 1)].y);
        *sLength = *sLength + 1;
      }
      break;
        
    // Right
    case 3:
      for(int i = 0; i < amount; ++i)
      {
        tail[*sLength + i] = SnakeSegment(tail[*sLength - (i + 1)].x + 1, tail[*sLength - (i + 1)].y);
        *sLength = *sLength + 1;
      }
      break;
  }
}

void drawFruit()
{
  display.drawPixel(fruit.x, fruit.y, SSD1306_WHITE);
  display.display();
}

void spawnFruit()
{
  if (fruit.spawned)  return;

  // Generate fruit's new position
  do
  {
    fruit.x = random() % SCREEN_WIDTH - 1;
    fruit.y = random() & SCREEN_HEIGHT - 1;
  }while(fruit.x == snake.tail[0].x && fruit.y == snake.tail[0].y);

  snakeAgent.sGoal = GridSegment(fruit.x, fruit.y);

  // Signal the fruit has spawned
  fruit.spawned = true;
}

void checkFruitCollision()
{
  if(snake.tail[0].x == fruit.x && snake.tail[0].y == fruit.y)
  {
    fruit.spawned = false;
    fruit = Fruit{255, 255};
    addSnakeSegment(snake.tail, &snake.sLength, &snake.sDirection, SNAKE_INCREMENT_VALUE);
    snake.sScore += 1;
    updateLcd();
  }
}

void displayScore()
{
  lcd.print("Score: ");
  lcd.setCursor(7, 0);
  lcd.print(snake.sScore);
}

void displayPlayerMode()
{
  lcd.setCursor(0, 1);
  if(isAi)
  {
    lcd.print("AI Mode");
  }
  else
    {
      lcd.print("User Mode");;
    }
}

void updateLcd()
{
  lcd.clear();
  displayScore();
  displayPlayerMode();
}

void changeToAi()
{
  if(timePassed - timeStart < 1000) return;
  timeStart = timePassed;
  double change = digitalRead(SW);
  if(change < 1) isAi = !isAi;
  updateLcd();
}

bool collisionDetection()
{
  // Check snake collision with the screen borders
  SnakeSegment head = snake.tail[0];
  if(head.x < 0 || head.x > 127)
  {
    return true;
  }
  if(head.y < 0 || head.y > 63)
  {
    return true;
  }

  // Verifica coliziunea snake-ului cu coada sa
  if (snake.sLength == SNAKE_INCREMENT_VALUE)  return false;
  SnakeSegment tailSegment;
  for(int i = 1; i < snake.sLength; ++i)
  {
    tailSegment = snake.tail[i];
    if(head.x == tailSegment.x && head.y == tailSegment.y)
    {
      return true;
    }
  }
  return false;
}

void startGameScreen()
{
    lcd.print("Press SW");
    lcd.setCursor(0, 1);
    lcd.print("to start!");
    while(digitalRead(SW) == 1);
  initGame();
}

void gameOverScreen()
{
  lcd.setCursor(0, 1);
  lcd.print("GAME OVER!");
  delay(3000);
  
  int startGame;
  do
  {
    lcd.clear();
    lcd.print("Press SW");
    lcd.setCursor(0, 1);
    lcd.print("to continue!");
  
    startGame = digitalRead(SW);
    if(startGame == 0) setup();
    delay(100);
  }while(startGame == 1);
}

void gameWonScreen()
{
  lcd.setCursor(0, 1);
  lcd.print("GAME WON!");
  delay(3000);
  
  int startGame;
  do
  {
    lcd.clear();
    lcd.print("Press SW");
    lcd.setCursor(0, 1);
    lcd.print("to continue!");
  
    startGame = digitalRead(SW);
    if(startGame == 0) setup();
    delay(100);
  }while(startGame == 1);
}

void checkIfGameWon()
{
  if(snake.sScore == GAME_WON_SCORE)
  {
    gameWonScreen();
  }
}

void loop()
{
  Serial.print(snake.sDirection);
  Serial.print(" ");
  Serial.println(snake.sScore);
  
  timePassed = millis();
  if(collisionDetection())  gameOverScreen();
  checkIfGameWon();
  changeToAi();
  
  drawSnake();
  
  if (!isAi)
    moveSnake();
  else
    snakeAgent.moveTowardsGoal();
    
  spawnFruit();
  checkFruitCollision();
  drawFruit();
} 
