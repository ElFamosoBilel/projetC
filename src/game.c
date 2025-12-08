#include "game.h"
#include <stdio.h> 
#include <stdlib.h>  // Pour la fonction (abs)

// IMPORT EXTERNE 
extern Texture2D gTileTextures[]; 
extern int gTileTextureCount; 

// VARIABLES GLOBALES 
int selectedX = -1; 
int selectedY = -1; 
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

// FONCTIONS UTILITAIRES

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

        while (x != endX) 
        {
            if (board->tiles[y][x].layerCount > 1) return false; 
            x += stepX; 
            y += stepY; 
        }
    }
    // Si ce n'est ni l'un ni l'autre (ex: Cavalier), on considère que le chemin n'est pas "bloquable" par cette fonction
    return true; 
}

// INITIALISATION DU JEU
void GameInit(Board *board) // met en place l'échiquier
{
    for (int y = 0; y < BOARD_ROWS; y++) // Lignes
    {
        for (int x = 0; x < BOARD_COLS; x++) // Colonnes
        {
            Tile *t = &board->tiles[y][x];
            TileClear(t); // vide la case

            // Sol
            int groundIndex = (x + y) % 2; 
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
    
    // Initialisation de l'état
    board->timer.whiteTime = 600.0f; // 10 minutes pour le joueur Blanc
    board->timer.blackTime = 600.0f; // 10 minutes pour le joueur Noir
    board->state = STATE_MAIN_MENU; // Sur le menu par défaut
    board->mode = MODE_NONE; // Aucun mode choisi par défaut
    board->winner = -1; // Aucun gagnant défini
    currentTurn = 0; // Toujours commencer par les Blancs
    selectedX = -1; // Aucune sélection
    selectedY = -1; // Aucune sélection
}

// LOGIQUE DE JEU
static void GameLogicUpdate(Board *board, float dt)
{
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

            // CAS 1 : SÉLECTION 
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
            // CAS 2 : ACTION
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
                
                // VÉRIFICATION PHYSIQUE (Tour, Fou, Reine, Roi, Cavalier, Pion)
                
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

                // VÉRIFICATION FINALE (Case occupée par un ami ?)
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
                            }
                            TilePop(clickedTile); // Mange l'ennemi
                        }
                    }
                }

                // EXÉCUTION
                if (moveAllowed)
                {
                    int objID = TilePop(oldTile); 
                    TilePush(clickedTile, objID); 

                    selectedX = -1; 
                    selectedY = -1;
                    
                    if (board->state != STATE_GAMEOVER) // Ne change le tour que si la partie continue
                    {
                        currentTurn = 1 - currentTurn; // change le tour
                        TraceLog(LOG_INFO, "Coup valide.");
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
    if (board->state == STATE_MAIN_MENU)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 m = GetMousePosition();
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();

            int buttonWidth = 300;
            int buttonHeight = 60;
            int centerW = screenW / 2;
            int startY = screenH / 2 - 100;

            Rectangle rectPvP = { (float)centerW - buttonWidth / 2, (float)startY, (float)buttonWidth, (float)buttonHeight };
            Rectangle rectPvA = { (float)centerW - buttonWidth / 2, (float)startY + 80, (float)buttonWidth, (float)buttonHeight };
            Rectangle rectQuit = { (float)centerW - buttonWidth / 2, (float)startY + 160, (float)buttonWidth, (float)buttonHeight };

            if (CheckCollisionPointRec(m , rectPvP))
            {
                board->mode = MODE_PLAYER_VS_PLAYER;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "Mode 1 vs 1 sélectionné. Début de la partie.");
            }
            
            else if (CheckCollisionPointRec(m , rectPvA))
            {
                board->mode = MODE_PLAYER_VS_IA;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "Mode vs IA sélectionné. Début de la partie.");
            }
            
            else if (CheckCollisionPointRec(m , rectQuit))
            {
                CloseWindow();
                TraceLog(LOG_INFO, "Quitter cliqué.");
            }
        }
    }
    else if (board->state == STATE_PLAYING)
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
            
            // Détection du clic sur la zone "REJOUER"
            // Zone approximative : Centre de l'écran, là où le texte "REJOUER" est affiché
            if (m.x > screenW/2 - 150 && m.x < screenW/2 + 150 && m.y > screenH/2 - 30 && m.y < screenH/2 + 30) 
            {
                GameInit(board); // Relance une nouvelle partie
                TraceLog(LOG_INFO, "Nouvelle partie lancée.");
            }
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
    if (board->state == STATE_MAIN_MENU) // NOUVEAU : Dessin du menu principal
    {
        int centerW = screenW / 2;
        int centerH = screenH / 2;
        int buttonWidth = 300;
        int buttonHeight = 60;
        int buttonFontSize = 30;
        int startY = centerH - 100;

        // Titre
        const char *titleText = "JEU D'ÉCHECS C";
        int titleWidth = MeasureText(titleText, 50);
        DrawText(titleText, centerW - titleWidth/2, startY - 100, 50, RAYWHITE);
        
        // Boutons
        
        // 1 vs 1
        Rectangle rectPvP = { (float)centerW - buttonWidth/2, (float)startY, (float)buttonWidth, (float)buttonHeight };
        DrawRectangleRec(rectPvP, LIGHTGRAY);
        DrawRectangleLinesEx(rectPvP, 3, RAYWHITE);
        const char *textPvP = "Jouer (1 vs 1)";
        int textPvPWidth = MeasureText(textPvP, buttonFontSize);
        DrawText(textPvP, centerW - textPvPWidth/2, startY + (buttonHeight - buttonFontSize) / 2, buttonFontSize, BLACK);

        // Contre IA
        Rectangle rectPvA = { (float)centerW - buttonWidth/2, (float)startY + 80, (float)buttonWidth, (float)buttonHeight };
        DrawRectangleRec(rectPvA, GRAY);
        DrawRectangleLinesEx(rectPvA, 3, RAYWHITE);
        const char *textPvA = "Jouer contre IA";
        int textPvAWidth = MeasureText(textPvA, buttonFontSize);
        DrawText(textPvA, centerW - textPvAWidth/2, startY + 80 + (buttonHeight - buttonFontSize) / 2, buttonFontSize, DARKGRAY);

        // Quitter
        Rectangle rectQuit = { (float)centerW - buttonWidth/2, (float)startY + 160, (float)buttonWidth, (float)buttonHeight };
        DrawRectangleRec(rectQuit, RED);
        DrawRectangleLinesEx(rectQuit, 3, RAYWHITE);
        const char *textQuit = "Quitter";
        int textQuitWidth = MeasureText(textQuit, buttonFontSize);
        DrawText(textQuit, centerW - textQuitWidth/2, startY + 160 + (buttonHeight - buttonFontSize) / 2, buttonFontSize, RAYWHITE);
    }
    else
    {
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
                
                if (x == selectedX && y == selectedY) DrawRectangleLines(drawX, drawY, tileSize, tileSize, GREEN); 
            }
        }

        // Dessin des Timers
        int whiteM = (int)board->timer.whiteTime / 60;
        int whiteS = (int)board->timer.whiteTime % 60;
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
            const char *menuText = "CLIQUEZ POUR REJOUER";
            
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
}