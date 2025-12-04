#include "game.h"
#include <stdio.h> // Pour TraceLog et printf

// --- 0. IMPORT EXTERNE ---
extern Texture2D gTileTextures[]; 
// Note: gTileTextureCount est défini dans main.c, on peut l'utiliser si on l'ajoute en extern,
// mais ici on vérifie juste que l'index existe.
extern int gTileTextureCount; 

// --- 1. VARIABLES GLOBALES ---
int selectedX = -1;
int selectedY = -1;
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

// --- 2. FONCTIONS UTILITAIRES (Helpers) ---

static void TileClear(Tile *t) 
{
    t->layerCount = 0;
    for(int i = 0; i < MAX_LAYERS; i++) {
        t->layers[i] = 0;
    }
}

static void TilePush(Tile *t, int textureIndex) 
{
    if (t->layerCount < MAX_LAYERS) {
        t->layers[t->layerCount] = textureIndex;
        t->layerCount++;
    }
}

static int TilePop(Tile *t) 
{
    if (t->layerCount <= 1) return 0;
    int objectIndex = t->layers[t->layerCount - 1];
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

            // --- PLACEMENT DES PIÈCES (Version Jules) ---
            
            // Pions Noirs (Ligne 1)
            if (y == 1) {
                TilePush(t, 7); // Pion noir (vérifie tes IDs dans main.c)
            }
            // Pions Blancs (Ligne 6)
            if (y == 6) {
                TilePush(t, 6); // Pion blanc
            }

            // Pièces Nobles Noires (Ligne 0)
            if (y == 0)
            {
                if (x == 0 || x == 7) TilePush(t, 13); // Tour noire
                if (x == 1 || x == 6) TilePush(t, 3);  // Cavalier noir
                if (x == 2 || x == 5) TilePush(t, 5);  // Fou noir
                if (x == 3) TilePush(t, 9);            // Reine noire
                if (x == 4) TilePush(t, 11);           // Roi noir
            }

            // Pièces Nobles Blanches (Ligne 7)
            if (y == 7)
            {
                if (x == 0 || x == 7) TilePush(t, 12); // Tour blanche
                if (x == 1 || x == 6) TilePush(t, 2);  // Cavalier blanc
                if (x == 2 || x == 5) TilePush(t, 4);  // Fou blanc
                if (x == 3) TilePush(t, 8);            // Reine blanche
                if (x == 4) TilePush(t, 10);           // Roi blanc
            }
        }
    }
}
// Renvoie 0 pour Blanc, 1 pour Noir, et -1 si ce n'est pas une pièce (sol)
static int GetPieceColor(int textureID)
{
    // Les sols sont 0 et 1, donc on les ignore
    if (textureID < 2) return -1;
    
    // Si l'ID est Pair (ex: 2, 4, 6) -> Blanc (0)
    // Si l'ID est Impair (ex: 3, 5, 7) -> Noir (1)
    return (textureID % 2 == 0) ? 0 : 1;
}

// --- 4. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{
    (void)dt; // On ignore dt pour l'instant

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition();

        // --- CALCUL DES COORDONNÉES (Version Jules) ---
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        
        int tileSizeW = screenW / BOARD_COLS;
        int tileSizeH = screenH / BOARD_ROWS;
        int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH;
        
        int boardW = tileSize * BOARD_COLS;
        int boardH = tileSize * BOARD_ROWS;
        
        int offsetX = (screenW - boardW) / 2;
        int offsetY = (screenH - boardH) / 2;

        // Conversion Souris -> Grille
        int x = (int)((m.x - offsetX) / tileSize);
        int y = (int)((m.y - offsetY) / tileSize);

        // Vérification qu'on est bien DANS le plateau
        if (x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS)
        {
            Tile *clickedTile = &board->tiles[y][x];

            // --- CAS 1 : RIEN N'EST SÉLECTIONNÉ ---
            if (selectedX == -1)
            {
                // S'il y a une pièce (couche > 1)
                if (clickedTile->layerCount > 1) 
                {
                    // On vérifie la couleur
                    int pieceID = clickedTile->layers[clickedTile->layerCount - 1];
                    int pieceColor = GetPieceColor(pieceID);

                    // Est-ce que c'est mon tour ?
                    if (pieceColor == currentTurn)
                    {
                        selectedX = x;
                        selectedY = y;
                        TraceLog(LOG_INFO, "Selection OK : (%d, %d)", x, y);
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Pas touche ! Ce n'est pas ton tour.");
                    }
                }
            }
            // --- CAS 2 : UNE PIÈCE EST DÉJÀ SÉLECTIONNÉE (ACTION) ---
            else 
            {
                bool moveAllowed = false;

                // A. On reclique sur soi-même -> Annulation
                if (x == selectedX && y == selectedY)
                {
                    selectedX = -1;
                    selectedY = -1;
                    TraceLog(LOG_INFO, "Annulation");
                }
                else
                {
                    // B. Analyse de la cible
                    
                    // Scénario 1 : La case est VIDE (Juste le sol)
                    if (clickedTile->layerCount == 1)
                    {
                        moveAllowed = true;
                    }
                    // Scénario 2 : La case est OCCUPÉE (Bagarre ?)
                    else if (clickedTile->layerCount > 1)
                    {
                        int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                        int targetColor = GetPieceColor(targetID);

                        // Si c'est un ENNEMI
                        if (targetColor != -1 && targetColor != currentTurn)
                        {
                            TraceLog(LOG_INFO, "BAGARRE ! Piece adverse eliminee.");
                            
                            // ON MANGE LA PIÈCE (On vide la case cible)
                            TilePop(clickedTile); 
                            
                            moveAllowed = true;
                        }
                        else
                        {
                            TraceLog(LOG_WARNING, "Impossible : Case bloquee par un ami.");
                        }
                    }

                    // C. Exécution du mouvement
                    if (moveAllowed)
                    {
                        // 1. On prend notre pièce de l'ancienne case
                        Tile *oldTile = &board->tiles[selectedY][selectedX];
                        int objID = TilePop(oldTile); 
                        
                        // 2. On la pose sur la nouvelle case
                        TilePush(clickedTile, objID);

                        // 3. Fin du tour
                        selectedX = -1;
                        selectedY = -1;
                        
                        // Changement de joueur (0->1 ou 1->0)
                        currentTurn = 1 - currentTurn; 
                        
                        TraceLog(LOG_INFO, "Coup valide. Au suivant !");
                    }
                }
            }
        }
        else
        {
            // Clic en dehors du plateau -> On déselectionne
            selectedX = -1;
            selectedY = -1;
        }
    }
}

// --- 5. DESSIN DU JEU ---
void GameDraw(const Board *board)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Calcul de la taille dynamique (responsive)
    int tileSizeW = screenW / BOARD_COLS;
    int tileSizeH = screenH / BOARD_ROWS;
    int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH;

    int boardW = tileSize * BOARD_COLS;
    int boardH = tileSize * BOARD_ROWS;

    int offsetX = (screenW - boardW) / 2;
    int offsetY = (screenH - boardH) / 2;

    // Dessin du plateau
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            const Tile *t = &board->tiles[y][x];

            // Position de dessin de CETTE case
            int drawX = offsetX + x * tileSize;
            int drawY = offsetY + y * tileSize;

            // Dessine toutes les couches
            for (int i = 0; i < t->layerCount; i++)
            {
                int idx = t->layers[i];
                // On suppose que idx est valide, DrawTexturePro gère le scaling
                DrawTexturePro(
                    gTileTextures[idx],
                    (Rectangle){0, 0, gTileTextures[idx].width, gTileTextures[idx].height},
                    (Rectangle){drawX, drawY, tileSize, tileSize},
                    (Vector2){0,0},
                    0,
                    WHITE
                );
            }
            
            // Dessin de la sélection (Le cadre vert)
            // On le dessine APRES les textures pour qu'il soit par dessus
            if (x == selectedX && y == selectedY)
            {
                DrawRectangleLines(drawX, drawY, tileSize, tileSize, GREEN);
            }
            // Optionnel : un petit quadrillage gris pour bien voir les cases
            else 
            {
                DrawRectangleLines(drawX, drawY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
            }
        }
    }
}