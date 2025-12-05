#include "game.h"
#include <stdio.h> // Pour TraceLog et printf

// --- 0. IMPORT EXTERNE ---
extern Texture2D gTileTextures[]; 
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

// Renvoie 0 pour Blanc, 1 pour Noir, et -1 si ce n'est pas une pièce (sol)
static int GetPieceColor(int textureID)
{
    // Les sols sont 0 et 1, donc on les ignore
    if (textureID < 2) return -1;
    
    // Si l'ID est Pair (ex: 2, 4, 6) -> Blanc (0)
    // Si l'ID est Impair (ex: 3, 5, 7) -> Noir (1)
    return (textureID % 2 == 0) ? 0 : 1;
}

// --- 3. LOGIQUE DE MOUVEMENT (PHYSIQUE) ---

// Vérifie si le mouvement est autorisé par les règles de la pièce (Géométrie + Obstacles)
static bool IsMoveValid(Board *board, int startX, int startY, int endX, int endY)
{
    Tile *startTile = &board->tiles[startY][startX];
    int pieceID = startTile->layers[startTile->layerCount - 1]; // ID de la pièce qu'on bouge
    
    int dx = endX - startX; // Différence en X
    int dy = endY - startY; // Différence en Y

    // --- LOGIQUE POUR LES TOURS (ID 12 = Blanche, 13 = Noire) ---
    if (pieceID == 12 || pieceID == 13)
    {
        // RÈGLE 1 : Géométrie (Doit être une ligne droite)
        // Soit dx est 0 (mouvement vertical), soit dy est 0 (mouvement horizontal)
        if (dx != 0 && dy != 0) return false; // Si les deux changent, c'est une diagonale -> Interdit

        // RÈGLE 2 : Obstacles (Le chemin doit être vide)
        
        // Cas Horizontal
        if (dy == 0)
        {
            int step = (dx > 0) ? 1 : -1; // On va vers la droite (1) ou la gauche (-1) ?
            // On boucle de la case d'après le départ, jusqu'à juste avant l'arrivée
            for (int x = startX + step; x != endX; x += step)
            {
                if (board->tiles[startY][x].layerCount > 1) return false; // Y'a un truc sur le chemin !
            }
        }
        // Cas Vertical
        else if (dx == 0)
        {
            int step = (dy > 0) ? 1 : -1; // On va vers le bas (1) ou le haut (-1) ?
            for (int y = startY + step; y != endY; y += step)
            {
                if (board->tiles[y][startX].layerCount > 1) return false; // Obstacle !
            }
        }
        
        return true; // Si on a passé les tests, c'est bon !
    }

    // --- AUTRES PIÈCES ---
    // Pour l'instant, on autorise tout pour les autres pièces (Cavaliers, Fous...)
    return true; 
}

// --- 4. INITIALISATION DU JEU ---
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

            // --- PLACEMENT DES PIÈCES ---
            
            // Pions Noirs (Ligne 1)
            if (y == 1) TilePush(t, 7);
            // Pions Blancs (Ligne 6)
            if (y == 6) TilePush(t, 6);

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
    board->timer.whiteTime = 600.0f; // 10 minutes
    board->timer.blackTime = 600.0f;
}

// --- 5. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{
    // Gestion du Timer
    if (currentTurn == 0) {
        if (board->timer.whiteTime > 0.0f) board->timer.whiteTime -= dt;
    } else {
        if (board->timer.whiteTime > 0.0f) board->timer.blackTime -= dt;
    }

    if (board->timer.whiteTime <= 0.0f || board->timer.blackTime <= 0.0f) {
        TraceLog(LOG_WARNING, "GAME OVER - Temps écoulé !");
    }

    // Gestion de la Souris
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition();

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        int tileSizeW = screenW / BOARD_COLS;
        int tileSizeH = screenH / BOARD_ROWS;
        int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH;
        int boardW = tileSize * BOARD_COLS;
        int boardH = tileSize * BOARD_ROWS;
        int offsetX = (screenW - boardW) / 2;
        int offsetY = (screenH - boardH) / 2;

        int x = (int)((m.x - offsetX) / tileSize);
        int y = (int)((m.y - offsetY) / tileSize);

        if (x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS)
        {
            Tile *clickedTile = &board->tiles[y][x];

            // --- CAS 1 : SÉLECTION ---
            if (selectedX == -1)
            {
                if (clickedTile->layerCount > 1) 
                {
                    int pieceID = clickedTile->layers[clickedTile->layerCount - 1];
                    int pieceColor = GetPieceColor(pieceID);

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
            // --- CAS 2 : ACTION ---
            else 
            {
                bool moveAllowed = false;

                // A. Annulation
                if (x == selectedX && y == selectedY)
                {
                    selectedX = -1;
                    selectedY = -1;
                    TraceLog(LOG_INFO, "Annulation");
                }
                else
                {
                    // B. Vérification Physique (Est-ce que la pièce a le droit ?)
                    if (IsMoveValid(board, selectedX, selectedY, x, y))
                    {
                        // C. Vérification Cible (Vide ou Ennemi ?)
                        
                        // Scénario 1 : Case VIDE
                        if (clickedTile->layerCount == 1)
                        {
                            moveAllowed = true;
                        }
                        // Scénario 2 : Case OCCUPÉE (Bagarre)
                        else if (clickedTile->layerCount > 1)
                        {
                            int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                            int targetColor = GetPieceColor(targetID);

                            if (targetColor != -1 && targetColor != currentTurn)
                            {
                                TraceLog(LOG_INFO, "BAGARRE !");
                                TilePop(clickedTile); // On mange
                                moveAllowed = true;
                            }
                            else
                            {
                                TraceLog(LOG_WARNING, "Bloqué par un ami.");
                            }
                        }
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Mouvement interdit (Règle ou Obstacle) !");
                    }

                    // D. Exécution
                    if (moveAllowed)
                    {
                        Tile *oldTile = &board->tiles[selectedY][selectedX];
                        int objID = TilePop(oldTile); 
                        TilePush(clickedTile, objID);

                        selectedX = -1;
                        selectedY = -1;
                        currentTurn = 1 - currentTurn; 
                        TraceLog(LOG_INFO, "Coup valide.");
                    }
                }
            }
        }
        else
        {
            selectedX = -1;
            selectedY = -1;
        }
    }
}

// --- 6. DESSIN DU JEU ---
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

    const int FONT_SIZE = 30; 
    const int TEXT_PADDING = 20; 

    int centerTextY = offsetY + boardH / 2 - FONT_SIZE / 2;

    // Dessin du plateau  
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            const Tile *t = &board->tiles[y][x];

            int drawX = offsetX + x * tileSize;
            int drawY = offsetY + y * tileSize;

            // Dessine toutes les couches
            for (int i = 0; i < t->layerCount; i++)
            {
                int idx = t->layers[i];
                DrawTexturePro(
                    gTileTextures[idx],
                    (Rectangle){0, 0, gTileTextures[idx].width, gTileTextures[idx].height},
                    (Rectangle){drawX, drawY, tileSize, tileSize},
                    (Vector2){0,0},
                    0,
                    WHITE
                );
            }
            
            // Dessin de la sélection
            if (x == selectedX && y == selectedY)
            {
                DrawRectangleLines(drawX, drawY, tileSize, tileSize, GREEN);
            }
            else 
            {
                DrawRectangleLines(drawX, drawY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
            }
        }
    }

    // --- AFFICHAGE TEXTE (TIMERS) ---
    // Blancs
    int whiteM = (int)board->timer.whiteTime / 60;
    int whiteS = (int)board->timer.whiteTime % 60;
    Color whiteColor = (currentTurn == 0) ? BLUE : DARKGRAY;
    char whiteText[32];
    TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS, whiteText);
    int whiteTextWidth = MeasureText("BLANCS", FONT_SIZE);

    DrawText(TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS),
            offsetX - whiteTextWidth - TEXT_PADDING,
            centerTextY,
            FONT_SIZE, whiteColor);

    // Noirs
    int blackM = (int)board->timer.blackTime / 60;
    int blackS = (int)board->timer.blackTime % 60;
    Color blackColor = (currentTurn == 1) ? BLUE : DARKGRAY;

    DrawText(TextFormat("NOIRS\n%02d:%02d", blackM, blackS),
            offsetX + boardW + TEXT_PADDING,
            centerTextY,
            FONT_SIZE, blackColor);
