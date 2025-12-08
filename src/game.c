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

// IA

static int GenerateLegalMoves(const Board *board, Move movelist[])
{
    int count = 0;
    int current_player = currentTurn; // 0 (Blanc) ou 1 (Noir)

    // Pour chaque case du plateau
    for (int startY = 0; startY < BOARD_SIZE; startY++)
    {
        for (int startX = 0; startX < BOARD_SIZE; startX++)
        {
            Tile *startTile = &board->tiles[startY][startX];
            
            // Si la case de départ n'a pas de pièce ou n'est pas la couleur du joueur actuel, ignorer
            if (startTile->layerCount <= 1) continue; 
            int pieceID = startTile->layers[startTile->layerCount - 1];
            if (GetPieceColor(pieceID) != current_player) continue;

            // Logique de mouvement par type de pièce
            if (pieceID == 12 || pieceID == 13 || pieceID == 4 || pieceID == 5 || pieceID == 8 || pieceID == 9)
            {
                // Définir ici les directions possibles pour la pièce
                int directions[8][2] = {
                    {0, 1}, {0, -1}, {1, 0}, {-1, 0}, // Horizontal/Vertical (Tour, Reine)
                    {1, 1}, {1, -1}, {-1, 1}, {-1, -1} // Diagonal (Fou, Reine)
                };
                int max_dirs = (pieceID == 12 || pieceID == 13) ? 4 : // Tour
                               (pieceID == 4 || pieceID == 5) ? 4 : // Fou
                               8; // Reine

                for (int d = 0; d < max_dirs; d++)
                {
                    for (int step = 1; step < BOARD_SIZE; step++) // Parcours des pas
                    {
                        int endX = startX + directions[d][0] * step;
                        int endY = startY + directions[d][1] * step;

                        // Vérifier si la destination est dans le plateau
                        if (endX < 0 || endX >= BOARD_SIZE || endY < 0 || endY >= BOARD_SIZE) break;
                        Tile *endTile = &board->tiles[endY][endX];

                        // Si la destination est bloquée par un ami, s'arrêter dans cette direction
                        if (endTile->layerCount > 1 && GetPieceColor(endTile->layers[endTile->layerCount - 1]) == current_player) break;

                        // Créer le coup (logique MakeMove/UnmakeMove pour la vérification d'échec est nécessaire ici)
                        
                        // NOTE : La vérification IsPathClear est implicite par le parcours 'step' et le 'break' ci-dessus.
                        // On doit vérifier si le coup met le roi en échec avant d'ajouter le coup à moveList.
                        // SIMULATION DU COUP... (MakeMove)
                        // VÉRIFICATION DE L'ÉCHEC... (IsKingInCheck)
                        // ANNULATION DU COUP... (UnmakeMove)

                        // Si le coup est légal, l'ajouter.
                        moveList[count++] = (Move){startX, startY, endX, endY, 0}; 

                        // Si la destination est une pièce ennemie, ajouter le coup, puis s'arrêter dans cette direction
                        if (endTile->layerCount > 1 && GetPieceColor(endTile->layers[endTile->layerCount - 1]) != current_player) break;
                    }
                }
            }
            // Pour les pièces de COURT PORTEE (Cavalier, Roi, Pion)
            else if (pieceID == 2 || pieceID == 3 || pieceID == 10 || pieceID == 11 || pieceID == 6 || pieceID == 7)
            {
                // Ajoutez ici la logique spécifique et les boucles pour chaque type de pièce
                // C'est ici que l'énorme logique des pions (initial, capture, en passant, promotion) va.
                // C'est ici que la vérification abs(dx) <= 1 && abs(dy) <= 1 pour le Roi va.
                // C'est ici que la vérification L (1x2 ou 2x1) pour le Cavalier va.

                // Pour chaque coup généré...
                // SIMULATION DU COUP... (MakeMove)
                // VÉRIFICATION DE L'ÉCHEC... (IsKingInCheck)
                // ANNULATION DU COUP... (UnmakeMove)
                // Si le coup est légal, l'ajouter à moveList[count++] = ...
            }
        }
    }

    return count;

}

static void MakeMove(Board *board, Move move)
{
    Tile *startTile = &board->tiles[move.startY][move.startX]; // Case départ
    Tile *endTile = &board->tiles[move.endY][move.endX]; // Case arrivée
    move.capturedPieceID = 0; // Quel pièce a été capturé (0 par défaut)
    if (endTile->layerCount > 1) // Vérifie si la case d'arrivée n'est pas vide
    {
        move.capturedPieceID = TilePop(endTile); // Retire la pièce manger et stock son ID
    }
    int pieceID = TilePop(startTile); // Retire la pièce de départ
    TilePush (endTile, pieceID); // Place la pièce sur la nouvelle case
    currentTurn = 1 - currentTurn;
}

static void UnmakeMove(Board *board, Move move)
{
    Tile *startTile = &board->tiles[move.startY][move.startX]; // Case départ originale
    Tile *endTile = &board->tiles[move.endY][move.endX]; // Case arrivée origniale
    int pieceID = TilePop(endTile); // Supprime la pièce tout en la stockant
    TilePush(startTile, pieceID); // Replace la pièce à son ancienne position
    if (move.capturedPieceID != 0)
    {
        TilePush(endTile, move.capturedPieceID); // Replace la pièce mangée 
    }
}

static int EvalutatePosition(const Board *board)
{
    int PAWN_VAL = 100;
    int KNIGHT_VAL = 320;
    int BISHOP_VAL = 330;
    int ROOK_VAL = 500;
    int QUEEN_VAL = 900;
    int score = 0;

    // Calcul de la valeur matérielle
    for (int y = 0; y < BOARD_ROWS; y++)
    {
        for (int x = 0; x < BOARD_COLS; x++)
        {
            const Tile *t = &board->tiles[y][x];
            
            // Si la case contient une pièce
            if (t->layerCount > 1)
            {
                int pieceID = t->layers[t->layerCount - 1]; // Pièce sur le dessus
                int pieceColor = GetPieceColor(pieceID); // 0 = Blanc, 1 = Noir

                int value = 0;

                // Identification de la pièce et assignation de la valeur
                if (pieceID == 6 || pieceID == 7) value = PAWN_VAL;       // Pion (6:Blanc, 7:Noir)
                else if (pieceID == 2 || pieceID == 3) value = KNIGHT_VAL; // Cavalier (2:B, 3:N)
                else if (pieceID == 4 || pieceID == 5) value = BISHOP_VAL;  // Fou (4:B, 5:N)
                else if (pieceID == 12 || pieceID == 13) value = ROOK_VAL;  // Tour (12:B, 13:N)
                else if (pieceID == 8 || pieceID == 9) value = QUEEN_VAL;   // Reine (8:B, 9:N)

                // Ajout ou soustraction au score total
                if (pieceColor == 0) // Blanc (maximise)
                {
                    score += value;
                }
                else // Noir (minimise)
                {
                    score -= value;
                }

                // Facteurs positionnels simples
                if ((pieceID == 6 || pieceID == 7) && (x >= 3 && x <= 4)) // Pions centraux
                {
                    if (pieceColor == 0) score += 5; // Bonus pour les pions Blancs au centre
                    else score -= 5;                 // Malus pour les pions Noirs au centre
                }
            }
        }
    }
    return score;
}

static int AlphaBeta(Board *b, int profondeur, int a, int beta, bool isMax)
{
    if (profondeur == 0) return EvalutatePosition(b); // Si on attends la profondeur souhaité o, retourne le score
    Move LocalMoveList[MAX_MOVES];
    int count = GenerateLegalMoves(b, LocalMoveList); // Génère tous les coups possibles de cette position
    if (isMax) // Cherche le meilleur coup
    {
        for (int i = 0; i < count; i++) // Pour chaque coup légal
        {
            Move m = moveList[i];
            MakeMove(b, m); // Execute le coup sur un plateau virtuel
            int eval = AlphaBeta(b, profondeur - 1, a, beta, false); // Appel récursif pour le tour de l'adversaire
            UnmakeMove(b, m); // Annule le coup
            a = max(a, eval); // Met a jour la meilleur valeur trouvé pour l'IA
            if (beta <= a) break; // Si la meilleur option de l'IA est pire que ce que l'humain a trouvé on arrête la recherche
        }
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            Move m = moveList[i];
            MakeMove(b , m);
            int eval = AlphaBeta(b, profondeur - 1, a, beta, true);
            UnmakeMove(b, m);
            beta = min(beta, eval);
            if (beta <= a) break;
        }
    }
}

static Move FindBestMove(Board *board, int depth)
{
    Move legalMoves[MAX_MOVES];
    int count = GenerateLegalMoves(board, legalMoves);
    int bestScore = (currentTurn == 0) ? -INFINITY : INFINITY;
    Move bestMove = legalMoves[0];

    if (count == 0) 
    {
        TraceLog(LOG_WARNING, "Aucun coup légal trouvé !");
        return bestMove;
    }

    for (int i = 0; i < count; i++)
    {
        Move move = legalMoves[i];
        MakeMove(board, move);
        int eval = AlphaBeta(board, depth - 1, -INFINITY, INFINITY, (currentTurn == 0) ? false : true);
        UnmakeMove(board, move);
        if (currentTurn == 0)
        {
            if (eval > bestScore)
            {
                bestScore = eval;
                bestMove = move;
            }
        }
        else 
        {
            if (eval < bestScore)
            {
                bestScore = eval;
                bestMove = move;
            }
        }
    }
    return bestMove;
}

static void AIMakeMove(Board *board)
{
    if (board->mode == MODE_PLAYER_VS_IA && currentTurn == ID_IA)
    {
        int depth = 4; // Permet d'ajuster la difficulté de l'IA
        Move bestMove = FindBestMove(board, depth);
        Tile *startTile = &board->tiles[bestMove.startY][bestMove.startX];
        Tile *endTile = &board->tiles[bestMove.endY][bestMove.endX];
        if (endTile->layerCount > 1)
        {
            TilePop(endTile);
        }
        int objID = TilePop(startTile);
        TilePush(endTile,objID);
        currentTurn = 1 - currentTurn;
    }
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
        if (board->mode == MODE_PLAYER_VS_IA && currentTurn == ID_IA)
        {
            AIMakeMove(board);
        }
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