#include "game.h"
#include <stdio.h>
#include <stdlib.h>

// Gestionnaire de texture
Texture2D gTileTextures[3];
int gTileTextureCount = 0;

// Définition des constantes pour les joueurs
#define BLANC 0
#define NOIR 1

int main(void)
{
    const int screenWidth = BOARD_COLS * TILE_SIZE;
    const int screenHeight = BOARD_ROWS * TILE_SIZE;

    InitWindow(screenWidth, screenHeight, "Raylib Board Game - Tour par tour");
    SetTargetFPS(60);

    // Chargement des textures
    gTileTextures[0] = LoadTexture("assets/carreau_noir.png");
    gTileTextures[1] = LoadTexture("assets/carreau_blanc.png");
    gTileTextures[2] = LoadTexture("assets/tool.png");
    gTileTextureCount = 3;
    
    Board board = {0};
    GameInit(&board);

    // --- ETAPE 1 : Initialisation du joueur ---
    int joueurActuel = BLANC; 
    printf("Debut de partie : Les Blancs commencent.\n");

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // --- ETAPE 2 : Gestion du Clic ---
        // Si le bouton gauche de la souris est pressé
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // On change de joueur (Opérateur ternaire)
            joueurActuel = (joueurActuel == BLANC) ? NOIR : BLANC;

            // Affichage dans la console pour debug
            printf("Changement de tour ! C'est aux %s de jouer.\n", (joueurActuel == BLANC) ? "Blancs" : "Noirs");
        }

        GameUpdate(&board, dt);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        GameDraw(&board);

        // --- ETAPE 3 : Affichage visuel du tour ---
        // On affiche en haut à gauche qui doit jouer
        const char* texteJoueur = (joueurActuel == BLANC) ? "Tour : BLANCS" : "Tour : NOIRS";
        Color couleurTexte = (joueurActuel == BLANC) ? WHITE : BLACK; // Le texte change aussi de couleur
        
    }

    // Libération mémoire
    for (int i = 0; i < gTileTextureCount; i++)
    {
        UnloadTexture(gTileTextures[i]);
    }

    CloseWindow();
    return 0;
}