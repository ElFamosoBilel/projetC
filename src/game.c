#include "game.h"
#include <stdio.h> 
#include <stdlib.h> 

// Ajout des fonctions min et max pour l'AlphaBeta
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// --- 0. IMPORT EXTERNE ---
extern Texture2D gTileTextures[]; 
extern int gTileTextureCount; 

// --- 1. VARIABLES GLOBALES ---
int selectedX = -1; 
int selectedY = -1; 
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

#define MAX_POSSIBLE_MOVES 64 // Taille maximale = toutes les cases
static int possibleMoves[MAX_POSSIBLE_MOVES][2]; 
static int possibleMoveCount = 0; 

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
    // Pour Cavalier, Roi, Pion, on considère que le chemin n'est pas "bloquable" par cette fonction
    return true; 
}

// Vérifie si le coup est possible selon les règles de la pièce (et non si le roi est en échec)
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
        // Déplacement 1 case dans toutes les directions
        if (abs(dx) <= 1 && abs(dy) <= 1)
        {
            ruleMatch = true;
        }
        // ROQUE (Seulement pour la vérification physique ici, l'échec doit être vérifié plus tard)
        else if (dy == 0 && (dx == 2 || dx == -2))
        {
            int rookX = (dx == 2) ? startX + 3 : startX - 4;
            Tile *rookTile = (Tile*)&board->tiles[startY][rookX];

            if (rookTile->layerCount > 1)
            {
                int rookID = rookTile->layers[rookTile->layerCount - 1];
                bool correctRook =
                    (currentTurnColor == 0 && rookID == 12) ||
                    (currentTurnColor == 1 && rookID == 13);

                if (correctRook)
                {
                    int step = (dx > 0) ? 1 : -1;
                    bool pathClear = true;

                    for (int cx = startX + step; cx != rookX; cx += step)
                    {
                        if (board->tiles[startY][cx].layerCount > 1)
                        {
                            pathClear = false;
                            break;
                        }
                    }
                    if (pathClear) ruleMatch = true;
                }
            }
        }
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


// --- Fonctions de l'IA (Ajout de la logique de conflit/Make/Unmake) ---

// NOTE : Pour une IA d'échecs complète, cette fonction devrait inclure la vérification d'échec/mat
static int GenerateLegalMoves(const Board *board, Move movelist[])
{
    int count = 0;
    int current_player = currentTurn; // 0 (Blanc) ou 1 (Noir)

    for (int startY = 0; startY < BOARD_SIZE; startY++)
    {
        for (int startX = 0; startX < BOARD_SIZE; startX++)
        {
            Tile *startTile = &board->tiles[startY][startX];
            
            // Vérifier que c'est une pièce du joueur actuel
            if (startTile->layerCount <= 1) continue; 
            int pieceID = startTile->layers[startTile->layerCount - 1];
            if (GetPieceColor(pieceID) != current_player) continue;

            // Générer tous les mouvements possibles selon les règles
            for (int endY = 0; endY < BOARD_SIZE; endY++)
            {
                for (int endX = 0; endX < BOARD_SIZE; endX++)
                {
                    if (IsMoveValid(board, startX, startY, endX, endY))
                    {
                        // ATTENTION : Pour une IA complète, la vérification d'échec est nécessaire ici.
                        // Sans ça, l'IA peut se mettre en échec ou ne pas sortir d'échec.
                        
                        // Créer le coup de base
                        Move m = {startX, startY, endX, endY, pieceID, 0}; 

                        // Récupérer la pièce capturée
                        Tile *endTile = &board->tiles[endY][endX];
                        if (endTile->layerCount > 1)
                        {
                            m.capturedPieceID = endTile->layers[endTile->layerCount - 1];
                        }

                        // Temporairement, j'ajoute le coup même si la vérification d'échec est manquante.
                        if (count < MAX_MOVES)
                        {
                            movelist[count++] = m;
                        }
                    }
                }
            }
        }
    }
    return count;
}

static void MakeMove(Board *board, Move move)
{
    Tile *startTile = &board->tiles[move.startY][move.startX]; 
    Tile *endTile = &board->tiles[move.endY][move.endX]; 
    
    // Sauvegarder la pièce capturée (si elle n'est pas déjà dans 'move.capturedPieceID')
    if (move.capturedPieceID == 0 && endTile->layerCount > 1) 
    {
        move.capturedPieceID = TilePop(endTile); // Retire la pièce mangée et stocke son ID
    } 
    else if (move.capturedPieceID != 0)
    {
         TilePop(endTile); // Retire la pièce mangée (car elle est déjà stockée dans move)
    }

    int pieceID = TilePop(startTile); // Retire la pièce de départ
    TilePush (endTile, pieceID); // Place la pièce sur la nouvelle case

    // Logique du roque si c'est un coup de roque (dx=2 ou dx=-2)
    if ((pieceID == 10 || pieceID == 11) && abs(move.endX - move.startX) == 2)
    {
        int rookX_start = (move.endX > move.startX) ? move.startX + 3 : move.startX - 4;
        int rookX_end = (move.endX > move.startX) ? move.startX + 1 : move.startX - 1;

        Tile *rookStartTile = &board->tiles[move.startY][rookX_start];
        Tile *rookEndTile = &board->tiles[move.endY][rookX_end];
        
        int rookID = TilePop(rookStartTile);
        TilePush(rookEndTile, rookID);
    }
    
    // Le changement de tour est fait à l'annulation, pas à l'exécution de l'AlphaBeta
    // currentTurn = 1 - currentTurn; // Le changement de tour se fait dans le GameUpdate
}

static void UnmakeMove(Board *board, Move move)
{
    Tile *startTile = &board->tiles[move.startY][move.startX]; // Case départ originale
    Tile *endTile = &board->tiles[move.endY][move.endX]; // Case arrivée origniale
    
    // 1. Déplacer la pièce qui a bougé
    int pieceID = TilePop(endTile); // Supprime la pièce tout en la stockant
    TilePush(startTile, pieceID); // Replace la pièce à son ancienne position

    // 2. Replacer la pièce mangée
    if (move.capturedPieceID != 0)
    {
        TilePush(endTile, move.capturedPieceID); // Replace la pièce mangée 
    }
    
    // 3. Annuler le roque si c'était un coup de roque
    if ((pieceID == 10 || pieceID == 11) && abs(move.endX - move.startX) == 2)
    {
        int rookX_start = (move.endX > move.startX) ? move.startX + 3 : move.startX - 4;
        int rookX_end = (move.endX > move.startX) ? move.startX + 1 : move.startX - 1;

        Tile *rookStartTile = &board->tiles[move.startY][rookX_start];
        Tile *rookEndTile = &board->tiles[move.endY][rookX_end];
        
        int rookID = TilePop(rookEndTile);
        TilePush(rookStartTile, rookID);
    }
    // currentTurn = 1 - currentTurn; // Pas besoin de changer le tour ici
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
                int pieceID = t->layers[t->layerCount - 1]; 
                int pieceColor = GetPieceColor(pieceID); 

                int value = 0;

                // Identification de la pièce et assignation de la valeur
                if (pieceID == 6 || pieceID == 7) value = PAWN_VAL;       
                else if (pieceID == 2 || pieceID == 3) value = KNIGHT_VAL; 
                else if (pieceID == 4 || pieceID == 5) value = BISHOP_VAL;  
                else if (pieceID == 12 || pieceID == 13) value = ROOK_VAL;  
                else if (pieceID == 8 || pieceID == 9) value = QUEEN_VAL;   

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
                    if (pieceColor == 0) score += 5; 
                    else score -= 5;                 
                }
            }
        }
    }
    return score;
}

static int AlphaBeta(Board *b, int profondeur, int a, int beta, bool isMax, int playerTurn)
{
    if (profondeur == 0) return EvalutatePosition(b); 
    
    Move LocalMoveList[MAX_MOVES];
    int count = GenerateLegalMoves(b, LocalMoveList); 

    if (count == 0)
    {
        // Gérer échec et mat / pat si IsKingInCheck est implémenté
        return EvalutatePosition(b); 
    }

    if (isMax) // Cherche le meilleur coup pour Blanc (maximise)
    {
        int maxEval = -INFINITY;
        for (int i = 0; i < count; i++) 
        {
            Move m = LocalMoveList[i];
            MakeMove(b, m); 
            int eval = AlphaBeta(b, profondeur - 1, a, beta, false, 1 - playerTurn); 
            UnmakeMove(b, m); 
            maxEval = max(maxEval, eval);
            a = max(a, eval); 
            if (beta <= a) break; 
        }
        return maxEval;
    }
    else // Cherche le meilleur coup pour Noir (minimise)
    {
        int minEval = INFINITY;
        for (int i = 0; i < count; i++)
        {
            Move m = LocalMoveList[i];
            MakeMove(b , m);
            int eval = AlphaBeta(b, profondeur - 1, a, beta, true, 1 - playerTurn);
            UnmakeMove(b, m);
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= a) break;
        }
        return minEval;
    }
}

static Move FindBestMove(Board *board, int depth)
{
    Move legalMoves[MAX_MOVES];
    int count = GenerateLegalMoves(board, legalMoves);
    int playerTurn = currentTurn;
    int bestScore = (playerTurn == 0) ? -INFINITY : INFINITY;
    Move bestMove = legalMoves[0];

    if (count == 0) 
    {
        TraceLog(LOG_WARNING, "Aucun coup légal trouvé pour l'IA !");
        // Si aucun coup, retournez un coup nul ou le premier (pour ne pas crasher)
        if (count > 0) return legalMoves[0];
        return (Move){-1, -1, -1, -1, 0, 0}; 
    }

    for (int i = 0; i < count; i++)
    {
        Move move = legalMoves[i];
        MakeMove(board, move);
        // L'IA joue (Max) si elle est Blanc (0), et Min si elle est Noir (1)
        int eval = AlphaBeta(board, depth - 1, -INFINITY, INFINITY, (playerTurn == 0) ? false : true, 1 - playerTurn);
        UnmakeMove(board, move);
        
        // Mise à jour du meilleur coup trouvé
        if (playerTurn == 0) // Blanc (Maximise)
        {
            if (eval > bestScore)
            {
                bestScore = eval;
                bestMove = move;
            }
        }
        else // Noir (Minimise)
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

static void AIMakeMove(Board *board, float dt)
{
    if (board->mode == MODE_PLAYER_VS_IA && currentTurn == ID_IA)
    {
        if (board->IADelay > 0.0f)
        {
            board->IADelay -= dt;
            return;
        }

        int depth = 2; 
        Move bestMove = FindBestMove(board, depth);

        if (bestMove.startX != -1)
        {
            // Effectuer la capture et le déplacement
            Tile *startTile = &board->tiles[bestMove.startY][bestMove.startX];
            Tile *endTile = &board->tiles[bestMove.endY][bestMove.endX];
            
            // Capture
            if (endTile->layerCount > 1)
            {
                int targetID = endTile->layers[endTile->layerCount - 1];
                if (GetPieceColor(targetID) != currentTurn)
                {
                    TilePop(endTile);
                    // Vérification de victoire
                    if (targetID == 10 || targetID == 11) 
                    {
                        board->winner = currentTurn;
                        board->state = STATE_GAMEOVER;
                        TraceLog(LOG_INFO, "ROI CAPTURE PAR L'IA ! PARTIE TERMINEE");
                    }
                }
            }

            // Déplacement de la pièce
            int objID = TilePop(startTile);
            TilePush(endTile, objID);
            
            // Gestion du roque
            if ((objID == 10 || objID == 11) && abs(bestMove.endX - bestMove.startX) == 2)
            {
                int rookX_start = (bestMove.endX > bestMove.startX) ? bestMove.startX + 3 : bestMove.startX - 4;
                int rookX_end = (bestMove.endX > bestMove.startX) ? bestMove.startX + 1 : bestMove.startX - 1;

                Tile *rookStartTile = &board->tiles[bestMove.startY][rookX_start];
                Tile *rookEndTile = &board->tiles[bestMove.endY][rookX_end];
                
                int rookID = TilePop(rookStartTile);
                TilePush(rookEndTile, rookID);
            }

            currentTurn = 1 - currentTurn;
        }
        else
        {
             TraceLog(LOG_ERROR, "L'IA n'a pas trouvé de coup valide.");
        }
    }
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
    board->state = STATE_MAIN_MENU;
    board->mode = MODE_NONE;
    board->winner = -1;
    board->IADelay = 0.0f;
    board->turnState = TURN_PLAYER;
    currentTurn = 0; 
    selectedX = -1;
    selectedY = -1;
    possibleMoveCount = 0; 
}

void GameReset(Board *board)
{
    GameInit(board);
}

// --- LOGIQUE DE JEU SEULEMENT ---
static void GameLogicUpdate(Board *board, float dt)
{
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

    // Gestion Souris
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition(); 
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
                        possibleMoveCount = 0;
                        for (int py = 0; py < BOARD_ROWS; py++)
                        {
                            for (int px = 0; px < BOARD_COLS; px++)
                            {
                                // Seulement les mouvements valides sont affichés
                                if (IsMoveValid(board, selectedX, selectedY, px, py))
                                {
                                    if (possibleMoveCount < MAX_POSSIBLE_MOVES)
                                    {
                                        possibleMoves[possibleMoveCount][0] = px; 
                                        possibleMoves[possibleMoveCount][1] = py; 
                                        possibleMoveCount++; 
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
                    possibleMoveCount = 0; 
                    TraceLog(LOG_INFO, "Annulation");
                    return; 
                }

                // B. Vérifier si c'est un coup possible (utilisé pour l'affichage, il est plus simple de le réutiliser)
                bool moveAllowed = false;
                for (int i = 0; i < possibleMoveCount; i++)
                {
                    if (possibleMoves[i][0] == endX && possibleMoves[i][1] == endY)
                    {
                        moveAllowed = true;
                        break;
                    }
                }

                // C. Si le coup est possible et valide physiquement
                if (moveAllowed)
                {
                    // Double-vérification de la validité
                    if (IsMoveValid(board, startX, startY, endX, endY))
                    {
                        Tile *oldTile = &board->tiles[selectedY][selectedX]; 
                        int movingPieceID = oldTile->layers[oldTile->layerCount - 1]; 

                        // 1. Capture/Victoire
                        if (clickedTile->layerCount > 1)
                        {
                            int targetID = clickedTile->layers[clickedTile->layerCount - 1];
                            // Clic sur un ami devrait être géré par l'annulation/resélection plus bas.
                            // Si c'est un ennemi (on a vérifié que le coup est légal), on capture.
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

                        // 2. Déplacement
                        int objID = TilePop(oldTile); 
                        TilePush(clickedTile, objID); 
                        
                        // 3. Gestion du roque (déplacement de la tour)
                        if ((objID == 10 || objID == 11) && abs(dx) == 2)
                        {
                            int rookX_start = (dx == 2) ? startX + 3 : startX - 4;
                            int rookX_end = (dx == 2) ? startX + 1 : startX - 1;

                            Tile *rookStartTile = &board->tiles[startY][rookX_start];
                            Tile *rookEndTile = &board->tiles[startY][rookX_end];
                            
                            int rookID = TilePop(rookStartTile);
                            TilePush(rookEndTile, rookID);
                        }

                        // 4. Fin du coup
                        selectedX = -1; 
                        selectedY = -1;
                        possibleMoveCount = 0; 

                        if (board->state != STATE_GAMEOVER) 
                        {
                            currentTurn = 1 - currentTurn; 
                            if (currentTurn == ID_IA && board->mode == MODE_PLAYER_VS_IA)
                            {
                                board->IADelay = 1.0f;
                            }
                            TraceLog(LOG_INFO, "Coup valide. Tour : %s", (currentTurn == 0) ? "Blanc" : "Noir");
                        }
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Coup invalide. (Problème de validation interne)");
                    }
                }
                else
                {
                    // Si le coup n'est pas autorisé, vérifions si l'utilisateur essaie de sélectionner une autre de ses pièces
                    if (clickedTile->layerCount > 1)
                    {
                        int targetColor = GetPieceColor(clickedTile->layers[clickedTile->layerCount - 1]);
                        if (targetColor == currentTurn)
                        {
                            selectedX = -1; // Force la réinitialisation pour relancer la sélection
                            possibleMoveCount = 0;
                            GameLogicUpdate(board, dt); // Re-appelle pour sélectionner la nouvelle pièce
                            return; 
                        }
                    }
                    // Si c'est un clic sur une case non valide (ennemi, vide, hors des règles)
                    selectedX = -1;
                    selectedY = -1;
                    possibleMoveCount = 0;
                    TraceLog(LOG_WARNING, "Coup non autorisé ou réinitialisation.");
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
    if (board->state == STATE_MAIN_MENU)
    {
        // Gestion du clic pour choisir le mode de jeu
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 m = GetMousePosition();
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();
            
            // Zones de clic (approximatives autour des textes du menu)
            int buttonW = 300;
            int buttonH = 50;
            int centerW = screenW / 2;
            int centerH = screenH / 2;
            
            // Bouton JcJ
            if (m.x > centerW - buttonW/2 && m.x < centerW + buttonW/2 && 
                m.y > centerH - 60 && m.y < centerH - 60 + buttonH) 
            {
                board->mode = MODE_PLAYER_VS_PLAYER;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "Mode JcJ sélectionné.");
            }
            // Bouton JcIA
            else if (m.x > centerW - buttonW/2 && m.x < centerW + buttonW/2 && 
                     m.y > centerH + 10 && m.y < centerH + 10 + buttonH) 
            {
                board->mode = MODE_PLAYER_VS_IA;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "Mode Jc IA sélectionné.");
            }
        }
    }
    else if (board->state == STATE_PLAYING)
    {
        if (board->mode == MODE_PLAYER_VS_IA && currentTurn == ID_IA)
        {
            if (board->turnState == TURN_PLAYER)
            {
                board->IADelay = 1.0f;
                board->turnState = TURN_IA_WAITING;
            }
            else if (board->turnState == TURN_IA_WAITING)
            {
                board->IADelay -= dt;
                if (board->IADelay <= 0.0f)
                {
                    board->turnState = TURN_IA_MOVING;
                }
                return;
            }
            else if (board->turnState == TURN_IA_MOVING)
            {
                AIMakeMove(board, dt); // Joue son coup
                board->turnState = TURN_PLAYER;
            }  
        }
        else
        {
            if (board->mode == MODE_PLAYER_VS_IA)
            {
                board->turnState = TURN_PLAYER;
            }
            
            if (board->mode == MODE_PLAYER_VS_PLAYER || (board->mode == MODE_PLAYER_VS_IA && currentTurn != ID_IA))
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
        }
    }
    else if (board->state == STATE_GAMEOVER)
    {
        // Gestion du clic dans le menu de fin de partie
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_R))
        {
            Vector2 m = GetMousePosition();
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();
            
            // Détection du clic sur la zone "REJOUER" (approximatif comme dans GameDraw)
            int mWidth = MeasureText("CLIQUEZ / APPUYEZ SUR R POUR REJOUER", 30);
            int centerW = screenW / 2;
            int centerH = screenH / 2;

            if (IsKeyPressed(KEY_R) || (m.x > centerW - mWidth/2 - 10 && m.x < centerW + mWidth/2 + 10 && 
                m.y > centerH - 5 && m.y < centerH + 35)) 
            {
                GameInit(board); 
                TraceLog(LOG_INFO, "Nouvelle partie lancée.");
            }
        }
    }
}

// DESSIN DU JEU 
void GameDraw(const Board *board)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    const int FONT_SIZE = 30; 
    const int TEXT_PADDING = 20; 

    // ÉCRAN PRINCIPAL (Plateau, Timers)
    if (board->state == STATE_PLAYING || board->state == STATE_GAMEOVER)
    {
        // Calcul des dimensions
        int tileSizeW = screenW / BOARD_COLS;
        int tileSizeH = screenH / BOARD_ROWS;
        int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH;
        int boardW = tileSize * BOARD_COLS;
        int boardH = tileSize * BOARD_ROWS;
        int offsetX = (screenW - boardW) / 2;
        int offsetY = (screenH - boardH) / 2;
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
                    DrawTexturePro(gTileTextures[idx],(Rectangle){0, 0, (float)gTileTextures[idx].width, (float)gTileTextures[idx].height},(Rectangle){(float)drawX, (float)drawY, (float)tileSize, (float)tileSize},(Vector2){0,0},0,WHITE); 
                }
                
                // Bordures
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
            
            Color indicatorColor = Fade(DARKGRAY, 0.3f);
            
            const Tile *t = &board->tiles[y][x];
            if (t->layerCount > 1 && GetPieceColor(t->layers[t->layerCount - 1]) != currentTurn) {
                indicatorColor = Fade(RED, 0.6f);
                DrawRectangleLinesEx((Rectangle){(float)drawX, (float)drawY, (float)tileSize, (float)tileSize}, 5, indicatorColor);
            } else {
                DrawCircle(drawX + tileSize / 2, drawY + tileSize / 2, tileSize / 8, indicatorColor);
            }
        }

        // Dessin de la sélection
        if (selectedX != -1) 
        {
            int drawX = offsetX + selectedX * tileSize;
            int drawY = offsetY + selectedY * tileSize;
            DrawRectangleLinesEx((Rectangle){(float)drawX, (float)drawY, (float)tileSize, (float)tileSize}, 4, GREEN); 
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
    }
    
    // ÉCRAN FIN DE PARTIE 
    if (board->state == STATE_GAMEOVER)
    {
        int centerW = screenW / 2;
        int centerH = screenH / 2;
        
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.8f)); 
        
        const char *winnerName = (board->winner == 0) ? "BLANCS" : "NOIRS";
        const char *winnerText = TextFormat("VICTOIRE DES %s !", winnerName);
        const char *menuText = "CLIQUEZ / APPUYEZ SUR R POUR REJOUER";
        
        int wWidth = MeasureText(winnerText, 60);
        int mWidth = MeasureText(menuText, 30);
        
        DrawText(winnerText, centerW - wWidth/2, centerH - 100, 60, GREEN);
        
        DrawText(menuText, centerW - mWidth/2, centerH, 30, YELLOW);
        DrawRectangleLines(centerW - mWidth/2 - 10, centerH - 5, mWidth + 20, 40, YELLOW); 
        
        const char *quitText = "Appuyer sur ECHAP pour quitter";
        int qWidth = MeasureText(quitText, 20);
        DrawText(quitText, centerW - qWidth/2, centerH + 100, 20, RAYWHITE);
    }
    
    // ÉCRAN MENU PRINCIPAL
    else if (board->state == STATE_MAIN_MENU)
    {
        int centerW = screenW / 2;
        int centerH = screenH / 2;
        
        DrawRectangle(0, 0, screenW, screenH, BLACK);
        
        const char *titleText = "JEU D'ÉCHECS";
        const char *pvpText = "1 v 1";
        const char *pviaText = "vs IA";
        
        int titleWidth = MeasureText(titleText, 80);
        int pvpWidth = MeasureText(pvpText, 30);
        int pviaWidth = MeasureText(pviaText, 30);
        
        // Titre
        DrawText(titleText, centerW - titleWidth/2, centerH - 200, 80, RAYWHITE);

        // Bouton JcJ
        DrawRectangleLines(centerW - 150, centerH - 60, 300, 50, GREEN);
        DrawText(pvpText, centerW - pvpWidth/2, centerH - 50, 30, RAYWHITE);
        
        // Bouton JcIA
        DrawRectangleLines(centerW - 150, centerH + 10, 300, 50, YELLOW);
        DrawText(pviaText, centerW - pviaWidth/2, centerH + 20, 30, RAYWHITE);
    }
}