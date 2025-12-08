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
int promotionPending = 0; // 0 = pas de promotion, 1 = en attente de choix 
int promotionX = -1;
int promotionY = -1;
int promotionColor = -1;

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
// MODIFICATION : Utilisation de IF / ELSE classiques au lieu de ternaires
static bool IsPathClear(const Board *board, int startX, int startY, int endX, int endY)
{
    // Déplacement horizontal (Y ne change pas)
    if (startY == endY) 
    {
        int step;
        if (endX > startX) {
            step = 1;
        } else {
            step = -1;
        }

        for (int x = startX + step; x != endX; x += step)
        {
            if (board->tiles[startY][x].layerCount > 1) return false;
        }
    }
    // Déplacement vertical (X ne change pas)
    else if (startX == endX)
    {
        int step;
        if (endY > startY) {
            step = 1;
        } else {
            step = -1;
        }

        for (int y = startY + step; y != endY; y += step)
        {
            if (board->tiles[y][startX].layerCount > 1) return false;
        }
    }
    // Déplacement diagonal
    else if (abs(endX - startX) == abs(endY - startY)) 
    {
        int stepX;
        if (endX > startX) stepX = 1; else stepX = -1;

        int stepY;
        if (endY > startY) stepY = 1; else stepY = -1;

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
    
    // 2. Initialisation de l'état et du timer (10 minutes = 600s)
    board->timer.whiteTime = 600.0f;
    board->timer.blackTime = 600.0f;
    board->state = STATE_PLAYING;
    board->winner = -1;
    currentTurn = 0; // Toujours commencer par les Blancs
    selectedX = -1;
    selectedY = -1;
}

// Fonction utilitaire pour recommencer une partie proprement
void GameReset(Board *board)
{
    GameInit(board);
}

// --- LOGIQUE DE JEU SEULEMENT ---
static void GameLogicUpdate(Board *board, float dt)
{
    // === GESTION DU MENU DE PROMOTION (Refactorisé) ===
    if (promotionPending == 1) 
    {
        Tile *promTile = &board->tiles[promotionY][promotionX];
        bool selected = false;
        int newPieceIdx = -1;

        // Touche 1 : Reine
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) 
        {
            newPieceIdx = (promotionColor == 1) ? 9 : 8; // 9=Noir, 8=Blanc
            selected = true;
            TraceLog(LOG_INFO, "Promotion : Reine choisie");
        }
        // Touche 2 : Cavalier
        else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2))
        {
            newPieceIdx = (promotionColor == 1) ? 3 : 2;
            selected = true;
            TraceLog(LOG_INFO, "Promotion : Cavalier choisi");
        }
        // Touche 3 : Tour
        else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))
        {
            newPieceIdx = (promotionColor == 1) ? 13 : 12;
            selected = true;
            TraceLog(LOG_INFO, "Promotion : Tour choisie");
        }
        // Touche 4 : Fou
        else if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4))
        {
            newPieceIdx = (promotionColor == 1) ? 5 : 4;
            selected = true;
            TraceLog(LOG_INFO, "Promotion : Fou choisi");
        }

        // Si un choix a été fait
        if (selected)
        {
            TilePop(promTile);       // Retire le pion
            TilePush(promTile, newPieceIdx); // Ajoute la nouvelle pièce
            
            promotionPending = 0;
            selectedX = -1;
            selectedY = -1;
            currentTurn = 1 - currentTurn; // On change le tour maintenant
        }

        // Bloque les autres clics pendant la promotion
        return;
    }

    // Gestion Souris
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition();
        
        // Calculs responsifs
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
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Pas ton tour !");
                    }
                }
            }
            // --- CAS 2 : ACTION (DÉPLACEMENT) ---
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
                
                // Tour (12=Blanc, 13=Noir)
                if (pieceID == 12 || pieceID == 13)
                {
                    if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0))
                    {
                        if (IsPathClear(board, startX, startY, endX, endY)) moveAllowed = true;
                    }
                }
                // Fou (4=Blanc, 5=Noir)
                else if (pieceID == 4 || pieceID == 5)
                {
                    if (abs(dx) == abs(dy) && dx != 0)
                    {
                        if (IsPathClear(board, startX, startY, endX, endY)) moveAllowed = true;
                    }
                }
                // Reine (8=Blanc, 9=Noir)
                else if (pieceID == 8 || pieceID == 9)
                {
                    if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0) || (abs(dx) == abs(dy)))
                    {
                        if (IsPathClear(board, startX, startY, endX, endY)) moveAllowed = true;
                    }
                }
                // Roi (10=Blanc, 11=Noir)
                else if (pieceID == 10 || pieceID == 11)
                {
                    if (abs(dx) <= 1 && abs(dy) <= 1) moveAllowed = true;
                }
                // Cavalier (2=Blanc, 3=Noir)
                else if (pieceID == 2 || pieceID == 3)
                {
                    if ((abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1)) moveAllowed = true;
                }
                // Pion (6=Blanc, 7=Noir)
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

                        if (targetColor != -1 && targetColor == currentTurn) // si y'a une pièce alliée
                        {
                            TraceLog(LOG_INFO, "Bloqué par un ami.");
                            moveAllowed = false; 
                        }
                        else if (targetColor != -1 && targetColor != currentTurn) // si y'a une pièce ennemi
                        {
                            TraceLog(LOG_INFO, "Capture !");
                            // VÉRIFICATION ROI CAPTURÉ 
                            if (targetID == 10 || targetID == 11) 
                            {
                                board->winner = currentTurn;
                                board->state = STATE_GAMEOVER;
                                TraceLog(LOG_INFO, "ROI CAPTURE ! PARTIE TERMINEE");
                            }
                            TilePop(clickedTile); // Mange l'ennemi
                        }
                    }
                }

                // D. EXÉCUTION
                if (moveAllowed)
                {
                    int objID = TilePop(oldTile);

                    // PROMOTION PION NOIR (atteint ligne 7 = bas)
                    if (objID == 7 && endY == 7)
                    {
                        TraceLog(LOG_INFO, " Promotion du pion noir disponible !");
                        TilePush(clickedTile, objID); 
                        promotionPending = 1;
                        promotionX = endX;
                        promotionY = endY;
                        promotionColor = 1; // noir
                        // ne change pas de tour, il se fera apres le choix 
                    }
                    // PROMOTION PION BLANC (atteint ligne 0 = haut)
                    else if (objID == 6 && endY == 0 )
                    {
                        TraceLog(LOG_INFO," Promotion du pion blanc disponible !");
                        TilePush(clickedTile, objID);
                        promotionPending = 1;
                        promotionX = endX;
                        promotionY = endY;
                        promotionColor = 0; // blanc
                        // ne change pas le tour, il se fera apres le choix 
                    }
                    // DÉPLACEMENT STANDARD
                    else
                    {
                        TilePush(clickedTile, objID);
                        selectedX = -1; 
                        selectedY = -1;
                        
                        if (board->state != STATE_GAMEOVER) // Ne change le tour que si la partie continue
                        {
                            currentTurn = 1 - currentTurn;
                            TraceLog(LOG_INFO, "Coup valide.");
                        }
                    }
                }
                else
                {
                    TraceLog(LOG_WARNING, "Mouvement invalide");
                }
            }
        }
        else // Si on clique en dehors du plateau, on annule la sélection
        {
            selectedX = -1;
            selectedY = -1;
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
        if (IsKeyPressed(KEY_SPACE))
        {
            board->state = STATE_GAMEOVER;
            board->winner = 1 - currentTurn; // Le gagnant est l'adversaire
            TraceLog(LOG_WARNING, "Le joueur %s a déclaré forfait (Espace).", (currentTurn == 0) ? "BLANC" : "NOIR");
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
            
            // Zone de détection du clic "REJOUER"
            int mWidth = MeasureText("CLIQUEZ POUR REJOUER", 30);
            int centerW = screenW / 2;
            int centerH = screenH / 2;

            if (m.x > centerW - mWidth/2 - 10 && m.x < centerW + mWidth/2 + 10 && 
                m.y > centerH - 5 && m.y < centerH + 35) 
            {
                GameInit(board);
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

    // --- 1. Dessin du plateau et des pièces ---
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
                DrawTexturePro(gTileTextures[idx],
                    (Rectangle){0, 0, gTileTextures[idx].width, gTileTextures[idx].height},
                    (Rectangle){drawX, drawY, tileSize, tileSize},
                    (Vector2){0,0},0,WHITE); 
            }
            
            // Sélection et bordures
            if (x == selectedX && y == selectedY) {
                DrawRectangleLines(drawX, drawY, tileSize, tileSize, GREEN);
            } else {
                DrawRectangleLines(drawX, drawY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
            }
        }
    }

    // --- 2. Dessin des Timers ---
    int whiteM = (int)board->timer.whiteTime / 60;
    int whiteS = (int)board->timer.whiteTime % 60;
    Color whiteColor = (currentTurn == 0 && board->state == STATE_PLAYING) ? RAYWHITE : DARKGRAY;
    DrawText(TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS), offsetX - MeasureText("BLANCS", FONT_SIZE) - TEXT_PADDING, centerTextY, FONT_SIZE, whiteColor);

    int blackM = (int)board->timer.blackTime / 60;
    int blackS = (int)board->timer.blackTime % 60;
    Color blackColor = (currentTurn == 1 && board->state == STATE_PLAYING) ? RAYWHITE : DARKGRAY;
    DrawText(TextFormat("NOIRS\n%02d:%02d", blackM, blackS), offsetX + boardW + TEXT_PADDING, centerTextY, FONT_SIZE, blackColor);

    // --- 4. ECRAN FIN DE PARTIE ---
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
        DrawRectangleLines(centerW - mWidth/2 - 10, centerH - 5, mWidth + 20, 40, YELLOW);
        
        const char *quitText = "Appuyer sur ECHAP pour quitter";
        int qWidth = MeasureText(quitText, 20);
        DrawText(quitText, centerW - qWidth/2, centerH + 100, 20, RAYWHITE);
    }
    // AFFICHAGE MENU DE PROMOTION
    if (promotionPending == 1) 
    {
        // Fond semi-transparent
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.7f));

        // Titre dynamique selon la couleur
        const char *title = (promotionColor == 1) ? "PROMOTION DU PION NOIR" : "PROMOTION DU PION BLANC";
        int titleWidth = MeasureText(title, 40);
        DrawText(title, screenW/2 - titleWidth/2, screenH/2 - 150, 40, YELLOW);

        // Options
        DrawText("Choisissez votre pièce :", screenW/2 - 150, screenH/2 - 80, 30, WHITE);
        DrawText("1 - Reine", screenW/2 - 100, screenH/2 - 40, 25, RAYWHITE);
        DrawText("2 - Cavalier", screenW/2 - 100, screenH/2, 25, RAYWHITE);
        DrawText("3 - Tour", screenW/2 - 100, screenH/2 + 40, 25, RAYWHITE);
        DrawText("4 - Fou", screenW/2 - 100, screenH/2 + 80, 25, RAYWHITE);
    }
}