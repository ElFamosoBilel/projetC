#include "raylib.h"
#include "game.h"

// Gestionnaire de texture
Texture2D gTileTextures[32];
int gTileTextureCount = 0;

int main(void)
{
    // ===============================================================
    // VERSION FENÊTRE MAXIMISÉE (Sûre et Nette)
    // ===============================================================

    // 1. FLAG_WINDOW_HIGHDPI : Indispensable pour que ce soit NET (Retina)
    // 2. FLAG_WINDOW_RESIZABLE : Permet de redimensionner la fenêtre si besoin
    // 3. FLAG_VSYNC_HINT : Évite les scintillements
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);

    // On initialise une fenêtre avec une taille standard confortable.
    // Raylib va gérer la densité de pixels automatiquement grâce au flag HighDPI.
    InitWindow(1280, 900, "Raylib Board Game");

    // On demande à la fenêtre de prendre toute la place disponible sur le bureau
    // (Sans passer en mode "Plein écran exclusif" qui faisait bugger le Mac)
    MaximizeWindow();

    // ===============================================================

    // Chargement des textures
    gTileTextures[0] = LoadTexture("assets/carreau_blanc.png");
    gTileTextures[1] = LoadTexture("assets/carreau_noir.png");
    gTileTextures[2] = LoadTexture("assets/cavalier_blanc.png");
    gTileTextures[3] = LoadTexture("assets/cavalier_noir.png");
    gTileTextures[4] = LoadTexture("assets/fou_blanc.png");
    gTileTextures[5] = LoadTexture("assets/fou_noir.png");
    gTileTextures[6] = LoadTexture("assets/pion_blanc.png");
    gTileTextures[7] = LoadTexture("assets/pion_noir.png");
    gTileTextures[8] = LoadTexture("assets/reine_blanche.png");
    gTileTextures[9] = LoadTexture("assets/reine_noir.png");
    gTileTextures[10] = LoadTexture("assets/roi_blanc.png");
    gTileTextures[11] = LoadTexture("assets/roi_noir.png");
    gTileTextures[12] = LoadTexture("assets/tour_blanche.png");
    gTileTextures[13] = LoadTexture("assets/tour_noir.png");
    gTileTextureCount = 14;
    
    Board board = {0}; 
    GameInit(&board); 

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); 

        GameUpdate(&board, dt); 

        BeginDrawing(); 
        ClearBackground(BLACK);  

        GameDraw(&board); 

        EndDrawing();
    }

    // Libération mémoire
    for (int i = 0; i < gTileTextureCount; i++)
    {
        UnloadTexture(gTileTextures[i]);
    }

    CloseWindow();
    return 0; 
}