#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 400
#define GRAVITY 0.6
#define JUMP_Speed -12.0
#define OBSTACLE_SPEED 8
#define MAX_OBSTACLES 5
#define MAX_CLOUDS 10
#define FRAME_RATE 60
#undef main
typedef enum { RUN, JUMP, DUCK, DEAD } DinoState;
typedef enum { TREE, BIRD } ObstacleType;

typedef struct {
    SDL_Texture* texture;
    SDL_Rect rect;
    DinoState state;
    float velocity;
    int frame;
    bool onGround;
} Dinosaur;

typedef struct {
    SDL_Texture* texture;
    SDL_Rect rect;
    bool isHigh;
    int type;
} Obstacle;

typedef struct {
    SDL_Texture* texture;
    SDL_Rect rect;
    float speed;
} Cloud;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

SDL_Texture* dinoRun[2];
SDL_Texture* dinoDuck[2];
SDL_Texture* dinoJump;
SDL_Texture* dinoDie;
SDL_Texture* treeTex[5];
SDL_Texture* cloudTex;
int treeH[6],treeW[6];

bool gameRunning = true;
bool gameStart = false;
int score = 0;
int bestScore = 0;
SDL_Event event;
int frameStart, frameTime;
float gameSpeed = 1.0f;
Obstacle obstacles[MAX_OBSTACLES] = {0};
Cloud clouds[MAX_CLOUDS] = {0};

void loadTextures();
void handleInput(Dinosaur* dino);
void updateDino(Dinosaur* dino);
void spawnObstacle();
void updateObstacles();
void spawnCloud();
void updateClouds();
bool checkCross(SDL_Rect a, SDL_Rect b);
void renderScore();
void cleanup(Dinosaur *dino);

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG){
        printf("SDL_image load error: %s\n", IMG_GetError());
        return 1;
    }
    if (TTF_Init() == -1){
        printf("SDL_ttf load error: %s\n", TTF_GetError());
        return 1;
    }
    window = SDL_CreateWindow("Tex-Dino",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             SCREEN_WIDTH,
                             SCREEN_HEIGHT,
                             SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    font = TTF_OpenFont("./res/minecraft.ttf", 24);
    loadTextures();
    srand(time(NULL));

    Dinosaur dino = {
        .rect = {100, 300, 70, 70},
        .state = RUN,
        .velocity = 0.0f,
        .frame = 0,
        .onGround = true
    };

    SDL_Texture *roadTexture=IMG_LoadTexture(renderer,"./res/road.png");

    for (int i = 0; i < MAX_CLOUDS; i++) {
        spawnCloud();
    }

    int lastSpawn = 0;
    int lastScoreUpdate = 0;
    int lastSpeedIncrease = 0;

    while (gameRunning) {
        frameStart = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                gameRunning = false;
        }

        if (gameStart) {
            handleInput(&dino);
            updateDino(&dino);
            if (SDL_GetTicks() - lastSpawn > 1500 / gameSpeed) {
                spawnObstacle();
                lastSpawn = SDL_GetTicks();
            }
            updateObstacles();
            updateClouds();
            if (SDL_GetTicks() - lastScoreUpdate > 100) {
                score += 5;
                lastScoreUpdate = SDL_GetTicks();
            }
            if (SDL_GetTicks() - lastSpeedIncrease > 5000) {
                gameSpeed += 0.1;
                lastSpeedIncrease = SDL_GetTicks();
            }
            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].texture && checkCross(dino.rect, obstacles[i].rect)) {
                    dino.state = DEAD;
                    gameStart = false;
                    if (score > bestScore) {
                        bestScore = score;
                    }
                    break;
                }
            }
        } else {
            const unsigned char * keys = SDL_GetKeyboardState(NULL);
            if (keys[SDL_SCANCODE_SPACE]) {
                gameStart = true;
                score = 0;
                gameSpeed = 1.0f;
                dino.state = RUN;
                dino.rect.y = 300;
                for (int i = 0; i < MAX_OBSTACLES; i++) {
                    obstacles[i].texture = NULL;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        for (int i = 0; i < MAX_CLOUDS; i++) {
            if (clouds[i].texture) {
                SDL_RenderCopy(renderer, clouds[i].texture, NULL, &clouds[i].rect);
            }
        }
        SDL_Rect ground = {0, 345, SCREEN_WIDTH, 12};
        SDL_RenderCopy(renderer, roadTexture, NULL, &ground);
        switch (dino.state) {
            case RUN:
                SDL_RenderCopy(renderer, dinoRun[(SDL_GetTicks() / 200) % 2], NULL, &dino.rect);
                break;
            case JUMP:
                SDL_RenderCopy(renderer, dinoJump, NULL, &dino.rect);
                break;
            case DUCK:
                SDL_RenderCopy(renderer, dinoDuck[(SDL_GetTicks() / 200) % 2], NULL, &dino.rect);
                break;
            case DEAD:
                SDL_RenderCopy(renderer, dinoDie, NULL, &dino.rect);
                break;
        }
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (obstacles[i].texture) {
                SDL_RenderCopy(renderer, obstacles[i].texture, NULL, &obstacles[i].rect);
            }
        }
        renderScore();
        
        if (!gameStart) {
            SDL_Color color = {0, 0, 0};
            SDL_Surface* surface = TTF_RenderText_Solid(font, 
                dino.state == DEAD ? "Game Over! Press SPACE to restart" : "Press SPACE to Start", 
                color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                SCREEN_WIDTH/2 - surface->w/2,
                SCREEN_HEIGHT/2 - surface->h/2,
                surface->w,
                surface->h
            };
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }

        SDL_RenderPresent(renderer);

        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 1000 / FRAME_RATE) {
            SDL_Delay(1000 / FRAME_RATE - frameTime);
        }
    }

    cleanup(&dino);
    return 0;
}

void loadTextures() {
    dinoRun[0] = IMG_LoadTexture(renderer, "./res/dino-run1.png");
    dinoRun[1] = IMG_LoadTexture(renderer, "./res/dino-run2.png");
    dinoDuck[0] = IMG_LoadTexture(renderer, "./res/dino-de1.png");
    dinoDuck[1] = IMG_LoadTexture(renderer, "./res/dino-de-2.png");
    dinoJump = IMG_LoadTexture(renderer, "./res/dino-jump.png");
    dinoDie = IMG_LoadTexture(renderer, "./res/dino-die.png");

    treeTex[0] = IMG_LoadTexture(renderer, "./res/ltree-1.png");
    treeTex[1] = IMG_LoadTexture(renderer, "./res/ltree-2.png");
    treeTex[2] = IMG_LoadTexture(renderer, "./res/ltree-3.png");
    treeTex[3] = IMG_LoadTexture(renderer, "./res/bird1.png");
    treeTex[4] = IMG_LoadTexture(renderer, "./res/bird2.png");

    cloudTex = IMG_LoadTexture(renderer, "./res/cloud.png");
    for(int i=0;i<5;i++){
        SDL_QueryTexture(treeTex[i],NULL,NULL,&treeW[i], &treeH[i]);
        treeH[i]*=1.5;treeW[i]*=1.5;
    }
}

void handleInput(Dinosaur* dino) {
    const unsigned char * keys = SDL_GetKeyboardState(NULL);

    if (dino->state == DEAD) return;

    if (keys[SDL_SCANCODE_SPACE] && dino->onGround) {
        dino->velocity = JUMP_Speed;
        dino->onGround = false;
        dino->state = JUMP;
    }

    if (keys[SDL_SCANCODE_LSHIFT] && dino->onGround && dino->state != DUCK) {
        dino->state = DUCK;
        dino->rect.h = 50;
        dino->rect.y = 330;
        printf("duck mode\n");
    }

    if (!keys[SDL_SCANCODE_LSHIFT] && dino->state == DUCK && dino->onGround) {
        dino->state = RUN;
        dino->rect.h = 70;
        printf("run mode\n");
    }
}


void updateDino(Dinosaur* dino) {
    dino->velocity += GRAVITY;
    dino->rect.y += dino->velocity;

    if (dino->rect.y >= 300) {
        dino->rect.y = 300;
        dino->velocity = 0;
        dino->onGround = true;

        if (dino->state == JUMP) {
            dino->state = RUN;
            dino->rect.h = 70;
            dino->rect.y = 300;
        }
    }
}

void spawnObstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].texture) {
            ObstacleType type = (rand() % 10 < 7) ? TREE : BIRD;
            obstacles[i].type = type;

            if (type == TREE) {
                int treeIndex = rand() % 3;
                obstacles[i].texture = treeTex[treeIndex];
                obstacles[i].rect.w = treeW[treeIndex];
                obstacles[i].rect.h = treeH[treeIndex];
                obstacles[i].rect.y = 300;
            } else { 
                int birdIndex = 3 + (rand() % 2);
                obstacles[i].texture = treeTex[birdIndex];
                obstacles[i].rect.w = treeW[birdIndex]*0.7;
                obstacles[i].rect.h = treeH[birdIndex]*0.7;
                obstacles[i].rect.y = (rand() % 2 == 0) ? 250 : 300;
            }\

            obstacles[i].rect.x = SCREEN_WIDTH;
            break;
        }
    }
}

void updateObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].texture) {
            obstacles[i].rect.x -= OBSTACLE_SPEED * gameSpeed;
            if (obstacles[i].rect.x + obstacles[i].rect.w < 0) {
                obstacles[i].texture = NULL;
            }
        }
    }
}

void spawnCloud() {
    int random_cloud_count=rand() % MAX_CLOUDS;
    for (int i = 0; i < random_cloud_count&&i < MAX_CLOUDS; i++) {
        if (!clouds[i].texture) {
            clouds[i].texture = cloudTex;
            clouds[i].rect.w = 84;
            clouds[i].rect.h = 26;
            clouds[i].rect.x = SCREEN_WIDTH + rand() % 800;
            clouds[i].rect.y = 50 + rand() % 150;
            clouds[i].speed = 1.0+ (rand() % 100) / 100.0;
            break;
        }
    }
}

void updateClouds() {
    for (int i = 0; i < MAX_CLOUDS; i++) {
        if (clouds[i].texture) {
            clouds[i].rect.x -= clouds[i].speed * gameSpeed;
            if (clouds[i].rect.x + clouds[i].rect.w < 0) {
                clouds[i].texture = NULL;
                spawnCloud();
            }
        }
    }
}

bool checkCross(SDL_Rect a, SDL_Rect b) {
    return SDL_HasIntersection(&a, &b);
}

void renderScore() {
    SDL_Color color = {0, 0, 0};
    char scoreText[64];
    snprintf(scoreText, sizeof(scoreText), "Score: %d  HI: %d", score, bestScore);
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, scoreText, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect rect = {10, 10, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void cleanup(Dinosaur *dino) {
    for (int i = 0; i < 2; i++) {
        SDL_DestroyTexture(dinoRun[i]);
        SDL_DestroyTexture(dinoDuck[i]);
    }
    SDL_DestroyTexture(dinoJump);
    SDL_DestroyTexture(dinoDie);
    for (int i = 0; i < 5; i++) {
        SDL_DestroyTexture(treeTex[i]);
    }
    SDL_DestroyTexture(cloudTex);
    dino->state = RUN;
    dino->rect.y = 300;
    dino->rect.h = 70;
    dino->velocity = 0;
    dino->onGround = true;

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}