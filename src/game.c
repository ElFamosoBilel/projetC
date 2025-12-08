#include "game.h"
#include <stdio.h> // Pour TraceLog et printf
#include <stdlib.h> // Pour abs()

// --- 0. IMPORT EXTERNE ---
extern Texture2D gTileTextures[]; 
extern int gTileTextureCount; 

// --- 1. VARIABLES GLOBALES ---
int selectedX = -1; // permet la selection
int selectedY = -1; // permet la selection
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

#define MAX_MOVES 64 // Taille maximale = toutes les cases
static int possibleMoves[MAX_MOVES][2]; // 64 coups possibles, avec leur coordonnées X et Y
static int possibleMoveCount = 0; // Initialise par défauts

// --- 2. FONCTIONS UTILITAIRES (Helpers) ---

static void TileClear(Tile *t) // initialise une case vide
{
    t->layerCount = 0;
    for(int i = 0; i < MAX_LAYERS; i++) {
        t->layers[i] = 0;
    }
}

static void TilePush(Tile *t, int textureIndex) // ajoute une texture
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

static bool IsMoveValid(const Board *board, int startX, int startY, int endX, int endY)
{
    // Ne pas bouger
    if (startX == endX && startY == endY) return false;
    
    Tile *oldTile = (Tile*)&board->tiles[startY][startX];
    Tile *targetTile = (Tile*)&board->tiles[endY][endX];
    
    // Si la case de départ est vide (ne devrait pas arriver, mais sécurité)
    if (oldTile->layerCount <= 1) return false;

    int pieceID = oldTile->layers[oldTile->layerCount - 1]; 
    int currentTurnColor = GetPieceColor(pieceID); 
    
    int dx = endX - startX; 
    int dy = endY - startY; 
    bool ruleMatch = false;

    // A. VÉRIFICATION RÈGLES PHYSIQUES
    if (pieceID == 12 || pieceID == 13) // Tour
    {
        if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0))
        {
            if (IsPathClear(board, startX, startY, endX, endY)) ruleMatch = true;
        }
    }
    else if (pieceID == 4 || pieceID == 5) // Fou
    {
        if (abs(dx) == abs(dy) && dx != 0)
        {
            if (IsPathClear(board, startX, startY, endX, endY)) ruleMatch = true;
        }
    }
    else if (pieceID == 8 || pieceID == 9) // Reine
    {
        if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0) || (abs(dx) == abs(dy)))
        {
            if (IsPathClear(board, startX, startY, endX, endY)) ruleMatch = true;
        }
    }
    else if (pieceID == 10 || pieceID == 11) // Roi
    {
        if (abs(dx) <= 1 && abs(dy) <= 1) ruleMatch = true;
    }
    else if (pieceID == 2 || pieceID == 3) // Cavalier
    {
        if ((abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1)) ruleMatch = true;
    }
    else if (pieceID == 6 || pieceID == 7) // Pion
    {
        int direction = (currentTurnColor == 0) ? -1 : 1; 
        int initialRow = (currentTurnColor == 0) ? 6 : 1; 
        
        // Capture Diagonale
        if (abs(dx) == 1 && dy == direction)
        {
            if (targetTile->layerCount > 1) { 
                int targetColor = GetPieceColor(targetTile->layers[targetTile->layerCount - 1]);
                if (targetColor != -1 && targetColor != currentTurnColor) ruleMatch = true; 
            }
        }
        // Avance 1 case
        else if (dx == 0 && dy == direction && targetTile->layerCount == 1) 
        {
            ruleMatch = true;
        }
        // Avance 2 cases (Premier tour)
        else if (dx == 0 && dy == 2 * direction && startY == initialRow && targetTile->layerCount == 1)
        {
            Tile *midTile = (Tile*)&board->tiles[startY + direction][startX];
            if (midTile->layerCount == 1) ruleMatch = true;
        }
    }

    if (!ruleMatch) return false;

    // B. VÉRIFICATION CONFLIT (Case occupée par un allié ?)
    if (targetTile->layerCount > 1)
    {
        int targetID = targetTile->layers[targetTile->layerCount - 1];
        int targetColor = GetPieceColor(targetID);

        if (targetColor != -1 && targetColor == currentTurnColor) // si y'a une pièce alliée
        {
            return false; // Bloqué par un ami
        }
    }

    return true; 
}

// --- NOUVEAU : FONCTION DE DÉTECTION D'ÉCHEC ---
// Placée ICI pour être connue avant GameLogicUpdate
static bool IsKingInCheck(const Board *board, int kingColor)
{
    int kingX = -1;
    int kingY = -1;
    int targetKingID = (kingColor == 0) ? 10 : 11; // 10=Blanc, 11=Noir

    // 1. Trouver le Roi
    for (int y = 0; y < BOARD_ROWS; y++) {
        for (int x = 0; x < BOARD_COLS; x++) {
            const Tile *t = &board->tiles[y][x];
            if (t->layerCount > 1) {
                if (t->layers[t->layerCount - 1] == targetKingID) {
                    kingX = x;
                    kingY = y;
                }
            }
        }
    }

    // Sécurité si roi pas trouvé (ne devrait pas arriver)
    if (kingX == -1 || kingY == -1) return false;

    // 2. Chercher les menaces
    for (int y = 0; y < BOARD_ROWS; y++) {
        for (int x = 0; x < BOARD_COLS; x++) {
            const Tile *t = &board->tiles[y][x];
            if (t->layerCount <= 1) continue; // Case vide

            int attackerID = t->layers[t->layerCount - 1];
            int attackerColor = GetPieceColor(attackerID);

            if (attackerColor == -1 || attackerColor == kingColor) continue; // Allié ou sol

            // Si cet ennemi peut manger le roi -> ECHEC
            if (IsMoveValid(board, x, y, kingX, kingY)) {
                return true;
            }
        }
    }
    return false;
}

// --- 3. INITIALISATION DU JEU ---
void GameInit(Board *board) // met en place l'échiquier
{
    // 1. Initialisation du plateau et des pièces
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            Tile *t = &board->tiles[y][x];
            TileClear(t); // vide la case

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
                if (x == 1 || x == 6) TilePush(t, 3); // Cavalier
                if (x == 2 || x == 5) TilePush(t, 5); // Fou
                if (x == 3) TilePush(t, 9); // Reine
                if (x == 4) TilePush(t, 11); // Roi
            }
            // Nobles Blancs (Ligne 7)
            if (y == 7)
            {
                if (x == 0 || x == 7) TilePush(t, 12); // Tour
                if (x == 1 || x == 6) TilePush(t, 2); // Cavalier
                if (x == 2 || x == 5) TilePush(t, 4); // Fou
                if (x == 3) TilePush(t, 8); // Reine
                if (x == 4) TilePush(t, 10); // Roi
            }
        }
    }
    
    // 2. Initialisation de l'état et du timer (10 minutes = 600s)
    board->timer.whiteTime = 600.0f; 
    board->timer.blackTime = 600.0f;
    board->state = STATE_PLAYING;
    board->winner = -1;
    currentTurn = 0; // Toujours commencer par les Blancs
    selectedX = -1;
    selectedY = -1;
    possibleMoveCount = 0; // Initialise par défaut
}

// Fonction utilitaire pour recommencer une partie proprement (Utilise GameInit qui fait tout le travail)
void GameReset(Board *board)
{
    GameInit(board);
}

// --- NOUVEAU : LOGIQUE DE JEU SEULEMENT ---
static void GameLogicUpdate(Board *board, float dt)
{
    // Gestion Souris
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition(); 

        // Calculs responsifs (inchangés)
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
                        TraceLog(LOG_INFO, "Selection OK"); 
                        
                        // Calcul des coups possibles pour l'affichage
                        possibleMoveCount = 0;
                        for (int py = 0; py < BOARD_ROWS; py++)
                        {
                            for (int px = 0; px < BOARD_COLS; px++)
                            {
                                if (IsMoveValid(board, selectedX, selectedY, px, py))
                                {
                                    // SIMULATION pour voir si le coup mettrait le roi en danger
                                    Board tempBoard = *board;
                                    Tile *sOld = &tempBoard.tiles[selectedY][selectedX];
                                    Tile *sNew = &tempBoard.tiles[py][px];
                                    int sPid = TilePop(sOld);
                                    if(sNew->layerCount > 1) TilePop(sNew);
                                    TilePush(sNew, sPid);
                                    
                                    if (!IsKingInCheck(&tempBoard, currentTurn)) {
                                        possibleMoves[possibleMoveCount][0] = px; // Note la coordonnées X
                                        possibleMoves[possibleMoveCount][1] = py; // Note la coordonnées Y
                                        possibleMoveCount++; // Augmente le compteur
                                    }
                                }
                            }
                        }  
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
                    possibleMoveCount = 0; // réinitialise
                    TraceLog(LOG_INFO, "Annulation");
                    return; 
                }
                
                // B. VÉRIFICATION PHYSIQUE (Règles de déplacement)
                if (IsMoveValid(board, startX, startY, endX, endY))
                {
                    moveAllowed = true;
                }

                // C. VÉRIFICATION CONFLIT (Ami ?)
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
                    }
                }

                // --- D. PROTECTION DU ROI (SIMULATION) ---
                if (moveAllowed)
                {
                    // 1. On crée une copie du plateau
                    Board tempBoard = *board; 

                    // 2. On joue le coup sur la copie
                    Tile *simOldTile = &tempBoard.tiles[startY][startX];
                    Tile *simNewTile = &tempBoard.tiles[endY][endX];

                    int simPieceID = TilePop(simOldTile); // Enlève départ
                    if (simNewTile->layerCount > 1) {
                        TilePop(simNewTile); // Enlève cible (mange)
                    }
                    TilePush(simNewTile, simPieceID); // Pose arrivée

                    // 3. On vérifie si le Roi est en danger sur la copie
                    if (IsKingInCheck(&tempBoard, currentTurn))
                    {
                        TraceLog(LOG_WARNING, "MOUVEMENT INTERDIT : Roi en echec !");
                        moveAllowed = false;
                    }
                }

                // E. EXÉCUTION
                if (moveAllowed)
                {
                    // Capture
                    if (clickedTile->layerCount > 1) {
                        int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                        // VÉRIFICATION ROI CAPTURÉ (Normalement impossible avec la protection D, mais sécurité)
                        if (targetID == 10 || targetID == 11) 
                        {
                            board->winner = currentTurn;
                            board->state = STATE_GAMEOVER;
                            TraceLog(LOG_INFO, "ROI CAPTURE ! PARTIE TERMINEE");
                        }
                        TilePop(clickedTile); // Mange l'ennemi
                    }

                    int objID = TilePop(oldTile); 
                    TilePush(clickedTile, objID); 

                    selectedX = -1; 
                    selectedY = -1;
                    possibleMoveCount = 0; // Réinitialise

                    if (board->state != STATE_GAMEOVER) // Ne change le tour que si la partie continue
                    {
                        currentTurn = 1 - currentTurn; // change le tour
                        TraceLog(LOG_INFO, "Coup valide.");
                    }
                }
                else
                {
                    // Si clic invalide sur une pièce alliée -> on change la sélection
                    if (clickedTile->layerCount > 1)
                    {
                        int targetColor = GetPieceColor(clickedTile->layers[clickedTile->layerCount - 1]);
                        if (targetColor == currentTurn)
                        {
                            selectedX = -1; 
                            possibleMoveCount = 0;
                            GameLogicUpdate(board, dt); // Récursion pour sélectionner instantanément
                            return; 
                        }
                    }
                    TraceLog(LOG_WARNING, "Mouvement invalide");
                }
            }
        }
        else // Si on clique en dehors du plateau, on annule la sélection
        {
            selectedX = -1;
            selectedY = -1;
            possibleMoveCount = 0;
        }
    }
}


// --- 4. MISE À JOUR (LOGIQUE) ---
void GameUpdate(Board *board, float dt)
{
    if (board->state == STATE_PLAYING)
    {
        // Gestion Timer
        if (currentTurn == 0) {
            if (board->timer.whiteTime > 0.0f) board->timer.whiteTime -= dt; 
        } else {
            if (board->timer.blackTime > 0.0f) board->timer.blackTime -= dt; 
        }

        // Fin de partie par temps
        if (board->timer.whiteTime <= 0.0f) {
            board->state = STATE_GAMEOVER;
            board->winner = 1; // Noir gagne au temps
            TraceLog(LOG_WARNING, "GAME OVER - Temps BLANC écoulé !");
            return;
        }
        if (board->timer.blackTime <= 0.0f) {
            board->state = STATE_GAMEOVER;
            board->winner = 0; // Blanc gagne au temps
            TraceLog(LOG_WARNING, "GAME OVER - Temps NOIR écoulé !");
            return;
        }
        
        // GESTION ABANDON (FORFAIT)
        if (IsKeyPressed(KEY_F))
        {
            board->state = STATE_GAMEOVER;
            board->winner = 1 - currentTurn; // Le gagnant est l'adversaire
            TraceLog(LOG_WARNING, "Le joueur %s a déclaré forfait (F).", (currentTurn == 0) ? "BLANC" : "NOIR");
            return;
        }
        // Logique de jeu (clics de pièces)
        GameLogicUpdate(board, dt);
    }
    else if (board->state == STATE_GAMEOVER)
    {
        // Gestion du clic dans le menu de fin de partie
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 m = GetMousePosition();
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();
            
            // Zone de détection du clic (approximative sur l'affichage "CLIQUEZ POUR REJOUER")
            int mWidth = MeasureText("CLIQUEZ POUR REJOUER", 30);
            int centerW = screenW / 2;
            int centerH = screenH / 2;

            // Détection du clic sur la zone "REJOUER"
            if (m.x > centerW - mWidth/2 - 10 && m.x < centerW + mWidth/2 + 10 && 
                m.y > centerH - 5 && m.y < centerH + 35) 
            {
                GameInit(board); // Relance une nouvelle partie
                TraceLog(LOG_INFO, "Nouvelle partie lancée.");
            }
        }
        // Alternative pour recommencer
        if (IsKeyPressed(KEY_R)) {
             GameInit(board);
             TraceLog(LOG_INFO, "Nouvelle partie lancée (Touche R).");
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

    // Dessin du plateau et des pièces
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
            
            // Sélection et bordures
            DrawRectangleLines(drawX, drawY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
        }
    }

    // Dessin des cases possibles
    for (int i = 0; i < possibleMoveCount; i++) 
    {
        int x = possibleMoves[i][0];
        int y = possibleMoves[i][1];
        int drawX = offsetX + x * tileSize;
        int drawY = offsetY + y * tileSize;
        
        // Indicateur visuel : un cercle pour indiquer la destination
        Color indicatorColor = Fade(DARKGRAY, 0.3f);
        
        // Si la case cible contient une pièce ennemi, on utilise un rectangle rouge
        const Tile *t = &board->tiles[y][x];
        if (t->layerCount > 1) {
            indicatorColor = Fade(RED, 0.6f);
            DrawRectangleLinesEx((Rectangle){(float)drawX, (float)drawY, (float)tileSize, (float)tileSize}, 5, indicatorColor);
        } else {
            // Sinon, un cercle pour les cases vides
            DrawCircle(drawX + tileSize / 2, drawY + tileSize / 2, tileSize / 8, indicatorColor);
        }
    }

    // Dessin de la sélection par dessus les indicateurs de mouvements
    if (selectedX != -1) 
    {
        int drawX = offsetX + selectedX * tileSize;
        int drawY = offsetY + selectedY * tileSize;
        DrawRectangleLinesEx((Rectangle){(float)drawX, (float)drawY, (float)tileSize, (float)tileSize}, 4, GREEN); 
    }

    // Dessin des Timers
    int whiteM = (int)board->timer.whiteTime / 60;
    int whiteS = (int)board->timer.whiteTime % 60;
    // La couleur active est un peu plus visible si l'état est "en cours de jeu"
    Color whiteColor = (currentTurn == 0 && board->state == STATE_PLAYING) ? RAYWHITE : DARKGRAY;
    DrawText(TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS), offsetX - MeasureText("BLANCS", FONT_SIZE) - TEXT_PADDING, centerTextY, FONT_SIZE, whiteColor); 

    int blackM = (int)board->timer.blackTime / 60;
    int blackS = (int)board->timer.blackTime % 60;
    Color blackColor = (currentTurn == 1 && board->state == STATE_PLAYING) ? RAYWHITE : DARKGRAY;
    DrawText(TextFormat("NOIRS\n%02d:%02d", blackM, blackS), offsetX + boardW + TEXT_PADDING, centerTextY, FONT_SIZE, blackColor); 
    
    // AFFICHAGE ÉCRAN FIN DE PARTIE 
    if (board->state == STATE_GAMEOVER)
    {
        int centerW = screenW / 2;
        int centerH = screenH / 2;
        
        // Fond semi-transparent
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.8f)); 
        
        const char *winnerName = (board->winner == 0) ? "BLANCS" : "NOIRS";
        const char *winnerText = TextFormat("VICTOIRE DES %s !", winnerName);
        const char *menuText = "CLIQUEZ / APPUYEZ SUR R POUR REJOUER";
        
        int wWidth = MeasureText(winnerText, 60);
        int mWidth = MeasureText(menuText, 30);
        
        // Affichage du gagnant
        DrawText(winnerText, centerW - wWidth/2, centerH - 100, 60, GREEN);
        
        // Bouton/Texte REJOUER
        DrawText(menuText, centerW - mWidth/2, centerH, 30, YELLOW);
        DrawRectangleLines(centerW - mWidth/2 - 10, centerH - 5, mWidth + 20, 40, YELLOW); // Cadre pour le bouton
        
        const char *quitText = "Appuyer sur ECHAP pour quitter";
        int qWidth = MeasureText(quitText, 20);
        DrawText(quitText, centerW - qWidth/2, centerH + 100, 20, RAYWHITE);
    }
}