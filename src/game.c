#include "game.h"
#include <stdio.h> // Pour TraceLog et printf
#include <stdlib.h> // INDISPENSABLE pour la fonction abs()

// --- 0. IMPORT EXTERNE ---
extern Texture2D gTileTextures[]; 
extern int gTileTextureCount; 

// --- 1. VARIABLES GLOBALES ---
int selectedX = -1;
int selectedY = -1;
int currentTurn = 0; // 0 = Blancs, 1 = Noirs
bool gameOver = false;
int winner = -1; // 0 = Blancs, 1 = Noirs


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
    if (textureID < 2) return -1;
    return (textureID % 2 == 0) ? 0 : 1;
}

// Vérifie si le chemin est libre (Scanner)
static bool IsPathClear(const Board *board, int startX, int startY, int endX, int endY){
    // Déplacement horizontal
    if (startY == endY)
    {
        int step = (endX > startX) ? 1 : -1; 
        for (int x = startX + step; x != endX; x += step)
        {
            if (board->tiles[startY][x].layerCount > 1) return false; 
        }
    }
    // Déplacement vertical
    else if (startX == endX)
    {
        int step = (endY > startY) ? 1 : -1;
        for (int y = startY + step; y != endY; y += step)
        {
            if (board->tiles[y][startX].layerCount > 1) return false;
        }
    }
    // Déplacement diagonal
    else if (abs(endX - startX) == abs(endY - startY))
    {
        int stepX = (endX > startX) ? 1 : -1; 
        int stepY = (endY > startY) ? 1 : -1; 

        int x = startX + stepX;
        int y = startY + stepY;

        while (x != endX) // On s'arrête avant la case finale
        {
            if (board->tiles[y][x].layerCount > 1) return false; 
            x += stepX;
            y += stepY;
        }
    }
    // Si ce n'est ni l'un ni l'autre (ex: Cavalier), on considère que le chemin n'est pas "bloquable" par cette fonction
    return true; 
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

            int groundIndex = (x + y) % 2;
            TilePush(t, groundIndex);

            if (y == 1) TilePush(t, 7); 
            if (y == 6) TilePush(t, 6);

            if (y == 0)
            {
                if (x == 0 || x == 7) TilePush(t, 13); 
                if (x == 1 || x == 6) TilePush(t, 3);  
                if (x == 2 || x == 5) TilePush(t, 5);  
                if (x == 3) TilePush(t, 9);            
                if (x == 4) TilePush(t, 11);           
            }
            if (y == 7)
            {
                if (x == 0 || x == 7) TilePush(t, 12);
                if (x == 1 || x == 6) TilePush(t, 2);
                if (x == 2 || x == 5) TilePush(t, 4);
                if (x == 3) TilePush(t, 8);
                if (x == 4) TilePush(t, 10);
            }
        }
    }

    board->timer.whiteTime = 15.0f; // ex : 10 min
    board->timer.blackTime = 15.0f;

    gameOver = false;
    winner = -1;
    selectedX = -1;
    selectedY = -1;
    currentTurn = 0;
}

// --- 4. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{
    // Gestion Timer
    if (currentTurn == 0) {
        if (board->timer.whiteTime > 0.0f) board->timer.whiteTime -= dt;
    } else {
        if (board->timer.blackTime > 0.0f) board->timer.blackTime -= dt;
    }

    // --- TIMER ---
    if (!gameOver)
    {
        if (currentTurn == 0)
            board->timer.whiteTime -= dt;
        else
            board->timer.blackTime -= dt;

        if (board->timer.whiteTime <= 0.0f)
        {
            board->timer.whiteTime = 0.0f;
            winner = 1; // noirs gagnent
            gameOver = true;
        }

        if (board->timer.blackTime <= 0.0f)
        {
            board->timer.blackTime = 0.0f;
            winner = 0; // blancs gagnent
            gameOver = true;
        }
    }


    // Gestion Souris
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition();

        // Calculs responsive
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
        if (gameOver)
        {
            TraceLog(LOG_INFO, "Partie terminée - aucun coup possible");
            return; // ← on sort, plus de mouvement possible
        }

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
                        TraceLog(LOG_INFO, "Selection OK");
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Pas ton tour !");
                    }
                }
            }
            // --- CAS 2 : ACTION ---
            else 
            {
                bool moveAllowed = false;

                Tile *oldTile = &board->tiles[selectedY][selectedX];
                int pieceID = oldTile->layers[oldTile->layerCount - 1];

                int startX = selectedX;
                int startY = selectedY;
                int endX = x;
                int endY = y;
                int dx = endX - startX;
                int dy = endY - startY;

                // Annulation (Clic sur soi-même)
                if (dx == 0 && dy == 0)
                {
                    selectedX = -1;
                    selectedY = -1;
                    TraceLog(LOG_INFO, "Annulation");
                    return; 
                }
                
                // --- VÉRIFICATION PHYSIQUE ---
                
                // Tour
                if (pieceID == 12 || pieceID == 13)
                {
                    if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0))
                    {
                        if (IsPathClear(board, startX, startY, endX, endY)) moveAllowed = true;
                    }
                }
                // Fou
                else if (pieceID == 4 || pieceID == 5)
                {
                    if (abs(dx) == abs(dy) && dx != 0)
                    {
                        if (IsPathClear(board, startX, startY, endX, endY)) moveAllowed = true;
                    }
                }
                // Reine
                else if (pieceID == 8 || pieceID == 9)
                {
                    if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0) || (abs(dx) == abs(dy)))
                    {
                        if (IsPathClear(board, startX, startY, endX, endY)) moveAllowed = true;
                    }
                }
                // Roi
                else if (pieceID == 10 || pieceID == 11)
                {
                    if (abs(dx) <= 1 && abs(dy) <= 1) moveAllowed = true;
                }
                // Cavalier
                else if (pieceID == 2 || pieceID == 3)
                {
                    if ((abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1)) moveAllowed = true;
                }
                // Pion
                else if (pieceID == 6 || pieceID == 7)
                {
                    int direction = (currentTurn == 0) ? -1 : 1; 
                    int initialRow = (currentTurn == 0) ? 6 : 1;
                    
                    // Capture Diagonale
                    if (abs(dx) == 1 && dy == direction)
                    {
                        if (clickedTile->layerCount > 1) { // Doit contenir quelque chose
                            int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                            int targetColor = GetPieceColor(targetID);
                            if (targetColor != -1 && targetColor != currentTurn) moveAllowed = true;
                        }
                    }
                    // Avance 1 case
                    else if (dx == 0 && dy == direction && clickedTile->layerCount == 1) 
                    {
                        moveAllowed = true;
                    }
                    // Avance 2 cases (Premier tour)
                    else if (dx == 0 && dy == 2 * direction && startY == initialRow && clickedTile->layerCount == 1)
                    {
                        Tile *midTile = &board->tiles[startY + direction][startX];
                        if (midTile->layerCount == 1) moveAllowed = true;
                    }
                }

                // --- VÉRIFICATION FINALE (Case cible occupée par un ami ?) ---
                if (moveAllowed)
                {
                    if (clickedTile->layerCount > 1)
                    {
                        int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                        int targetColor = GetPieceColor(targetID);

                        if (targetColor != -1 && targetColor == currentTurn)
                        {
                            TraceLog(LOG_INFO, "Bloqué par un ami.");
                            moveAllowed = false; 
                        }
                        else if (targetColor != -1 && targetColor != currentTurn)
                        {
                            TraceLog(LOG_INFO, "Capture !");
                            TilePop(clickedTile); // Mange l'ennemi
                        }
                    }
                }

                // --- EXÉCUTION ---
                if (moveAllowed)
                {
                    int objID = TilePop(oldTile); 
                    TilePush(clickedTile, objID);

                    selectedX = -1;
                    selectedY = -1;
                    currentTurn = 1 - currentTurn; 
                    TraceLog(LOG_INFO, "Coup valide.");
                }
                else
                {
                    TraceLog(LOG_WARNING, "Mouvement invalide");
                }
            }
        }
        else
        {
            selectedX = -1;
            selectedY = -1;
        }
    }


} // <--- C'EST CELLE-LA QUI MANQUAIT CHEZ JULES !

void GameReset(Board *board)
{
    GameInit(board);
    gameOver = false;
    winner = -1;
    currentTurn = 0;
    selectedX = -1;
    selectedY = -1;
}


// --- 5. DESSIN DU JEU ---
void GameDraw(const Board *board)
{
    // (Garde ton code de dessin ici, il est parfait)
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
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

    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            const Tile *t = &board->tiles[y][x];
            int drawX = offsetX + x * tileSize;
            int drawY = offsetY + y * tileSize;

            for (int i = 0; i < t->layerCount; i++)
            {
                int idx = t->layers[i];
                DrawTexturePro(gTileTextures[idx],(Rectangle){0, 0, gTileTextures[idx].width, gTileTextures[idx].height},(Rectangle){drawX, drawY, tileSize, tileSize},(Vector2){0,0},0,WHITE);
            }
            
            if (x == selectedX && y == selectedY) DrawRectangleLines(drawX, drawY, tileSize, tileSize, GREEN);
            else DrawRectangleLines(drawX, drawY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
        }
    }

    int whiteM = (int)board->timer.whiteTime / 60;
    int whiteS = (int)board->timer.whiteTime % 60;
    Color whiteColor = (currentTurn == 0) ? BLUE : DARKGRAY;
    DrawText(TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS), offsetX - MeasureText("BLANCS", FONT_SIZE) - TEXT_PADDING, centerTextY, FONT_SIZE, whiteColor);

    int blackM = (int)board->timer.blackTime / 60;
    int blackS = (int)board->timer.blackTime % 60;
    Color blackColor = (currentTurn == 1) ? BLUE : DARKGRAY;
    DrawText(TextFormat("NOIRS\n%02d:%02d", blackM, blackS), offsetX + boardW + TEXT_PADDING, centerTextY, FONT_SIZE, blackColor);

    if (gameOver)
    {
        const char *text = (winner == 0) ? "VICTOIRE DES BLANCS" : "VICTOIRE DES NOIRS";
        int size = 60;
        DrawText(text,
                GetScreenWidth()/2 - MeasureText(text, size)/2,
                GetScreenHeight()/2 - size/2,
                size,
                RED);
    }

}