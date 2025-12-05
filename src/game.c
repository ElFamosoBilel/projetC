#include "game.h"
#include <stdio.h> // Pour TraceLog et printf
#include <stdlib.h>

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
    board->timer.whiteTime = 600.0f;
    board->timer.blackTime = 600.0f;
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
// Vérifie si le chemin est libre (utile pour Fou/Reine/Tour)
// On utilise board pour accéder à l'ensemble des tuiles
static bool IsPathClear(const Board *board, int startX, int startY, int endX, int endY){
    // Déplacement horizontal
    if (startY == endY)
    {
        int step = (endX > startX) ? 1 : -1; // 1 pour aller vers la droite, -1 pour aller vers la gauche
        for (int x = startX + step; x != endX; x += step)
        {
            if (board->tiles[startY][x].layerCount > 1)
            {
                return false; // Obstacle trouvé
            }
        }
    }
    // Déplacement vertical
    else if (startX == endX)
    {
        int step = (endY > startY) ? 1 : -1; // 1 pour aller vers le bas, -1 pour aller vers le haut
        for (int y = startY + step; y != endY; y += step)
        {
            if (board->tiles[y][startX].layerCount > 1)
            {
                return false;
            }
        }
    }
    // Déplacement diagonal
    else if (abs(endX - startX) == abs(endY - startY))
    {
        int stepX = (endX > startX) ? 1 : -1; // Direction X 1 = droite -1 = gauche
        int stepY = (endY > startY) ? 1 : -1; // Direction Y 1 = bas -1 = haut

        int x = startX + stepX;
        int y = startY + stepY;

        while (x != endX)
        {
            if (board->tiles[y][x].layerCount > 1)
            {
                return false; // Obstacle rencontré
            }
            x += stepX;
            y += stepY;
        }
    }
    return true; // Le chemin est libre
}

// --- 4. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{

    if (currentTurn == 0) // si c'est le tour des BLancs
    {
        if (board->timer.whiteTime > 0.0f) {
            board->timer.whiteTime -= dt;
        }  
    }
    else // si c'est le tour des Noirs
    {
        if (board->timer.blackTime > 0.0f) {
            board->timer.blackTime -= dt;
        }
    }

    if (board->timer.whiteTime <= 0.0f || board->timer.blackTime <= 0.0f)
    {
        TraceLog(LOG_WARNING, "GAME OVER - Temps écoulé !");
    }

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
                    // On vérifie la pièce et la couleur
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

                Tile *oldTile = &board->tiles[selectedY][selectedX];
                int pieceID = oldTile->layers[oldTile->layerCount - 1];

                int startX = selectedX;
                int startY = selectedY;
                int endX = x;
                int endY = y;

                // Mouvement absolu
                int dx = endX - startX;
                int dy = endY - startY;

                // A. On reclique sur soi-même -> Annulation
                if (dx == 0 && dy == 0)
                {
                    selectedX = -1;
                    selectedY = -1;
                    TraceLog(LOG_INFO, "Annulation");
                    return; // Sort de la fonction pour ne pas executer le reste
                }
                
                // --- LOGIQUE DES PIÈCES ---
                // Tour (IDs 12:Blanche, 13:Noire)
                if (pieceID == 12 || pieceID == 13)
                {
                    // Est-ce un mouvement en ligne droite (horizontal ou vertical) ?
                    if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0))
                    {
                        // Le chemin est-il libre d'obstacles ?
                        if (IsPathClear(board, startX, startY, endX, endY))
                        {
                            moveAllowed = true;
                        }
                    }
                }

                // Fou (IDs 4:Blanc, 5:Noir)
                else if (pieceID == 4 || pieceID == 5)
                {
                    // Mouvement diagonale ?
                    if (abs(dx) == abs(dy) && dx != 0) // dx != 0 pour enlever le non-mouvement
                    {
                        // Chemin libre ?
                        if (IsPathClear(board, startX, startY, endX, endY))
                        {
                            moveAllowed = true;
                        }
                    }
                }

                // Reine (IDs 8:Blanche, 9:Noire)
                else if (pieceID == 8 || pieceID == 9)
                {
                    if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0) || (abs(dx) == abs(dy)))
                    {
                        // Chemin libre ?
                        if (IsPathClear(board, startX, startY, endX, endY))
                        {
                            moveAllowed = true;
                        }
                    }
                }

                // Roi (IDs 10:Blanc, 11:Noir)
                else if (pieceID == 10 || pieceID == 11)
                {
                    //Mouvement d'une seule case dans n'importe quelle direction
                    if (abs(dx) <= 1 && abs(dy) <= 1 && (dx !=0 || dy != 0))
                    {
                        moveAllowed = true;
                    }
                }

                // Cavalier (IDs 2:Blanc, 3:Noir)
                else if (pieceID == 2 || pieceID == 3)
                {
                    // Mouvement en L
                    if ((abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1))
                    {
                        // Saute pas dessus les autres donc pas besoin de IsPathClear
                        moveAllowed = true;
                    }
                }

                // Pion (IDs 6:Blanc, 7:Noir) 
                else if (pieceID == 6 || pieceID == 7)
                {
                    int direction = (currentTurn == 0) ? -1 : 1; 
                    int initialRow = (currentTurn == 0) ? 6 : 1;
                    // Capture en diagonale
                    if (abs(dx) == 1 && dy == direction)
                    {
                        int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                        int targetColor = GetPieceColor(targetID);
                        // Case occupée par un ennemi
                        if (clickedTile->layerCount > 1 && targetColor != -1 && targetColor != currentTurn)
                        {
                            moveAllowed = true;
                        }
                    }
                    // Avance de deux cases (uniquement au départ) et chemin libre/cible vide
                    // Avance vertical
                    else if (dx == 0) 
                    {
                        // Avance d'une case
                        if (dy == direction && clickedTile->layerCount == 1) 
                        {
                            moveAllowed = true;
                        }
                        // Avance de 2 cases
                        else if (dy == 2 * direction && startY == initialRow && clickedTile->layerCount == 1)
                        {
                            // Vérifie que la case intermédiaire est aussi vide
                            Tile *midTile = &board->tiles[startY + direction][startX];
                            if (midTile->layerCount == 1)
                            {
                                moveAllowed = true;
                            }
                        }
                    }
                }

                if (moveAllowed)
                {
                    // Vérifie la case
                    if (clickedTile->layerCount > 1) // La case est occupée
                    {
                        int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                        int targetColor = GetPieceColor(targetID);

                        // Si la pièce est un alliée, le mouvement devient invalide
                        if (targetColor != -1 && targetColor == currentTurn)
                        {
                            TraceLog(LOG_INFO, "Mouvement invalide: la case contient déjà une de vos pièces.");
                            moveAllowed = false; // Annule le mouvement
                        }
                        // Si la pièce est un ennemie
                        else if (targetColor != -1 && targetColor != currentTurn)
                        {
                            TraceLog(LOG_WARNING, "BAGARRE ! Piece adverse eliminee.");
                            TilePop(clickedTile); // On mange la pièce
                        }
                    }
                }
                // C. Exécution du mouvement
                if (moveAllowed)
                {
                    // 1. On prend notre pièce de l'ancienne case
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
                else
                {
                    TraceLog(LOG_WARNING, "Mouvement invalide pour cette piece");
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

    const int FONT_SIZE = 30; // taille de la police
    const int TEXT_PADDING = 20; // marge entre le plateau et le texte

    // calcul ed la position verticale centrée
    int centerTextY = offsetY + boardH / 2 - FONT_SIZE / 2;
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
            
            // Temps des blancs à gauche du plateau
            int whiteM = (int)board->timer.whiteTime / 60;
            int whiteS = (int)board->timer.whiteTime % 60;
            Color whiteColor = (currentTurn == 0) ? BLUE : DARKGRAY; // Met en évidence le joueur qui doit jouer
            char whiteText[32];
            TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS, whiteText); // \n pour mettre a la ligne
            int whiteTextWidth = MeasureText("BLANCS", FONT_SIZE);

            DrawText(TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS),
                    offsetX - whiteTextWidth - TEXT_PADDING,
                    centerTextY,
                    FONT_SIZE, whiteColor);

            // Temps des noirs à droite du plateau
            int blackM = (int)board->timer.blackTime / 60;
            int blackS = (int)board->timer.blackTime % 60;
            Color blackColor = (currentTurn == 1) ? BLUE : DARKGRAY; // Met en évidence le joueur qui doit jouer

            DrawText(TextFormat("NOIRS\n%02d:%02d", blackM, blackS),
                    offsetX + boardW + TEXT_PADDING,
                    centerTextY,
                    FONT_SIZE, blackColor);
            


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