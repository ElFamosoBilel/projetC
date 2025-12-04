#include "game.h"
#include <stdio.h> // Pour TraceLog et printf

// --- 0. IMPORT EXTERNE ---
// On a besoin d'accéder au tableau des textures défini dans main.c
// (Assure-toi que gTileTextures est bien une variable globale dans main.c)
extern Texture2D gTileTextures[]; 

// --- 1. VARIABLES GLOBALES ---
// Définition réelle des variables (elles sont déclarées extern dans game.h)
int selectedX = -1;
int selectedY = -1;

// --- 2. FONCTIONS UTILITAIRES (Helpers) ---

// Vide une tuile complètement
static void TileClear(Tile *t) 
{
    t->layerCount = 0;
    for(int i = 0; i < MAX_LAYERS; i++) {
        t->layers[i] = 0;
    }
}

// Ajoute un élément sur le dessus de la pile (Push)
static void TilePush(Tile *t, int textureIndex) 
{
    if (t->layerCount < MAX_LAYERS) {
        t->layers[t->layerCount] = textureIndex;
        t->layerCount++;
    }
}

// Retire l'élément du dessus et renvoie son ID (Pop)
static int TilePop(Tile *t) 
{
    // Sécurité : on ne retire rien si c'est juste le sol (couche 0) ou vide
    if (t->layerCount <= 1) return 0;

    // On récupère l'index de la texture du sommet
    int objectIndex = t->layers[t->layerCount - 1];
    
    // On réduit la taille de la pile (effacement virtuel)
    t->layerCount--;

    return objectIndex;
}

// --- 3. INITIALISATION DU JEU ---
void GameInit(Board *board)
{
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            Tile *t = &board->tiles[y][x];
            TileClear(t);

            // Couche 0 : Sol (Damier)
            int groundIndex = (x + y) % 2; 
            TilePush(t, groundIndex);

            // Couche 1 : Objets
            // Condition : 2 premières lignes (0,1) OU 2 dernières (6,7)
            if (y < 2 || y >= BOARD_ROWS - 2)
            {
                int objectIndex = 2; // Ton objet (marteau, etc.)
                TilePush(t, objectIndex);
            }
        }
    }
}

// --- 4. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{
    (void)dt; // On ignore dt pour l'instant (évite le warning)

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition();
        
        // Conversion Pixels -> Coordonnées Grille
        int x = (int)(m.x / TILE_SIZE);
        int y = (int)(m.y / TILE_SIZE);

        // Vérification des limites du tableau
        if (x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS)
        {
            Tile *clickedTile = &board->tiles[y][x];

            // CAS 1 : Rien n'est sélectionné -> On sélectionne
            if (selectedX == -1)
            {
                // On ne sélectionne que s'il y a un objet (plus que le sol)
                if (clickedTile->layerCount > 1) 
                {
                    selectedX = x;
                    selectedY = y;
                    TraceLog(LOG_INFO, "SELECTION: (%d, %d)", x, y);
                }
            }
            // CAS 2 : Un objet est déjà sélectionné -> On agit
            else 
            {
                // Si on reclique sur le même -> Annulation
                if (x == selectedX && y == selectedY)
                {
                    selectedX = -1;
                    selectedY = -1;
                    TraceLog(LOG_INFO, "DESELECTION");
                }
                // Si la case cible est vide (layerCount == 1, juste le sol) -> Déplacement
                else if (clickedTile->layerCount == 1)
                {
                    // 1. Récupérer la case source
                    Tile *oldTile = &board->tiles[selectedY][selectedX];

                    // 2. Prendre l'objet (Pop)
                    int objID = TilePop(oldTile); 

                    // 3. Poser l'objet (Push)
                    TilePush(clickedTile, objID);

                    // 4. Réinitialiser la sélection
                    selectedX = -1;
                    selectedY = -1;
                    TraceLog(LOG_INFO, "DEPLACEMENT vers (%d, %d)", x, y);
                }
            }
        }
    }
}

// --- 5. DESSIN DU JEU ---
void GameDraw(const Board *board)
{
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            const Tile *t = &board->tiles[y][x];

            for (int i = 0; i < t->layerCount; i++)
            {
                int textureIndex = t->layers[i];
                DrawTexture(
                    gTileTextures[textureIndex], 
                    x * TILE_SIZE, 
                    y * TILE_SIZE, 
                    WHITE
                );
            }
        }
    }

    // Dessin du curseur de sélection 
    if (selectedX != -1 && selectedY != -1)
    {
        DrawRectangleLines(
            selectedX * TILE_SIZE, 
            selectedY * TILE_SIZE, 
            TILE_SIZE, 
            TILE_SIZE, 
            BLUE
        );
    }
}