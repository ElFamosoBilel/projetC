#include "raylib.h"
#include "game.h"


// Gestionnaire de texture
Texture2D gTileTextures[3];
int gTileTextureCount = 0;


int main(void)
{
    const int screenWidth = BOARD_COLS * TILE_SIZE;
    const int screenHeight = BOARD_ROWS * TILE_SIZE;

    InitWindow(screenWidth, screenHeight, "Raylib Board Game - macOS M1");
    SetTargetFPS(60);

    // Chargement des textures
    gTileTextures[0] = LoadTexture("assets/carreau_noir.png");
    gTileTextures[1] = LoadTexture("assets/carreau_blanc.png");
    gTileTextures[2] = LoadTexture("assets/tool.png");
    gTileTextureCount = 3;
    
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
        /* DrawText(TextFormat("Time : %.2f", time), 170, 10, 20, GREEN); */

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
