#include "game.h"
#include <stdio.h> // Pour TraceLog et printf
#include <stdlib.h> // Pour abs()

// --- 0. IMPORT EXTERNE ---
extern Texture2D gTileTextures[]; 
extern int gTileTextureCount; 

// --- 1. VARIABLES GLOBALES ---
int selectedX = -1;
int selectedY = -1;
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

// Variables de fin de partie (Ajout Gauthier)
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
    return true; 
}

// --- 3. INITIALISATION DU JEU ---
void GameInit(Board *board)
{
    // 1. Initialisation du plateau et des pièces
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            Tile *t = &board->tiles[y][x];
            TileClear(t);

            // Sol
            int groundIndex = (x + y) % 2;
            TilePush(t, groundIndex);

            // Pions
            if (y == 1) TilePush(t, 7); // Noir
            if (y == 6) TilePush(t, 6); // Blanc

            // Nobles Noirs (Ligne 0)
            if (y == 0)
            {
                if (x == 0 || x == 7) TilePush(t, 13); // Tour
                if (x == 1 || x == 6) TilePush(t, 3);  // Cavalier
                if (x == 2 || x == 5) TilePush(t, 5);  // Fou
                if (x == 3) TilePush(t, 9);            // Reine
                if (x == 4) TilePush(t, 11);           // Roi
            }
            // Nobles Blancs (Ligne 7)
            if (y == 7)
            {
                if (x == 0 || x == 7) TilePush(t, 12); // Tour
                if (x == 1 || x == 6) TilePush(t, 2);  // Cavalier
                if (x == 2 || x == 5) TilePush(t, 4);  // Fou
                if (x == 3) TilePush(t, 8);            // Reine
                if (x == 4) TilePush(t, 10);           // Roi
            }
        }
    }

    // 2. Initialisation des Timers (10 minutes = 600s)
    board->timer.whiteTime = 600.0f;
    board->timer.blackTime = 600.0f;

    // 3. Reset des variables de jeu
    gameOver = false;
    winner = -1;
    selectedX = -1;
    selectedY = -1;
    currentTurn = 0;
}

// Fonction utilitaire pour recommencer une partie proprement
void GameReset(Board *board)
{
    GameInit(board);
}

// --- 4. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{
    // --- GESTION DU TIMER ---
    if (!gameOver)
    {
        if (currentTurn == 0)
            board->timer.whiteTime -= dt;
        else
            board->timer.blackTime -= dt;

        // Vérification fin du temps
        if (board->timer.whiteTime <= 0.0f)
        {
            board->timer.whiteTime = 0.0f;
            winner = 1; // Les Noirs gagnent
            gameOver = true;
            TraceLog(LOG_INFO, "GAME OVER - Temps Blancs écoulé");
        }

        if (board->timer.blackTime <= 0.0f)
        {
            board->timer.blackTime = 0.0f;
            winner = 0; // Les Blancs gagnent
            gameOver = true;
            TraceLog(LOG_INFO, "GAME OVER - Temps Noirs écoulé");
        }
    }

    // --- GESTION DES INPUTS ---
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        // Si la partie est finie, on ne peut plus jouer (clic = rien)
        if (gameOver) return;

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

        // Vérification qu'on est DANS le plateau
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

                // A. Annulation (Clic sur soi-même)
                if (dx == 0 && dy == 0)
                {
                    selectedX = -1;
                    selectedY = -1;
                    TraceLog(LOG_INFO, "Annulation");
                    return; 
                }
                
                // B. VÉRIFICATION PHYSIQUE (Règles de déplacement)
                
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
                        if (clickedTile->layerCount > 1) { 
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

                // C. VÉRIFICATION FINALE (Case cible occupée par un ami ?)
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
                            
                            // Petite vérif Game Over si on mange le Roi (Basic)
                            if (targetID == 10) { winner = 1; gameOver = true; } // Roi blanc mangé
                            if (targetID == 11) { winner = 0; gameOver = true; } // Roi noir mangé
                        }
                    }
                }

                // D. EXÉCUTION
                if (moveAllowed)
                {
                    int objID = TilePop(oldTile); 
                    TilePush(clickedTile, objID);

                    selectedX = -1;
                    selectedY = -1;
                    
                    // On ne change le tour que si la partie continue
                    if (!gameOver) {
                        currentTurn = 1 - currentTurn; 
                    }
                    
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
                DrawTexturePro(gTileTextures[idx],
                    (Rectangle){0, 0, gTileTextures[idx].width, gTileTextures[idx].height},
                    (Rectangle){drawX, drawY, tileSize, tileSize},
                    (Vector2){0,0}, 0, WHITE);
            }
            
            // Sélection
            if (x == selectedX && y == selectedY) DrawRectangleLines(drawX, drawY, tileSize, tileSize, GREEN);
            else DrawRectangleLines(drawX, drawY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
        }
    }

    // Affichage des Timers
    int whiteM = (int)board->timer.whiteTime / 60;
    int whiteS = (int)board->timer.whiteTime % 60;
    Color whiteColor = (currentTurn == 0 && !gameOver) ? BLUE : DARKGRAY;
    
    char whiteText[32];
    sprintf(whiteText, "BLANCS\n%02d:%02d", whiteM, whiteS);
    int whiteTextWidth = MeasureText("BLANCS", FONT_SIZE);
    DrawText(whiteText, offsetX - whiteTextWidth - TEXT_PADDING, centerTextY, FONT_SIZE, whiteColor);

    int blackM = (int)board->timer.blackTime / 60;
    int blackS = (int)board->timer.blackTime % 60;
    Color blackColor = (currentTurn == 1 && !gameOver) ? BLUE : DARKGRAY;
    
    char blackText[32];
    sprintf(blackText, "NOIRS\n%02d:%02d", blackM, blackS);
    DrawText(blackText, offsetX + boardW + TEXT_PADDING, centerTextY, FONT_SIZE, blackColor);

    // Affichage Game Over
    if (gameOver)
    {
        const char *text = (winner == 0) ? "VICTOIRE DES BLANCS" : "VICTOIRE DES NOIRS";
        int size = 40;
        int textW = MeasureText(text, size);
        
        // Fond semi-transparent
        DrawRectangle(0, screenH/2 - 50, screenW, 100, Fade(WHITE, 0.8f));
        // Texte
        DrawText(text, screenW/2 - textW/2, screenH/2 - size/2, size, RED);
        DrawText("Appuyez sur R pour recommencer", screenW/2 - MeasureText("Appuyez sur R pour recommencer", 20)/2, screenH/2 + 30, 20, DARKGRAY);
    }
}