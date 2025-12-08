#ifndef GAME_H
#define GAME_H

#include "raylib.h"

#define TILE_SIZE 32
#define BOARD_COLS 8
#define BOARD_ROWS 8
#define MAX_LAYERS 4
#define INFINITY 2000000
#define MAX_MOVES 256
#define BOARD_SIZE 8
Move moveList[MAX_MOVES];
#define ID_IA 1

typedef struct
{
    int layers[MAX_LAYERS];     // indices dans gTileTextures
    int layerCount;             // nombre de couches utilisées
} Tile;

typedef enum // Etat possible du jeu
{
    STATE_MAIN_MENU, // Etat : sur le menu
    STATE_PLAYING, // Etat : En jeu
    STATE_GAMEOVER // Etat : Fin du jeu
} GameState;

typedef enum
{
    MODE_NONE, // Aucun mode
    MODE_PLAYER_VS_PLAYER, // 1v1
    MODE_PLAYER_VS_IA // Contre IA
} GameMode;

typedef struct
{
    int startX, startY; // Ligne et colonne avant le mouvement
    int endX, endY; // Après le mouvement
    int movingPieceID; // Quel pièce est joué
    int capturedPieceID; // Stocke la pièce mangé si y'en a une
}Move;
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
    GameMode mode;
    int winner; // 0 = Blanc, 1 = Noir, -1 = Non-défini
    Move move;
} Board;

void GameInit(Board *board);
void GameUpdate(Board *board, float dt);
void GameDraw(const Board *board);

#endif