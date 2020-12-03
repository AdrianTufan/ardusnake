
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

#define SNAKE_MAX_LENGTH 40

typedef struct SnakeSegment
{
  int x;
  int y;
}SnakeSegment;

typedef struct Snake
{
  SnakeSegment tail[SNAKE_MAX_LENGTH];
  int sLength;
  int sDirection;
}Snake;

typedef struct Fruit
{
  int x;
  int y;
  bool spawned;
}Fruit;

 Snake snake;
 Fruit fruit;
 
 int xValue;
 int yValue;
 int pxValue;
 int pyValue;
 int epsilon;


void setup()
{
  Serial.begin(9600);

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
}

void draw_snake()
{
  display.display();
  delay(10);
  display.clearDisplay();

  for(int i = 0; i < snake.sLength; ++i)
  {
    display.drawPixel(snake.tail[i].x, snake.tail[i].y, SSD1306_WHITE);
  }
  display.display();

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

  for(int i = snake.sLength; i >= 1; --i)
  {
    snake.tail[i] = snake.tail[i - 1];
  }
    
  switch(snake.sDirection)
  {
    // Up
    case 0:
      snake.tail[0] = {snake.tail[0].x, snake.tail[0].y - 1};
      break;
        
    // Down
    case 1: 
      snake.tail[0] = {snake.tail[0].x, snake.tail[0].y + 1};
      break;
        
    // Left
    case 2:
      snake.tail[0] = {snake.tail[0].x - 1, snake.tail[0].y};
      break;
        
    // Right
    case 3:
      snake.tail[0] = {snake.tail[0].x + 1, snake.tail[0].y};
      break;
  }
  
  snake.tail[snake.sLength] = {-1, -1};

  delay(5);
}

void spawn_fruit()
{
  
  if(!fruit.spawned)
  {
    do
    {
      fruit.x = random() % SCREEN_WIDTH - 1;
      fruit.y = random() & SCREEN_HEIGHT - 1;
    }while(fruit.x == snake.tail[0].x && fruit.y == snake.tail[0].y);
  }
  display.drawPixel(fruit.x, fruit.y, SSD1306_WHITE);
  display.display();

  if(!fruit.spawned) fruit.spawned = true;
  
  delay(10);
}

void loop()
{
  draw_snake();
  spawn_fruit();
  
  delay(10);
}
