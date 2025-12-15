#include "raylib.h"
#include "game.h"

// Gestionnaire de texture
Texture2D gTileTextures[32];
int gTileTextureCount = 0;

int main(void)
{
    // ===============================================================
    // MODE FENÊTRÉ REDIMENSIONNABLE (Le bouton vert marchera !)
    // ===============================================================
    
    // 1. FLAG_WINDOW_RESIZABLE : C'est la clé ! Ça autorise à agrandir la fenêtre manuellement.
    // 2. FLAG_WINDOW_HIGHDPI : Pour que ce soit net sur Mac.
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

    // On initialise une fenêtre de base
    InitWindow(800, 600, "Raylib Board Game");

    // On la redimensionne tout de suite pour qu'elle soit confortable (90% de l'écran)
    int monitor = GetCurrentMonitor();
    int screenW = GetMonitorWidth(monitor);
    int screenH = GetMonitorHeight(monitor);
    
    int windowWidth = (int)(screenW * 0.90f);
    int windowHeight = (int)(screenH * 0.90f);

    SetWindowSize(windowWidth, windowHeight);
    SetWindowPosition((screenW - windowWidth) / 2, (screenH - windowHeight) / 2);

    // IMPORTANT : On définit une taille minimale pour éviter de tout casser si on réduit trop
    SetWindowMinSize(400, 400);

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

        // Si tu redimensionnes la fenêtre, Raylib mettra à jour GetScreenWidth/Height
        // et ton GameDraw s'adaptera automatiquement.
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