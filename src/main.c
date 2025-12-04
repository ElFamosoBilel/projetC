#include "raylib.h"
#include "game.h"


// Gestionnaire de texture
<<<<<<< Updated upstream
Texture2D gTileTextures[3];
int gTileTextureCount = 0;
=======
Texture2D gTileTextures[14];  // <-- IMPORTANT : il faut 14 textures, pas 3 !
int gTileTextureCount = 14;
>>>>>>> Stashed changes


int main(void)
{
<<<<<<< Updated upstream
    const int screenWidth = BOARD_COLS * TILE_SIZE;
    const int screenHeight = BOARD_ROWS * TILE_SIZE;

    InitWindow(screenWidth, screenHeight, "Raylib Board Game - macOS M1");
    SetTargetFPS(60);

    // Chargement des textures
    gTileTextures[0] = LoadTexture("assets/sand.png");
    gTileTextures[1] = LoadTexture("assets/water.png");
    gTileTextures[2] = LoadTexture("assets/tool.png");
    gTileTextureCount = 3;
    
=======
    // Taille du moniteur
    int screenW = GetMonitorWidth(0);
    int screenH = GetMonitorHeight(0);

    InitWindow(screenW, screenH, "Raylib Board Game");
    ToggleFullscreen();
    SetTargetFPS(60);

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

>>>>>>> Stashed changes
    Board board = {0};
    GameInit(&board);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        GameUpdate(&board, dt);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        GameDraw(&board);
        DrawFPS(10, 10);

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
