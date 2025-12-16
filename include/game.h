#ifndef GAME_H
#define GAME_H

#include "raylib.h"

extern Sound gPieceSound;
extern Sound gCheckSound;
extern Sound gEatingSound;

extern Texture2D gMenuBackground;

#define TILE_SIZE 32
#define BOARD_COLS 8
#define BOARD_ROWS 8
#define MAX_LAYERS 4
#define INFINITY 2000000
#define INFINITY_SCORE 999999
#define MAX_MOVES 256 // Augmenté pour la génération de coups
#define BOARD_SIZE 8
#define ID_IA 1 // ID du joueur IA (Noir)

typedef struct
{
    int layers[MAX_LAYERS];     // indices dans gTileTextures
    int layerCount;             // nombre de couches utilisées
} Tile;

typedef enum // Etat possible du jeu
{
    STATE_MAIN_MENU, // Etat : sur le menu <- NOUVEL ÉTAT INITIAL
    STATE_DIFFICULTY_MENU, // Choix de la difficulté de l'IA
    STATE_TIME_MENU, // Choix du temps données aux joueurs
    STATE_PLAYING, // Etat : En jeu
    STATE_GAMEOVER // Etat : Fin du jeu
} GameState;

typedef enum // Niveau de difficulté
{
    DIFF_EASY, // Depth = 1, Delay = 3s
    DIFF_MEDIUM, // Depth = 3, Delay = 2s
    DIFF_HARD // Depth = 5, Delay = 1s
} AIDifficulty;

typedef enum
{
    MODE_NONE, // Aucun mode
    MODE_PLAYER_VS_PLAYER, // 1v1
    MODE_PLAYER_VS_IA // Contre IA
} GameMode;

typedef enum
{
    TURN_PLAYER,
    TURN_IA_WAITING,
    TURN_IA_MOVING
} TurnState;

typedef struct
{
    int startX, startY; // Ligne et colonne avant le mouvement
    int endX, endY; // Après le mouvement
    int movingPieceID; // Quel pièce est joué
    int capturedPieceID; // Stocke la pièce mangé si y'en a une
} Move;

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
    float IADelay; // Délai avant que l'IA puisse jouer
    TurnState turnState;
    AIDifficulty difficulty; // Difficulté choisie
    int AIDepth; // Profondeur AlphaBeta
    float AIDefaultDelay; // Délai par défaut
    Move lastMove; // Stocke le dernier coup

    // --- NOUVEAU : GESTION DES PIÈCES CAPTURÉES ---
    int capturedByWhite[16]; // Liste des ID des pièces mangées par les Blancs
    int capturedByWhiteCount; // Nombre de pièces mangées par les Blancs
    
    int capturedByBlack[16]; // Liste des ID des pièces mangées par les Noirs
    int capturedByBlackCount; // Nombre de pièces mangées par les Noirs
    // ----------------------------------------------

} Board;

void GameInit(Board *board);
void GameUpdate(Board *board, float dt);
void GameDraw(const Board *board);

#endif