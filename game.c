#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_stdinc.h>
#include <stdbool.h>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define PLAYERSPEED 100
#define PLAYERROTATIONSPEED 5

#define PLAYERSIZE 10
#define TRIANGLEANGLE (SDL_PI_D - SDL_atan(0.5))
#define TRIANGLEMULTIPLYER (SDL_sqrt(5)/2)

#define MAXBULLETS 100

#define BULLETSIZE 3
#define BULLETSPEED 150

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

typedef struct {
  SDL_FPoint position;
  double direction;
  bool isDead;
} Player;

typedef struct {
  SDL_FPoint position;
  float xVelocity;
  float yVelocity;
  bool isPlayer1;
} Bullet;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  Player player1;
  Player player2;
  int bulletCount;
  Bullet bullets[MAXBULLETS];
  unsigned char arrows;
  unsigned char wasd;
  bool gameOver;
} AppState;

/* We will use this renderer to draw into this window every frame. */

bool isOutside(SDL_FPoint point){
  return point.x < 0 || point.x > WINDOW_WIDTH || point.y < 0 || point.y > WINDOW_HEIGHT;
}

void shoot(Player player, Bullet *bullets, int *bulletCount, bool isPlayer1){
  *bulletCount += 1;
  bullets[*bulletCount-1].position = player.position;
  bullets[*bulletCount-1].xVelocity = BULLETSPEED * SDL_cos(player.direction);
  bullets[*bulletCount-1].yVelocity = -BULLETSPEED * SDL_sin(player.direction);
  bullets[*bulletCount-1].isPlayer1 = isPlayer1;
}

void drawCircle(SDL_Renderer *renderer, float x, float y, float radius, int red, int green, int blue)
{
    SDL_SetRenderDrawColor(renderer, red, green, blue, SDL_ALPHA_OPAQUE);
    for (int w = 0; w <= radius * 2; w++)
    {
        for (int h = 0; h <= radius * 2; h++)
        {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx*dx + dy*dy) <= (radius * radius))
            {
                SDL_RenderPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

void drawPlayer(SDL_Renderer *renderer,SDL_FPoint position, double direction, float red, float green, float blue){
  SDL_Vertex verticies[3];

  verticies[0].position.x = PLAYERSIZE * SDL_cos(direction) + position.x;
  verticies[0].position.y = -PLAYERSIZE * SDL_sin(direction) + position.y;
  verticies[0].color.r = red;
  verticies[0].color.g = green;
  verticies[0].color.b = blue;
  verticies[0].color.a = 1.0;

  verticies[1].position.x = PLAYERSIZE * TRIANGLEMULTIPLYER * SDL_cos(direction + TRIANGLEANGLE) + position.x;
  verticies[1].position.y = -PLAYERSIZE * TRIANGLEMULTIPLYER * SDL_sin(direction + TRIANGLEANGLE) + position.y;
  verticies[1].color.r = red;
  verticies[1].color.g = green;
  verticies[1].color.b = blue;
  verticies[1].color.a = 1.0;

  verticies[2].position.x = PLAYERSIZE * TRIANGLEMULTIPLYER * SDL_cos(direction - TRIANGLEANGLE) + position.x;
  verticies[2].position.y = -PLAYERSIZE * TRIANGLEMULTIPLYER * SDL_sin(direction - TRIANGLEANGLE) + position.y;
  verticies[2].color.r = red;
  verticies[2].color.g = green;
  verticies[2].color.b = blue;
  verticies[2].color.a = 1.0;

  SDL_RenderGeometry(renderer, NULL, verticies, 3, NULL, 0);
}

void removeBullet(Bullet *bullets, int bulletIndex, int *bulletCount){
  for (int i = bulletIndex; i < MAXBULLETS-1; i++) {
    bullets[i] = bullets[i+1];
  }
  SDL_zero(bullets[MAXBULLETS-1]);
  *bulletCount -= 1;
}

void updateBullet(Bullet *bullets, int bulletIndex, int *bulletCount, Uint64 deltaTimeNS){
  double deltaTime = (double)deltaTimeNS * 1e-9;
  Bullet *bullet = &bullets[bulletIndex];
  bullet->position.x += bullet->xVelocity * deltaTime;
  bullet->position.y += bullet->yVelocity * deltaTime;
  if (isOutside(bullet->position)){
    removeBullet(bullets,bulletIndex,bulletCount);
  }
}

float squareDistance(SDL_FPoint point1, SDL_FPoint point2){
  float dx = point2.x - point1.x;
  float dy = point2.y - point1.y;
  return dx*dx+dy*dy;
}

bool bulletTouchingPlayer(SDL_FPoint bulletPosition, SDL_FPoint playerPosition){
  return squareDistance(bulletPosition, playerPosition) < (BULLETSIZE + PLAYERSIZE)*(BULLETSIZE + PLAYERSIZE);
}

void checkCollisions(Player *player1, Player *player2, Bullet bullets[], int bulletCount, bool *gameOver){
  for (int i = 0; i < bulletCount; i++) {
    if (bullets[i].isPlayer1 && bulletTouchingPlayer(bullets[i].position, player2->position)){
      player2-> isDead = true;
      *gameOver = true;
    }
    if (!bullets[i].isPlayer1 && bulletTouchingPlayer(bullets[i].position, player1->position)){
      player1-> isDead = true;
      *gameOver = true;
    }
  }
}

void update(Player *player1, Player *player2, Bullet bullets[], int *bulletCount, unsigned char arrows, unsigned char wasd, bool *gameOver, Uint64 deltaTimeNS){
  double deltaTime = (double)deltaTimeNS * 1e-9;

  checkCollisions(player1, player2, bullets, *bulletCount, gameOver);

  double horizontal1 = (wasd & 8 ? 1.0 : 0.0) - (wasd & 2 ? 1.0 : 0.0);
  double vertical1 = (wasd & 1 ? 1.0 : 0.0) - (wasd & 4 ? 1.0 : 0.0);
  double horizontal2 = (arrows & 8 ? 1.0 : 0.0) - (arrows & 2 ? 1.0 : 0.0);
  double vertical2 = (arrows & 1 ? 1.0 : 0.0) - (arrows & 4 ? 1.0 : 0.0);

  if (!*gameOver){
    player1->position.x += PLAYERSPEED * SDL_cos(player1->direction) * vertical1 * deltaTime;
    player1->position.y -= PLAYERSPEED * SDL_sin(player1->direction) * vertical1 * deltaTime;
    player1->direction -= PLAYERROTATIONSPEED * horizontal1 * deltaTime;
    player2->position.x += PLAYERSPEED * SDL_cos(player2->direction) * vertical2 * deltaTime;
    player2->position.y -= PLAYERSPEED * SDL_sin(player2->direction) * vertical2 * deltaTime;
    player2->direction -= PLAYERROTATIONSPEED * horizontal2 * deltaTime;
  }

  for (int i = 0; i < *bulletCount; i++) {
      updateBullet(bullets, i, bulletCount, deltaTimeNS);
  }
}

void render(SDL_Renderer *renderer, Player player1, Player player2, Bullet *bullets, int bulletCount){
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  if (!player1.isDead){
    drawPlayer(renderer, player1.position, player1.direction, 0.0, 0.0, 1.0);
  }
  if (!player2.isDead){
    drawPlayer(renderer, player2.position, player2.direction, 1.0, 0.0, 0.0);
  }
  for (int i = 0; i < bulletCount; i++) {
    drawCircle(renderer, bullets[i].position.x, bullets[i].position.y, BULLETSIZE, (bullets[i].isPlayer1 ? 0 : 255), 0, (bullets[i].isPlayer1 ? 255 : 0));
  }

  SDL_RenderPresent(renderer);
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  SDL_SetAppMetadata("dungeon game", "1.0", "com.example.dungeon-game");
  AppState *as = SDL_calloc(1, sizeof(AppState));

  if (!as){
    return SDL_APP_FAILURE;
  } else {
    *appstate = as;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
      SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
      return SDL_APP_FAILURE;
  }
  if (!SDL_CreateWindowAndRenderer("yesyesyesyayyayya", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &as->window, &as->renderer)) {
      SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
      return SDL_APP_FAILURE;
  }

  as->bulletCount = 0;

  SDL_FPoint position;
  position.x = (float) WINDOW_WIDTH/2.0f- 150;
  position.y = (float) WINDOW_HEIGHT/2.0f;
  float direction = SDL_PI_D;
  as->player1.position = position;
  as->player1.direction = direction;
  as->player1.isDead = false;
  position.x = (float) WINDOW_WIDTH/2.0f + 150;
  position.y = (float) WINDOW_HEIGHT/2.0f;
  direction = 0;
  as->player2.position = position;
  as->player2.direction = direction;
  as->player2.isDead = false;
  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  AppState *as = appstate;
  switch (event->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    case SDL_EVENT_KEY_DOWN:{
      SDL_Keycode keycode = event->key.key;
      if (keycode == SDLK_UP){
        as->arrows |= 1;
      }
      if (keycode == SDLK_LEFT){
        as->arrows |= 2;
      }
      if (keycode == SDLK_DOWN){
        as->arrows |= 4;
      }
      if (keycode == SDLK_RIGHT){
        as->arrows |= 8;
      }
      if (keycode == SDLK_SPACE && !as->gameOver){
        shoot(as->player1,as->bullets,&as->bulletCount,true);
      }
      if (keycode == SDLK_W){
        as->wasd |= 1;
      }
      if (keycode == SDLK_A){
        as->wasd |= 2;
      }
      if (keycode == SDLK_S){
        as->wasd |= 4;
      }
      if (keycode == SDLK_D){
        as->wasd |= 8;
      }
      if (keycode == SDLK_SEMICOLON && !as->gameOver){
        shoot(as->player2,as->bullets,&as->bulletCount,false);
      }
      break;
    }
    case SDL_EVENT_KEY_UP:{
      SDL_Keycode keycode = event->key.key;
      if (keycode == SDLK_UP){
        as->arrows &= 14;
      }
      if (keycode == SDLK_LEFT){
        as->arrows &= 13;
      }
      if (keycode == SDLK_DOWN){
        as->arrows &= 11;
      }
      if (keycode == SDLK_RIGHT){
        as->arrows &= 7;
      }
      if (keycode == SDLK_W){
        as->wasd &= 14;
      }
      if (keycode == SDLK_A){
        as->wasd &= 13;
      }
      if (keycode == SDLK_S){
        as->wasd &= 11;
      }
      if (keycode == SDLK_D){
        as->wasd &= 7;
      }
      break;
    }
  }

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
  AppState *as = appstate;
  Player player1 = as->player1;
  Player player2 = as->player2;
  static Uint64 past = 0;
  Uint64 now = SDL_GetTicksNS();
  Uint64 deltaTimeNS = now - past;
  update(&as->player1, &as->player2, as->bullets,&as->bulletCount,as->arrows, as->wasd, &as->gameOver, deltaTimeNS);
  render(as->renderer, player1, player2, as->bullets,as->bulletCount);
  past = now;
  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  SDL_free(appstate);
}

