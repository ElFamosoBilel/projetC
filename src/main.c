#include "raylib.h"
#include "game.h"

// Gestionnaire de texture
Texture2D gTileTextures[32];
int gTileTextureCount = 0;

int main(void)
{
    // ===============================================================
    // CONFIGURATION INTELLIGENTE (WINDOWS vs MAC)
    // ===============================================================
    
    // On part sur une base commune : Redimensionnable + VSync
    unsigned int flags = FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT;

    // MAGIE DU C : Ce bloc ne s'active que si on compile sur Mac (__APPLE__)
    #if defined(__APPLE__)
        flags |= FLAG_WINDOW_HIGHDPI; // Ajoute le HighDPI pour Mac (netteté)
    #endif

    // Note : Sur Windows, on N'AJOUTE PAS le HighDPI, car c'est lui qui crée
    // l'effet "zoomé" si ton écran est réglé à 125% ou 150% dans les paramètres Windows.

    SetConfigFlags(flags);

    // ===============================================================

    // Initialisation
    InitWindow(800, 600, "Raylib Board Game");

    // Calcul de la taille de l'écran
    int monitor = GetCurrentMonitor();
    int screenW = GetMonitorWidth(monitor);
    int screenH = GetMonitorHeight(monitor);

    // Taille cible : 90% de l'écran
    int windowWidth = (int)(screenW * 0.90f);
    int windowHeight = (int)(screenH * 0.90f);

    SetWindowSize(windowWidth, windowHeight);
    SetWindowPosition((screenW - windowWidth) / 2, (screenH - windowHeight) / 2);
    
    // Taille minimale pour éviter de casser l'affichage
    SetWindowMinSize(400, 400);

    // Chargement des assets
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