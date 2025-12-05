#ifndef GAME_H
#define GAME_H

#include "raylib.h"

#define TILE_SIZE 32
#define BOARD_COLS 8
#define BOARD_ROWS 8
#define MAX_LAYERS 4

typedef struct
{
    int layers[MAX_LAYERS];     // indices dans gTileTextures
    int layerCount;             // nombre de couches utilisées
} Tile;

typedef enum // Etat possible du jeu
{
    STATE_PLAYING, // Etat : En jeu
    STATE_MENU, // Etat : Menu
    STATE_GAMEOVER // Etat : Fin du jeu
} GameState;

typedef struct 
{
    float whiteTime; // Temps restant pout les Blancs (en secondes)
    float blackTime; // Temps restant pout les Noirs (en secondes)
} Timer;

typedef struct
{
    Tile tiles[BOARD_ROWS][BOARD_COLS];
    Timer timer; 
    GameState state;
    int winner; // 0 = Blanc, 1 = Noir, -1 = Non-défini
} Board;

void GameInit(Board *board);
void GameUpdate(Board *board, float dt);
void GameDraw(const Board *board);

#endif