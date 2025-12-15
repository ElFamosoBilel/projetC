#include "raylib.h"
#include "game.h"

// Gestionnaire de texture
Texture2D gTileTextures[32];
int gTileTextureCount = 0;
Texture2D gMenuBackground = { 0 };

Sound gPieceSound = { 0 };
Sound gCheckSound = { 0 };
Sound gEatingSound = { 0 };

int main(void)
{
    // Permet de prendre la taille du moniteur de l'utilisateur
    int screenW = GetMonitorWidth(0);
    int screenH = GetMonitorHeight(0);

    InitWindow(screenW, screenH, "Raylib Board Game");
    ToggleFullscreen(); // met en plein écran

    InitAudioDevice();
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

    gMenuBackground = LoadTexture("assets/fond_bois.jpg");
    gPieceSound = LoadSound("assets/piece_sound.mp3");
    gCheckSound = LoadSound("assets/echec_sound.mp3");
    gEatingSound = LoadSound("assets/eating_sound.mp3");
    SetSoundVolume(gPieceSound, 2.0f);
    SetSoundVolume(gCheckSound, 1.5f);
    SetSoundVolume(gEatingSound, 1.5f);

    Board board = {0}; // Met tous les membres à 0
    GameInit(&board); // Appelle le Gameinit de game

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Temps écoulé depuis la derniere image

        GameUpdate(&board, dt); // Appelle le GameUpdate de game

        BeginDrawing(); // Dessin
        ClearBackground(BLACK);  // Le fond

        GameDraw(&board); // Dessin l'échiquier et tout se qui va avec

        EndDrawing();
    }

    UnloadSound(gPieceSound);
    UnloadSound(gCheckSound);
    UnloadSound(gEatingSound);

    UnloadTexture(gMenuBackground);

    CloseAudioDevice();

    // Libération mémoire
    for (int i = 0; i < gTileTextureCount; i++)
    {
        UnloadTexture(gTileTextures[i]);
    }

    CloseWindow();
    return 0; // Termine l'éxécution
}