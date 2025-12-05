#include "game.h"
#include <stdio.h> // Pour TraceLog et printf
#include <stdlib.h> // pour la fonction abs()

// --- 0. IMPORT EXTERNE ---
extern Texture2D gTileTextures[]; // accède au image depuis le dossier depuis le main
extern int gTileTextureCount; // déclare le nombre de textures disponibles

// --- 1. VARIABLES GLOBALES ---
int selectedX = -1; // permet la selection
int selectedY = -1; // permet la selection
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

// --- 2. FONCTIONS UTILITAIRES (Helpers) ---

static void TileClear(Tile *t) // initialise une case vide
{
    t->layerCount = 0;
    for(int i = 0; i < MAX_LAYERS; i++) {
        t->layers[i] = 0;
    }
}

static void TilePush(Tile *t, int textureIndex)  // ajoute une texture
{
    if (t->layerCount < MAX_LAYERS) {
        t->layers[t->layerCount] = textureIndex;
        t->layerCount++;
    }
}

static int TilePop(Tile *t) // supprime une texture
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

// Vérifie si le chemin est libre (Tour, Reine, Fou)
static bool IsPathClear(const Board *board, int startX, int startY, int endX, int endY){
    // Déplacement horizontal
    if (startY == endY) // si c'est purement un mouvement horrizontal
    {
        int step = (endX > startX) ? 1 : -1; // si on va à droite ou à gauche
        for (int x = startX + step; x != endX; x += step)
        {
            if (board->tiles[startY][x].layerCount > 1) return false; // fait toute la colonne
        }
    }
    // Déplacement vertical
    else if (startX == endX)
    {
        int step = (endY > startY) ? 1 : -1; // si on va en bas ou en haut
        for (int y = startY + step; y != endY; y += step)
        {
            if (board->tiles[y][startX].layerCount > 1) return false; // fait toute la ligne
        }
    }
    // Déplacement diagonal
    else if (abs(endX - startX) == abs(endY - startY)) // si c'est purement horizontal
    {
        int stepX = (endX > startX) ? 1 : -1; // diagonale droite ou gauche
        int stepY = (endY > startY) ? 1 : -1; // diagonale bas ou haut

        int x = startX + stepX; // la case de départ
        int y = startY + stepY; // la cade de départ

        while (x != endX) // On s'arrête avant la case finale
        {
            if (board->tiles[y][x].layerCount > 1) return false; // vérifie si c'est bloqué par une pièce
            x += stepX; // avance de colonne
            y += stepY; // avance de ligne
        }
    }
    // Si ce n'est ni l'un ni l'autre (ex: Cavalier), on considère que le chemin n'est pas "bloquable" par cette fonction
    return true; 
}

// --- 3. INITIALISATION DU JEU ---
void GameInit(Board *board) // met en place l'échiquier
{
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            Tile *t = &board->tiles[y][x];
            TileClear(t); // vide la case

            // Sol
            int groundIndex = (x + y) % 2; // case noire / blanche
            TilePush(t, groundIndex);

            // Pions
            if (y == 1) TilePush(t, 7); // Noir
            if (y == 6) TilePush(t, 6); // Blanc

            // Nobles Noirs
            if (y == 0)
            {
                if (x == 0 || x == 7) TilePush(t, 13); // Tour
                if (x == 1 || x == 6) TilePush(t, 3);  // Cavalier
                if (x == 2 || x == 5) TilePush(t, 5);  // Fou
                if (x == 3) TilePush(t, 9);            // Reine
                if (x == 4) TilePush(t, 11);           // Roi
            }
            // Nobles Blancs
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
    board->timer.whiteTime = 600.0f; // timer du joueur Blanc
    board->timer.blackTime = 600.0f; // timer du joueur Noir
}

// --- 4. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{
    // Gestion Timer
    if (currentTurn == 0) {
        if (board->timer.whiteTime > 0.0f) board->timer.whiteTime -= dt; // si c'est au tour du joueur BLanc :
    } else {
        if (board->timer.blackTime > 0.0f) board->timer.blackTime -= dt; // si c'est au tour du joueur Noir :
    }

    if (board->timer.whiteTime <= 0.0f || board->timer.blackTime <= 0.0f) {
        TraceLog(LOG_WARNING, "GAME OVER - Temps écoulé !"); // si le temps d'un des deux joueurs arrive à 0
    }

    // Gestion Souris
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition(); // récupère la position de la souris

        // Calculs responsifs
        int screenW = GetScreenWidth(); // largeur de la fenêtre
        int screenH = GetScreenHeight(); // longueur de la fenêtre

        int tileSizeW = screenW / BOARD_COLS; // Calcul la taille potentielle de la case en fonction de la largeur
        int tileSizeH = screenH / BOARD_ROWS; // Calcul la taille potentielle de la case en fonction de la longueur

        int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH; // taille réel de la case

        int boardW = tileSize * BOARD_COLS; // Largeur du tableau
        int boardH = tileSize * BOARD_ROWS; // Longueur du tableau

        int offsetX = (screenW - boardW) / 2; // centrage du tableau
        int offsetY = (screenH - boardH) / 2; // centrage du tableau

        int x = (int)((m.x - offsetX) / tileSize); // Convertit les coordonnées de la souris en fonction de la colonne
        int y = (int)((m.y - offsetY) / tileSize); // Convertit les coordonnées de la souris en fonction de la ligne

        if (x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS) // Vérifie les limites de l'échiquier, quelles soient valides
        {
            Tile *clickedTile = &board->tiles[y][x];

            // --- CAS 1 : SÉLECTION ---
            if (selectedX == -1) // Si mode sélection
            {
                if (clickedTile->layerCount > 1) // si la case contient une pièce
                {
                    int pieceID = clickedTile->layers[clickedTile->layerCount - 1]; // vérifie la pièce
                    int pieceColor = GetPieceColor(pieceID); // verifie la couleur de la pièce

                    if (pieceColor == currentTurn) // est-ce que la couleur de la pièce correspond à celle du joueur entrain de jouer ?
                    {
                        selectedX = x;
                        selectedY = y;
                        TraceLog(LOG_INFO, "Selection OK"); // sélectionne la pièce
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

                Tile *oldTile = &board->tiles[selectedY][selectedX]; // Pointeur vers la case de départ
                int pieceID = oldTile->layers[oldTile->layerCount - 1]; // récupère l'ID de la pièce qui va être déplacé

                int startX = selectedX; // Coordonnées de départ
                int startY = selectedY; // Coordonnées de départ
                int endX = x; // Coordonnées d'arrivée
                int endY = y; // Coordonnées d'arrivée
                int dx = endX - startX; // Différencce de colonne
                int dy = endY - startY; // Différence de ligne

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
                    int direction = (currentTurn == 0) ? -1 : 1; // si il descend ou monte en fonction de la couleur
                    int initialRow = (currentTurn == 0) ? 6 : 1; // La colonne précise
                    
                    // Capture Diagonale
                    if (abs(dx) == 1 && dy == direction)
                    {
                        if (clickedTile->layerCount > 1) { // Si la case contient quelque chose
                            int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                            int targetColor = GetPieceColor(targetID);
                            if (targetColor != -1 && targetColor != currentTurn) moveAllowed = true; // si c'est une pièce ennemi en diagonal, on mange
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

                        if (targetColor != -1 && targetColor == currentTurn) // si y'a une pièce alliée
                        {
                            TraceLog(LOG_INFO, "Bloqué par un ami.");
                            moveAllowed = false; 
                        }
                        else if (targetColor != -1 && targetColor != currentTurn) // si y'a une pièce ennemi
                        {
                            TraceLog(LOG_INFO, "Capture !");
                            TilePop(clickedTile); // Mange l'ennemi
                        }
                    }
                }

                // --- EXÉCUTION ---
                if (moveAllowed)
                {
                    int objID = TilePop(oldTile); // retire la pièce
                    TilePush(clickedTile, objID); // ajoute la pièce

                    selectedX = -1; // réinitialise la sélection
                    selectedY = -1;
                    currentTurn = 1 - currentTurn; // change le tour
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
}

// --- 5. DESSIN DU JEU ---
void GameDraw(const Board *board)
{
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
                DrawTexturePro(gTileTextures[idx],(Rectangle){0, 0, gTileTextures[idx].width, gTileTextures[idx].height},(Rectangle){drawX, drawY, tileSize, tileSize},(Vector2){0,0},0,WHITE); // affiche d'abord le sol puis la pièce
            }
            
            if (x == selectedX && y == selectedY) DrawRectangleLines(drawX, drawY, tileSize, tileSize, GREEN); // si une case est sélectionné, un petit carré vert apparaît
            else DrawRectangleLines(drawX, drawY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
        }
    }

    int whiteM = (int)board->timer.whiteTime / 60;
    int whiteS = (int)board->timer.whiteTime % 60;
    Color whiteColor = (currentTurn == 0) ? RAYWHITE : DARKGRAY;
    DrawText(TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS), offsetX - MeasureText("BLANCS", FONT_SIZE) - TEXT_PADDING, centerTextY, FONT_SIZE, whiteColor); // Affiche le timer des Blancs

    int blackM = (int)board->timer.blackTime / 60;
    int blackS = (int)board->timer.blackTime % 60;
    Color blackColor = (currentTurn == 1) ? RAYWHITE : DARKGRAY;
    DrawText(TextFormat("NOIRS\n%02d:%02d", blackM, blackS), offsetX + boardW + TEXT_PADDING, centerTextY, FONT_SIZE, blackColor); // Affiche le timer des Noirs
}