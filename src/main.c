#include "raylib.h"
#include "game.h"

// Gestionnaire de texture
Texture2D gTileTextures[32];
int gTileTextureCount = 0;

int main(void)
{
    // Permet de prendre la taille du moniteur
    int screenW = GetMonitorWidth(0);
    int screenH = GetMonitorHeight(0);

    InitWindow(screenW, screenH, "Raylib Board Game");
    ToggleFullscreen();

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
        ClearBackground(RAYWHITE);

        GameDraw(&board);

        double time = GetTime();
        DrawText(TextFormat("Time : %.2f", time), 170, 10, 20, GREEN);

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
