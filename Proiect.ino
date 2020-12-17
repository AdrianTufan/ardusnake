
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display size in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Analog Joystick
#define joystickX A0
#define joystickY A1
#define SW 2

#define SNAKE_MAX_LENGTH 10000

char grid[SCREEN_WIDTH][SCREEN_HEIGHT];

typedef struct GridSegment
{
  int x;
  int y;
  
  GridSegment(int x, int y)
  {
    this->x = x;
    this->y = y;
  }

  GridSegment()
  {
    this->x = -1;
    this->y = -1;
  }
}GridSegment;

typedef struct SnakeSegment
{
  char x;
  char y;
  
  SnakeSegment(char x, char y)
  {
    this->x = x;
    this->y = y;
  }

  SnakeSegment()
  {
    this->x = -1;
    this->y = -1;
  }
  
}SnakeSegment;

typedef struct Snake
{
  SnakeSegment tail[SNAKE_MAX_LENGTH];
  int sLength;
  int sDirection;

  void tailFollow()
  {
    for(int i = sLength; i >= 1; --i)
    {
      tail[i] = tail[i - 1];
    }
  }
  
  void moveUp()
  {
    tailFollow();
    tail[0] = {tail[0].x, tail[0].y - 1};
  }

  void moveDown()
  {
    tailFollow();
    tail[0] = {tail[0].x, tail[0].y + 1};
  }

  void moveLeft()
  {
    tailFollow();
    tail[0] = {tail[0].x - 1, tail[0].y};
  }

  void moveRight()
  {
    tailFollow();
    tail[0] = {tail[0].x + 1, tail[0].y};
  }
  
}Snake;

typedef struct Fruit
{
  char x;
  char y;
  bool spawned;
}Fruit;

typedef struct SnakeAgent
{
    Snake* snake;
    GridSegment sGoal;
    Fruit* fruit;

    SnakeAgent()
    {
      snake = 0;
      sGoal = GridSegment(-1, -1);
      fruit = 0;
    }
  
    SnakeAgent(Snake* snake, Fruit* fruit)
    {
      this->snake = snake;
      sGoal = GridSegment(fruit->x, fruit->y);
      this->fruit = fruit;
    }
    
    void moveTowardsGoal()
    {
      SnakeSegment snakeHead = snake->tail[0];
      Serial.println(snake->sDirection);
      Serial.print(snakeHead.x);
      Serial.print("\t");
      Serial.println(snakeHead.y);
      Serial.print(sGoal.x);
      Serial.print("\t");
      Serial.println(sGoal.y);
      // If snake is currently moving up

      if (!fruit->spawned) return;
      if(fruit->x == -1 && fruit->y == -1)  return;
      
      switch(snake->sDirection)
      {
        case 0:
          if (sGoal.y < snakeHead.y)
          {
            // snake->sDirection = 0;
            snake->moveUp();
            break;
          }
          if(sGoal.x < snakeHead.x)
          {
            snake->moveLeft();
            snake->sDirection = 2;
            break;
          }
          if(sGoal.x > snakeHead.x)
          {
            snake->moveRight();
            snake->sDirection = 3;
            break;
          }
          break;
        case 1:
          if (sGoal.y > snakeHead.y)
          {
            // snake->sDirection = 1;
            snake->moveDown();
            break;
          }
          if(sGoal.x < snakeHead.x)
          {
            snake->moveLeft();
            snake->sDirection = 2;
            break;
          }
          if(sGoal.x > snakeHead.x)
          {
            snake->moveRight();
            snake->sDirection = 3;
            break;
          }
          break;
        case 2:
          if(sGoal.x < snakeHead.x)
          {
            // snake->sDirection = 2;
            snake->moveLeft();
            break;
          }
          if (sGoal.y > snakeHead.y)
          {
             snake->moveDown();
             snake->sDirection = 1;
             break;
          }
          if(sGoal.y < snakeHead.y)
          {
            snake->moveUp();
            snake->sDirection = 0;
            break;
          }
          break;
        case 3:
          if(sGoal.x > snakeHead.x)
          {
            // snake->sDirection = 3;
            snake->moveRight();
            break;
          }
          if (sGoal.y < snakeHead.y)
          {
             snake->moveUp();
             snake->sDirection = 0;
             break;
          }
          if (sGoal.y > snakeHead.y)
          {
            snake->moveDown();
            snake->sDirection = 1;
            break;
          }
          break;
      }
      snake->tail[snake->sLength] = {-1, -1};
      return;
    }
}SnakeAgent;

 

 Snake snake;
 Fruit fruit;
 SnakeAgent snakeAgent;

 
 int xValue;
 int yValue;
 int pxValue;
 int pyValue;
 int epsilon;


void setup()
{
  pinMode(SW, INPUT);
  digitalWrite(SW, HIGH);
  Serial.begin(9600);

  randomSeed(analogRead(0));

  if(!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  for(int i = 0; i < 10; ++i)
  {
    SnakeSegment snakeSegment = {64 + i, 32};
    snake.tail[i] = snakeSegment;
  }

  for(int i = 10; i < SNAKE_MAX_LENGTH; ++i)
  {
    SnakeSegment snakeSegment = {-1, -1};
    snake.tail[i] = snakeSegment;
  }

  snake.sLength = 10;
  snake.sDirection = 3;

  fruit.x = -1;
  fruit.y = -1;
  fruit.spawned = false;
  
  epsilon = 20;

  snakeAgent.snake = &snake;
  snakeAgent.sGoal = GridSegment(fruit.x, fruit.y);
  snakeAgent.fruit = &fruit;
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
  
  snake.tail[snake.sLength] = {-1, -1};
}

void addSnakeSegment(SnakeSegment* tail, int* sLength, int* sDirection, int amount)
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
    fruit = Fruit{-1 , -1};
    addSnakeSegment(snake.tail, &snake.sLength, &snake.sDirection, 5);
  }
}

void loop()
{
  drawSnake();
  //moveSnake();
  snakeAgent.moveTowardsGoal();
  spawnFruit();
  checkFruitCollision();
  drawFruit();
}
