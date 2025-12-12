#include "game.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h> // Ajout pour bool et les fonctions

// Ajout des fonctions min et max pour l'AlphaBeta
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// IMPORTATIONS EXTERNES
extern Texture2D gTileTextures[]; 
extern int gTileTextureCount; 

// VARIABLES GLOBALES
int selectedX = -1; 
int selectedY = -1; 
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

// Gestion de la Promotion (Pion -> Reine/Tour/Etc)
int promotionPending = 0; // 1 si le jeu est en pause pour choisir une pièce
int promotionX = -1;
int promotionY = -1;
int promotionColor = -1;

// Gestion des coups possibles (pour l'affichage des ronds/cadres rouges)
static int possibleMoves[MAX_MOVES][2]; 
static int possibleMoveCount = 0; 

// FONCTIONS UTILITAIRES

// Vide complètement une case (enlève toutes les pièces)
static void TileClear(Tile *t) 
{
    t->layerCount = 0;
    for(int i = 0; i < MAX_LAYERS; i++) 
    {
        t->layers[i] = 0;
    }
}

// Ajoute une texture (ID de pièce) sur une case
static void TilePush(Tile *t, int textureIndex) 
{
    if (t->layerCount < MAX_LAYERS) 
    {
        t->layers[t->layerCount] = textureIndex;
        t->layerCount++;
    }
}

// Retire la dernière texture posée (la pièce du dessus) et renvoie son ID
static int TilePop(Tile *t) 
{
    if (t->layerCount <= 1) return 0; // Ne retire pas le sol
    
    int objectIndex = t->layers[t->layerCount - 1];
    t->layerCount--;
    
    return objectIndex;
}

// Renvoie la couleur d'une pièce : 0 = Blanc, 1 = Noir, -1 = Pas une pièce
static int GetPieceColor(int textureID)
{
    if (textureID < 2) return -1; // Les ID 0 et 1 sont les sols
    
    // Astuce : Les ID pairs sont blancs, les impairs sont noirs
    if (textureID % 2 == 0)
    {
        return 0; // Blanc
    }
    else 
    {
        return 1; // Noir
    }
}

// LOGIQUE DE DÉPLACEMENT

// Vérifie si le chemin est libre entre A et B (pour Tour, Fou, Reine)
static bool IsPathClear(const Board *board, int startX, int startY, int endX, int endY)
{
    // Cas 1 : Déplacement Horizontal
    if (startY == endY) 
    {
        int step = (endX > startX) ? 1 : -1;

        // On parcourt les cases ENTRE le départ et l'arrivée
        for (int x = startX + step; x != endX; x += step)
        {
            if (board->tiles[startY][x].layerCount > 1) 
            {
                return false; // Obstacle trouvé
            }
        }
    }
    // Cas 2 : Déplacement Vertical
    else if (startX == endX)
    {
        int step = (endY > startY) ? 1 : -1;

        for (int y = startY + step; y != endY; y += step)
        {
            if (board->tiles[y][startX].layerCount > 1) 
            {
                return false; // Obstacle trouvé
            }
        }
    }
    // Cas 3 : Déplacement Diagonal
    else if (abs(endX - startX) == abs(endY - startY)) 
    {
        int stepX = (endX > startX) ? 1 : -1;
        int stepY = (endY > startY) ? 1 : -1;

        int x = startX + stepX; 
        int y = startY + stepY; 

        // On avance en diagonale jusqu'à la case juste avant l'arrivée
        while (x != endX) 
        {
            if (board->tiles[y][x].layerCount > 1) 
            {
                return false; // Obstacle trouvé
            }
            x += stepX; 
            y += stepY; 
        }
    }
    return true; // Chemin libre
}

// Vérifie si une pièce a le droit de bouger de A vers B (Règles des échecs de base)
static bool IsMoveValid(const Board *board, int startX, int startY, int endX, int endY)
{
    // Règle 0 : On ne peut pas faire du surplace
    if (startX == endX && startY == endY) return false;
    
    const Tile *oldTile = &board->tiles[startY][startX];
    const Tile *targetTile = &board->tiles[endY][endX];
    
    // Sécurité : Si la case de départ est vide
    if (oldTile->layerCount <= 1) return false;

    int pieceID = oldTile->layers[oldTile->layerCount - 1]; 
    int currentTurnColor = GetPieceColor(pieceID); 
    
    int dx = endX - startX; 
    int dy = endY - startY; 
    bool ruleMatch = false;

    // ANALYSE SELON LA PIÈCE

    // TOUR (ID 12 Blanc, 13 Noir)
    if (pieceID == 12 || pieceID == 13) 
    {
        // Doit bouger en ligne droite (soit dx est 0, soit dy est 0)
        if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0)) 
        {
            if (IsPathClear(board, startX, startY, endX, endY)) 
            {
                ruleMatch = true;
            }
        }
    }
    // FOU (ID 4 Blanc, 5 Noir)
    else if (pieceID == 4 || pieceID == 5) 
    {
        // Doit bouger en diagonale parfaite (dx égal à dy en valeur absolue)
        if (abs(dx) == abs(dy) && dx != 0) 
        {
            if (IsPathClear(board, startX, startY, endX, endY)) 
            {
                ruleMatch = true;
            }
        }
    }
    // REINE (ID 8 Blanc, 9 Noir)
    else if (pieceID == 8 || pieceID == 9) 
    {
        // Combine Tour et Fou
        if ((dx != 0 && dy == 0) || (dx == 0 && dy != 0) || (abs(dx) == abs(dy))) 
        {
            if (IsPathClear(board, startX, startY, endX, endY)) 
            {
                ruleMatch = true;
            }
        }
    }
    // ROI (ID 10 Blanc, 11 Noir)
    else if (pieceID == 10 || pieceID == 11) 
    {
        // Déplacement 1 case dans toutes les directions
        if (abs(dx) <= 1 && abs(dy) <= 1) 
        {
            ruleMatch = true;
        }
        // ROQUE
        else if (dy == 0 && (dx == 2 || dx == -2))
        {
            // chemin clair et tour présente.
            int rookX = (dx == 2) ? startX + 3 : startX - 4; // Colonne 7 ou 0  
            if (rookX >= 0 && rookX < BOARD_COLS) 
            {
                const Tile *rookTile = &board->tiles[startY][rookX];

                if (rookTile->layerCount > 1)
                {
                    int rookID = rookTile->layers[rookTile->layerCount - 1];
                    // Vérifie si la pièce est la bonne Tour (blanche/noire)
                    bool correctRook =
                        (currentTurnColor == 0 && rookID == 12) ||
                        (currentTurnColor == 1 && rookID == 13);

                    if (correctRook)
                    {
                        // Vérifie si le chemin est clair entre Roi et Tour
                        int step = (dx > 0) ? 1 : -1;
                        bool pathClear = true;
                        for (int x = startX + step; x != rookX; x += step)
                        {
                            if (board->tiles[startY][x].layerCount > 1)
                            {
                                pathClear = false;
                                break;
                            }
                        }
                        
                        // Si le chemin est clair, on valide le match physique
                        if (pathClear)
                        {
                            ruleMatch = true;
                        }
                    }
                }
            }
        }
    }

    // CAVALIER (ID 2 Blanc, 3 Noir)
    else if (pieceID == 2 || pieceID == 3) 
    {
        // Mouvement en "L" (2 cases d'un côté, 1 case de l'autre)
        if ((abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1)) 
        {
            ruleMatch = true;
        }
    }
    // PION (ID 6 Blanc, 7 Noir)
    else if (pieceID == 6 || pieceID == 7) 
    {
        int direction;
        int initialRow;

        if (currentTurnColor == 0) { // Blanc
            direction = -1; // Monte
            initialRow = 6;
        } else { // Noir
            direction = 1; // Descend
            initialRow = 1;
        }
        
        // Capture en diagonale
        if (abs(dx) == 1 && dy == direction) 
        {
            // Il faut qu'il y ait une pièce ennemie sur la cible
            if (targetTile->layerCount > 1) 
            { 
                int targetColor = GetPieceColor(targetTile->layers[targetTile->layerCount - 1]);
                if (targetColor != -1 && targetColor != currentTurnColor) 
                {
                    ruleMatch = true; 
                }
            }
        }
        // Avance simple (1 case)
        else if (dx == 0 && dy == direction) 
        {
            // La case cible doit être vide
            if (targetTile->layerCount == 1) 
            {
                ruleMatch = true;
            }
        }
        // Double avance (Premier tour)
        else if (dx == 0 && dy == 2 * direction && startY == initialRow) 
        {
            // La case cible ET la case intermédiaire doivent être vides
            const Tile *midTile = &board->tiles[startY + direction][startX];
            if (targetTile->layerCount == 1 && midTile->layerCount == 1) 
            {
                ruleMatch = true;
            }
        }
    }

    // Si la règle physique n'est pas respectée, c'est invalide
    if (!ruleMatch) return false;

    // VÉRIFICATION COLLISION ALLIÉE
    // On ne peut pas manger ses propres pièces
    if (targetTile->layerCount > 1) 
    {
        int targetID = targetTile->layers[targetTile->layerCount - 1];
        int targetColor = GetPieceColor(targetID);
        
        if (targetColor != -1 && targetColor == currentTurnColor) 
        {
            // Exception : Pour le roque, l'arrivée est vide.
            if (!((pieceID == 10 || pieceID == 11) && abs(dx) == 2))
            {
                return false; // Bloqué par un ami
            }
        }
    }

    return true; 
}

// LOGIQUE DE SÉCURITÉ (ECHEC / MAT)

// Détecte si le Roi d'une couleur donnée est menacé ACTUELLEMENT
static bool IsKingInCheck(const Board *board, int kingColor)
{
    int kingX = -1;
    int kingY = -1;
    int targetKingID = (kingColor == 0) ? 10 : 11; // 10=Blanc, 11=Noir

    // On cherche où est le Roi
    for (int y = 0; y < BOARD_ROWS; y++) 
    {
        for (int x = 0; x < BOARD_COLS; x++) 
        {
            const Tile *t = &board->tiles[y][x];
            if (t->layerCount > 1) 
            {
                if (t->layers[t->layerCount - 1] == targetKingID) 
                {
                    kingX = x; 
                    kingY = y;
                }
            }
        }
    }
    
    if (kingX == -1 || kingY == -1) return false;

    // On regarde si un ennemi peut attaquer cette case (kingX, kingY)
    for (int y = 0; y < BOARD_ROWS; y++) 
    {
        for (int x = 0; x < BOARD_COLS; x++) 
        {
            const Tile *t = &board->tiles[y][x];
            
            if (t->layerCount <= 1) continue;

            int attackerID = t->layers[t->layerCount - 1];
            int attackerColor = GetPieceColor(attackerID);

            if (attackerColor == -1 || attackerColor == kingColor) continue;
            
            // On fait une copie du board pour l'appel à IsMoveValid
            Board tempBoard = *board; 
            int originalTurn = currentTurn;
            
            // On simule que c'est le tour de l'attaquant pour IsMoveValid
            currentTurn = attackerColor;
            
            // Si cet ennemi peut légalement aller sur la case du Roi
            if (IsMoveValid(&tempBoard, x, y, kingX, kingY)) 
            {
                currentTurn = originalTurn; // Restaure le tour
                return true; // Le Roi est en échec !
            }
            currentTurn = originalTurn; // Restaure le tour (si l'appel a échoué)
        }
    }
    return false;
}

// LOGIQUE DE L'IA

// Effectue le coup (déplace la pièce, gère la capture, le roque)
static void MakeMove(Board *board, Move move)
{
    Tile *startTile = &board->tiles[move.startY][move.startX]; 
    Tile *endTile = &board->tiles[move.endY][move.endX]; 
    
    // Sauvegarder la pièce capturée (si elle n'est pas déjà dans 'move.capturedPieceID')
    // Pour l'IA, on suppose que 'capturedPieceID' est déjà pré-rempli par GenerateLegalMoves.
    if (move.capturedPieceID == 0 && endTile->layerCount > 1) 
    {
        move.capturedPieceID = TilePop(endTile); // Retire la pièce mangée et stocke son ID
    } 
    else if (move.capturedPieceID != 0)
    {
         TilePop(endTile); // Retire la pièce mangée (elle est déjà stockée)
    }
    
    // Le sol reste (sauf s'il y a un bug). TilePop gère cela.
    int pieceID = TilePop(startTile); // Retire la pièce de départ
    TilePush (endTile, pieceID); // Place la pièce sur la nouvelle case

    // Logique du roque si c'est un coup de roque
    if ((pieceID == 10 || pieceID == 11) && abs(move.endX - move.startX) == 2)
    {
        int rookX_start = (move.endX > move.startX) ? move.startX + 3 : move.startX - 4;
        int rookX_end = (move.endX > move.startX) ? move.startX + 1 : move.startX - 1;

        Tile *rookStartTile = &board->tiles[move.startY][rookX_start];
        Tile *rookEndTile = &board->tiles[move.endY][rookX_end];
        
        int rookID = TilePop(rookStartTile);
        TilePush(rookEndTile, rookID);
    }
}

// Annule le coup (replace la pièce, replace la pièce capturée, annule le roque)
static void UnmakeMove(Board *board, Move move)
{
    Tile *startTile = &board->tiles[move.startY][move.startX]; // Case départ originale
    Tile *endTile = &board->tiles[move.endY][move.endX]; // Case arrivée origniale
    
    // Déplacer la pièce qui a bougé
    int pieceID = TilePop(endTile); // Supprime la pièce tout en la stockant
    TilePush(startTile, pieceID); // Replace la pièce à son ancienne position

    // Replacer la pièce mangée
    if (move.capturedPieceID != 0)
    {
        TilePush(endTile, move.capturedPieceID); // Replace la pièce mangée 
    }
    
    // Annuler le roque si c'était un coup de roque
    if ((pieceID == 10 || pieceID == 11) && abs(move.endX - move.startX) == 2)
    {
        int rookX_start = (move.endX > move.startX) ? move.startX + 3 : move.startX - 4;
        int rookX_end = (move.endX > move.startX) ? move.startX + 1 : move.startX - 1;

        Tile *rookStartTile = &board->tiles[move.startY][rookX_start];
        Tile *rookEndTile = &board->tiles[move.endY][rookX_end];
        
        int rookID = TilePop(rookEndTile);
        TilePush(rookStartTile, rookID);
    }
}

// Génère tous les coups LÉGAUX (qui ne mettent pas le roi en échec) pour le joueur donné
static int GenerateLegalMoves(Board *board, Move movelist[], int playerColor)
{
    int count = 0;
    int originalTurn = currentTurn; // Sauvegarde du tour

    for (int startY = 0; startY < BOARD_ROWS; startY++)
    {
        for (int startX = 0; startX < BOARD_COLS; startX++)
        {
            Tile *startTile = &board->tiles[startY][startX];
            
            // Vérifier que c'est une pièce du joueur actuel
            if (startTile->layerCount <= 1) continue; 
            int pieceID = startTile->layers[startTile->layerCount - 1];
            if (GetPieceColor(pieceID) != playerColor) continue;
            
            // NOTE : On doit temporairement mettre le tour pour que IsMoveValid sache quelle couleur vérifier
            currentTurn = playerColor;

            // Générer tous les mouvements possibles selon les règles physiques
            for (int endY = 0; endY < BOARD_ROWS; endY++)
            {
                for (int endX = 0; endX < BOARD_COLS; endX++)
                {
                    if (IsMoveValid(board, startX, startY, endX, endY))
                    {
                        // Créer le coup de base
                        Move m = {startX, startY, endX, endY, pieceID, 0}; 

                        // Récupérer la pièce capturée (simulée)
                        Tile *endTile = &board->tiles[endY][endX];
                        if (endTile->layerCount > 1)
                        {
                            m.capturedPieceID = endTile->layers[endTile->layerCount - 1];
                        }
                        
                        // On simule le coup
                        MakeMove(board, m); 
                        
                        // Si le roi n'est PAS en échec après le coup, c'est un coup légal
                        if (!IsKingInCheck(board, playerColor))
                        {
                            if (count < MAX_MOVES)
                            {
                                movelist[count++] = m;
                            }
                        }
                        
                        // Annuler le coup pour revenir à la position de départ
                        UnmakeMove(board, m); 
                    }
                }
            }
            currentTurn = originalTurn; // Restaure le tour
        }
    }
    currentTurn = originalTurn; // Restaure le tour
    return count;
}


// Fonction d'évaluation simple
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
    int count = GenerateLegalMoves(b, LocalMoveList, playerTurn); 
    if (count == 0)
    {
        // Gérer échec et mat / pat
        if (IsKingInCheck(b, playerTurn))
        {
             return isMax ? -INFINITY : INFINITY; // Mat
        }
        else
        {
            return 0; // Pat
        }
    }

    if (isMax) // Cherche le meilleur coup pour Blanc (maximise)
    {
        int maxEval = -INFINITY_SCORE;
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
        int minEval = INFINITY_SCORE;
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
    int playerTurn = currentTurn;
    int count = GenerateLegalMoves(board, legalMoves, playerTurn);
    
    int bestScore = (playerTurn == 0) ? -INFINITY_SCORE : INFINITY_SCORE;
    Move bestMove = legalMoves[0];

    if (count == 0) 
    {
        TraceLog(LOG_WARNING, "Aucun coup légal trouvé pour l'IA !");
        if (count > 0) return legalMoves[0];
        return (Move){-1, -1, -1, -1, 0, 0}; 
    }

    for (int i = 0; i < count; i++)
    {
        Move move = legalMoves[i];
        MakeMove(board, move);
        // L'IA joue (Min) si elle est Noir (1), et Max si elle est Blanc (0)
        // L'appel AlphaBeta va évaluer la position du point de vue de l'adversaire (1 - playerTurn)
        int eval = AlphaBeta(board, depth - 1, -INFINITY_SCORE, INFINITY_SCORE, (playerTurn == 0) ? false : true, 1 - playerTurn);
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
        
        // Enregistrement du temps avant le calcul bloquant
        double startTime = GetTime();

        int depth = board->AIDepth; // Défini en fonction de la difficulté
        Move bestMove = FindBestMove(board, depth);

        // Calcul du temps écoulé pendant le calcul
        double endTime = GetTime();
        float calculationTime = (float)(endTime - startTime);

        if (board->timer.blackTime > 0.0f)
        {
            board->timer.blackTime -= calculationTime;
            TraceLog(LOG_INFO, "Temps de Calcul de l'IA déduit du temps des Noirs");
        }

        if (bestMove.startX != -1)
        {
            // Effectuer le coup (utilise MakeMove pour ne pas dupliquer la logique)
            MakeMove(board, bestMove);

            // Gérer la promotion de l'IA (Pion arrive en ligne 7)
            Tile *endTile = &board->tiles[bestMove.endY][bestMove.endX];
            int pieceID = endTile->layers[endTile->layerCount - 1];

            if (pieceID == 7 && bestMove.endY == 7) // Pion noir en ligne 7
            {
                // L'IA choisit la Reine par défaut (ID 9)
                TilePop(endTile); // Enlève le pion
                TilePush(endTile, 9); // Met la Reine
                TraceLog(LOG_INFO, "Promotion de l'IA (Noir) en Reine.");
            }

            // Vérification de victoire (le roi capturé est géré dans MakeMove/UnmakeMove,
            // mais l'état de la partie doit être mis à jour ici pour le jeu réel)
            if (bestMove.capturedPieceID == 10) // Capture le Roi Blanc
            {
                board->winner = ID_IA;
                board->state = STATE_GAMEOVER;
                TraceLog(LOG_INFO, "ROI BLANC CAPTURE PAR L'IA ! PARTIE TERMINEE");
            }
            
            // Changement de tour
            currentTurn = 1 - currentTurn;
        }
        else
        {
             // Si l'IA n'a pas trouvé de coup légal, c'est mat ou pat
             if (IsKingInCheck(board, ID_IA))
             {
                 board->state = STATE_GAMEOVER;
                 board->winner = 0; // Mat: Blanc gagne
                 TraceLog(LOG_INFO, "ECHEC ET MAT ! L'IA NE PEUT PLUS BOUGER");
             }
             else
             {
                 board->state = STATE_GAMEOVER;
                 board->winner = -1; // Pat: Nul
                 TraceLog(LOG_INFO, "PAT ! L'IA NE PEUT PLUS BOUGER");
             }
        }
    }
}

// INITIALISATION & RESET

void GameInit(Board *board) 
{
    // On parcourt tout le plateau pour placer les pièces
    for (int y = 0; y < BOARD_ROWS; y++) 
    {
        for (int x = 0; x < BOARD_COLS; x++) 
        {
            Tile *t = &board->tiles[y][x];
            TileClear(t); // On nettoie la case
            
            // Ajout du sol (Carreaux)
            int groundIndex = (x + y) % 2;
            TilePush(t, groundIndex);

            // PLACEMENT DES PIECES 
            
            // Pions Noirs (Ligne 1)
            if (y == 1) TilePush(t, 7); 
            
            // Pions Blancs (Ligne 6)
            if (y == 6) TilePush(t, 6); 

            // Pièces Nobles Noires (Ligne 0)
            if (y == 0) 
            { 
                if (x == 0 || x == 7) TilePush(t, 13); // Tour (13)
                if (x == 1 || x == 6) TilePush(t, 3);  // Cavalier (3)
                if (x == 2 || x == 5) TilePush(t, 5);  // Fou (5)
                if (x == 3) TilePush(t, 9);            // Reine (9)
                if (x == 4) TilePush(t, 11);           // Roi (11)
            }
            
            // Pièces Nobles Blanches (Ligne 7)
            if (y == 7) 
            { 
                if (x == 0 || x == 7) TilePush(t, 12); // Tour (12)
                if (x == 1 || x == 6) TilePush(t, 2);  // Cavalier (2)
                if (x == 2 || x == 5) TilePush(t, 4);  // Fou (4)
                if (x == 3) TilePush(t, 8);            // Reine (8)
                if (x == 4) TilePush(t, 10);           // Roi (10)
            }
        }
    }
    
    // Initialisation des variables de jeu
    board->timer.whiteTime = 600.0f; 
    board->timer.blackTime = 600.0f;
    board->state = STATE_MAIN_MENU;
    board->mode = MODE_NONE;
    board->winner = -1;
    board->difficulty = DIFF_MEDIUM;
    board->AIDepth = 3;
    board->AIDefaultDelay = 2.0f;
    board->IADelay = 0.0f; // Ajout de la variable d'IA
    currentTurn = 0; 
    selectedX = -1; 
    selectedY = -1;
    possibleMoveCount = 0;
    promotionPending = 0;
}

// Raccourci pour redémarrer
void GameReset(Board *board) 
{ 
    GameInit(board); 
}

// 7. GESTION DES ÉVÉNEMENTS (INPUTS)

static void GameLogicUpdate(Board *board, float dt)
{
    // GESTION DE LA PROMOTION (Si un pion atteint le bout)
    if (promotionPending == 1) 
    {
        Tile *promTile = &board->tiles[promotionY][promotionX];
        bool selected = false;
        int newPieceIdx = -1;

        // Choix (1:Reine, 2:Cavalier, 3:Tour, 4:Fou)
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
            newPieceIdx = (promotionColor == 1) ? 9 : 8; // Reine (Noir/Blanc)
            selected = true; 
        } 
        else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
            newPieceIdx = (promotionColor == 1) ? 3 : 2; // Cavalier
            selected = true; 
        } 
        else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
            newPieceIdx = (promotionColor == 1) ? 13 : 12; // Tour
            selected = true; 
        } 
        else if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) {
            newPieceIdx = (promotionColor == 1) ? 5 : 4; // Fou
            selected = true; 
        }

        // Application du choix
        if (selected) 
        {
            TilePop(promTile); // Enlève le pion
            TilePush(promTile, newPieceIdx); // Met la nouvelle pièce
            
            // Réinitialisation après promotion
            promotionPending = 0;
            selectedX = -1; 
            selectedY = -1;
            possibleMoveCount = 0;
            currentTurn = 1 - currentTurn; // Le tour change enfin
        }
        return; // IMPORTANT : On bloque le jeu tant que la promotion n'est pas choisie
    }

    // GESTION DE LA SOURIS (Jeu normal)
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition(); 
        
        // Calculs pour savoir sur quelle case on clique (Assure des cases carrées)
        int screenW = GetScreenWidth(); 
        int screenH = GetScreenHeight(); 
        int tileSizeW = screenW / BOARD_COLS;
        int tileSizeH = screenH / BOARD_ROWS;
        int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH;
        int offsetX = (screenW - (tileSize * BOARD_COLS)) / 2; 
        int offsetY = (screenH - (tileSize * BOARD_ROWS)) / 2; 
        
        int x = (int)((m.x - offsetX) / tileSize); 
        int y = (int)((m.y - offsetY) / tileSize); 

        // Si le clic est bien DANS le plateau
        if (x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS) 
        {
            Tile *clickedTile = &board->tiles[y][x];

            // CAS 1 : JE SÉLECTIONNE UNE PIÈCE
            if (selectedX == -1) 
            {
                if (clickedTile->layerCount > 1) 
                {
                    int pieceID = clickedTile->layers[clickedTile->layerCount - 1]; 
                    
                    if (GetPieceColor(pieceID) == currentTurn) 
                    {
                        selectedX = x; 
                        selectedY = y;
                        TraceLog(LOG_INFO, "Selection de la piece en %d, %d", x, y); 
                        
                        // On doit simuler la sélection en déplaçant la pièce sur le board temporaire
                        int originalTurn = currentTurn;
                        currentTurn = GetPieceColor(pieceID); 
                        
                        // Pour la vérification des coups légaux, on parcourt toutes les cases possibles
                        possibleMoveCount = 0;
                        for (int py = 0; py < BOARD_ROWS; py++) 
                        {
                            for (int px = 0; px < BOARD_COLS; px++) 
                            {
                                if (IsMoveValid(board, selectedX, selectedY, px, py))
                                {
                                    // SIMULATION DE SÉCURITÉ (Copie du plateau pour la simulation)
                                    Board temp = *board;
                                    Move m = {selectedX, selectedY, px, py, pieceID, 0};
                                    Tile *sNew = &temp.tiles[py][px];
                                    
                                    // Récupérer la pièce capturée (si nécessaire)
                                    if(sNew->layerCount > 1) 
                                    {
                                         m.capturedPieceID = sNew->layers[sNew->layerCount-1];
                                    }

                                    // Faire le coup simulé
                                    MakeMove(&temp, m);
                                    
                                    // Si ce coup ne met pas mon Roi en échec, je l'ajoute à la liste affichée
                                    if (!IsKingInCheck(&temp, currentTurn)) 
                                    {
                                        possibleMoves[possibleMoveCount][0] = px;
                                        possibleMoves[possibleMoveCount][1] = py;
                                        possibleMoveCount++;
                                    }
                                }
                            }
                        }
                        currentTurn = originalTurn; // Restaure le tour
                    }
                }
            }

            // CAS 2 : JE DÉPLACE LA PIÈCE SÉLECTIONNÉE
            else 
            {
                int startX = selectedX; 
                int startY = selectedY; 
                int endX = x; 
                int endY = y; 
                
                // Annulation (Clic sur soi-même)
                if (endX == startX && endY == startY) 
                {
                    selectedX = -1;
                    selectedY = -1;
                    possibleMoveCount = 0; 
                    return; 
                }

                // Vérifier si c'est un coup possible (dans la liste précalculée)
                bool moveAllowed = false;
                for (int i = 0; i < possibleMoveCount; i++)
                {
                    if (possibleMoves[i][0] == endX && possibleMoves[i][1] == endY)
                    {
                        moveAllowed = true;
                        break;
                    }
                }

                // Exécution réelle du coup si autorisé
                if (moveAllowed)
                {
                    Tile *oldTile = &board->tiles[selectedY][selectedX]; 
                    int pieceID = oldTile->layers[oldTile->layerCount - 1];

                    Move actualMove = {startX, startY, endX, endY, pieceID, 0};
                    
                    // Pré-calcul de la pièce capturée
                    if (clickedTile->layerCount > 1) 
                    {
                        actualMove.capturedPieceID = clickedTile->layers[clickedTile->layerCount - 1];
                    }

                    // On utilise MakeMove pour le déplacement et la gestion du roque
                    MakeMove(board, actualMove);
                    
                    // GESTION SPÉCIALE : PROMOTION
                    // Pion blanc arrive en haut (0) OU Pion noir arrive en bas (7)
                    if ((pieceID == 6 && endY == 0) || (pieceID == 7 && endY == 7)) 
                    {
                        // On déclenche le mode Promotion
                        promotionPending = 1;
                        promotionX = endX; 
                        promotionY = endY;
                        promotionColor = (pieceID == 7) ? 1 : 0;
                        
                    }
                    else 
                    {
                        // Changement de tour (si la partie continue)
                        if (board->state != STATE_GAMEOVER) 
                        {
                            currentTurn = 1 - currentTurn;
                        }
                    }

                    // Vérification de victoire par capture de Roi
                    if (actualMove.capturedPieceID == 10 || actualMove.capturedPieceID == 11)
                    {
                         board->winner = 1 - currentTurn; // L'adversaire du joueur qui vient de jouer
                         board->state = STATE_GAMEOVER;
                         TraceLog(LOG_INFO, "ROI CAPTURE ! PARTIE TERMINEE");
                    }

                    // Réinitialisation de la sélection
                    selectedX = -1; 
                    selectedY = -1; 
                    possibleMoveCount = 0; 
                    
                    // Lancement de l'IA si nécessaire
                    if (currentTurn == ID_IA && board->mode == MODE_PLAYER_VS_IA && promotionPending == 0)
                    {
                         board->IADelay = board->AIDefaultDelay; // Délai pour l'IA
                    }
                }
                else 
                {
                    // Si le coup est invalide, mais qu'on a cliqué sur une autre pièce à nous
                    // On change simplement la sélection
                    if (clickedTile->layerCount > 1 && GetPieceColor(clickedTile->layers[clickedTile->layerCount-1]) == currentTurn) 
                    {
                        selectedX = -1; 
                        possibleMoveCount = 0;
                        // On force la re-sélection en appelant à nouveau la fonction
                        GameLogicUpdate(board, dt); 
                        return;
                    }
                    // Si le coup est invalide et qu'on a cliqué sur une case vide ou un ennemi
                    selectedX = -1; 
                    selectedY = -1; 
                    possibleMoveCount = 0; 
                }
            }
        }
        else 
        { 
            // Clic hors du plateau -> Désélection
            selectedX = -1; 
            selectedY = -1; 
            possibleMoveCount = 0; 
        }
    }
}

// 8. MISE À JOUR PRINCIPALE (UPDATE)

// Vérifie si le joueur a au moins UN coup légal qui sauve son Roi
static bool HasLegalMoves(Board *board, int color)
{
    Move movelist[MAX_MOVES];
    int count = GenerateLegalMoves(board, movelist, color);
    return count > 0;
}

void GameUpdate(Board *board, float dt)
{
    if (board->state == STATE_MAIN_MENU)
    {
        // Gestion du clic pour choisir le mode de jeu
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 m = GetMousePosition();
            int centerW = GetScreenWidth() / 2;
            int centerH = GetScreenHeight() / 2;
            
            // Bouton 1v1
            if (m.x > centerW - 150 && m.x < centerW + 150 && m.y > centerH - 60 && m.y < centerH - 60 + 50) 
            {
                board->mode = MODE_PLAYER_VS_PLAYER;
                board->state = STATE_TIME_MENU;
                TraceLog(LOG_INFO, "Mode 1v1 sélectionné. Passage au menu du temps.");
            }
            // Bouton VS IA
            else if (m.x > centerW - 150 && m.x < centerW + 150 && m.y > centerH + 10 && m.y < centerH + 10 + 50) 
            {
                board->state = STATE_DIFFICULTY_MENU;
                TraceLog(LOG_INFO, "Transition vers le menu de difficulté IA.");
            }
        }
    }
    else if (board->state == STATE_TIME_MENU)
    {
        if (IsKeyPressed(KEY_ESCAPE))
        {
            board->state = STATE_MAIN_MENU;
            return;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 m = GetMousePosition();
            int centerW = GetScreenWidth() / 2;
            int centerH = GetScreenHeight() / 2;

            // 10 minutes

            if (m.x > centerW - 150 && m.x < centerW + 150 && m.y > centerH - 80 && m.y < centerH - 80 + 50)
            {
                board->timer.blackTime = 600.0f;
                board->timer.whiteTime = 600.0f;
                board->mode = MODE_PLAYER_VS_PLAYER;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "10 minutes sélectionné");
            }
            // 3 minutes

            else if (m.x > centerW - 150 && m.x < centerW + 150 && m.y > centerH - 10 && m.y < centerH - 10 + 50)
            {
                board->timer.blackTime = 180.0f;
                board->timer.whiteTime = 180.0f;
                board->mode = MODE_PLAYER_VS_PLAYER;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "3 minutes sélectionné");
            }
            // 1 minute
            else if (m.x > centerW - 150 && m.x < centerW + 150 && m.y > centerH + 60 && m.y < centerH + 60 + 50)
            {
                board->timer.blackTime = 60.0f;
                board->timer.whiteTime = 60.0f;
                board->mode = MODE_PLAYER_VS_PLAYER;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "1 minute sélectionné");
            }
        }
    }
    else if(board->state == STATE_DIFFICULTY_MENU)
    {
        if (IsKeyPressed(KEY_ESCAPE))
        {
            board->state = STATE_MAIN_MENU;
            return;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 m = GetMousePosition();
            int centerW = GetScreenWidth() / 2;
            int centerH = GetScreenHeight() / 2;

            // Facile
            if (m.x > centerW - 150 && m.x < centerW + 150 && m.y > centerH - 80 && m.y < centerH - 80 + 50)
            {
                board->difficulty = DIFF_EASY;
                board->AIDepth = 1;
                board->AIDefaultDelay = 3.0f;
                board->mode = MODE_PLAYER_VS_IA;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "Difficulté : Facile");
            }
            // Intermédiaire
            else if (m.x > centerW - 150 && m.x < centerW + 150 && m.y >centerH - 10 && m.y < centerH - 10 + 50)
            {
                board->difficulty = DIFF_MEDIUM;
                board->AIDepth = 3;
                board->AIDefaultDelay = 2.0f;
                board->mode = MODE_PLAYER_VS_IA;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "Difficulté : Intermédiaire");
            }
            // Difficile
            else if (m.x > centerW - 150 && m.x < centerW + 150 && m.y > centerH + 60 && m.y < centerH + 60 + 50)
            {
                board->difficulty = DIFF_HARD;
                board->AIDepth = 5;
                board->AIDefaultDelay = 1.0f;
                board->mode = MODE_PLAYER_VS_IA;
                board->state = STATE_PLAYING;
                TraceLog(LOG_INFO, "Difficulté : Difficile");
            }
        }
    }
    else if (board->state == STATE_PLAYING)
    {
        // GESTION DU TEMPS
        if (currentTurn == 0) {
            if (board->timer.whiteTime > 0.0f) board->timer.whiteTime -= dt; 
        } else {
            if (board->timer.blackTime > 0.0f) board->timer.blackTime -= dt; 
        }

        // VÉRIFICATION DÉFAITE PAR TEMPS 
        if (board->timer.whiteTime <= 0.0f) {
            board->state = STATE_GAMEOVER; 
            board->winner = 1; // Noirs gagnent
            TraceLog(LOG_WARNING, "GAME OVER - Temps BLANC écoulé !");
            return;
        }
        if (board->timer.blackTime <= 0.0f) {
            board->state = STATE_GAMEOVER; 
            board->winner = 0; // Blancs gagnent
            TraceLog(LOG_WARNING, "GAME OVER - Temps NOIR écoulé !");
            return;
        }
        
        // LOGIQUE IA 
        if (board->mode == MODE_PLAYER_VS_IA && currentTurn == ID_IA)
        {
             AIMakeMove(board, dt);
             return; // L'IA prend le contrôle total du tour
        }
        
        
        // GESTION ABANDON (FORFAIT) 
        if (IsKeyPressed(KEY_F))
        {
            board->state = STATE_GAMEOVER;
            board->winner = 1 - currentTurn; // Le gagnant est l'adversaire
            TraceLog(LOG_WARNING, "Le joueur %s a déclaré forfait (F).", (currentTurn == 0) ? "BLANC" : "NOIR");
            return;
        }

        // DETECTION DE FIN DE PARTIE (MAT / PAT) 
        if (promotionPending == 0 && !HasLegalMoves(board, currentTurn))
        {
            bool check = IsKingInCheck(board, currentTurn);
            
            board->state = STATE_GAMEOVER;
            
            if (check) 
            {
                board->winner = 1 - currentTurn; // Mat: L'adversaire gagne
                TraceLog(LOG_INFO, "ECHEC ET MAT !");
            } 
            else 
            {
                board->winner = -1; // Pat: Nul
                TraceLog(LOG_INFO, "PAT (Match Nul) !");
            }
            return;
        }

        // Mise à jour de la logique de jeu (Souris, etc.)
        GameLogicUpdate(board, dt);
    }
    // Si la partie est terminée
    else if (board->state == STATE_GAMEOVER)
    {
        // Touche R pour recommencer ou clic sur le bouton "Rejouer"
        if (IsKeyPressed(KEY_R) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
        {
            Vector2 m = GetMousePosition();
            int cW = GetScreenWidth() / 2; 
            int cH = GetScreenHeight() / 2;
            
            if (IsKeyPressed(KEY_R) || (m.x > cW - 160 && m.x < cW + 160 && m.y > cH - 5 && m.y < cH + 35)) 
            {
                GameInit(board);
                TraceLog(LOG_INFO, "Nouvelle partie lancée.");
            }
        }
    }
}

// 9. DESSIN

void GameDraw(const Board *board)
{
    int screenW = GetScreenWidth(); 
    int screenH = GetScreenHeight();
    const int FONT_SIZE = 30; 
    const int TEXT_PADDING = 20;

    // Calcul taille dynamique
    int tileSizeW = screenW / BOARD_COLS;
    int tileSizeH = screenH / BOARD_ROWS;
    int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH;
    
    int boardW = tileSize * BOARD_COLS;
    int boardH = tileSize * BOARD_ROWS;
    int offsetX = (screenW - boardW) / 2; 
    int offsetY = (screenH - boardH) / 2; 

    // ÉCRAN PRINCIPAL (Plateau, Timers)
    if (board->state == STATE_PLAYING || board->state == STATE_GAMEOVER)
    {
        // DESSIN DU PLATEAU ET DES PIÈCES
        for (int y = 0; y < BOARD_ROWS; y++) 
        {
            for (int x = 0; x < BOARD_COLS; x++) 
            {
                const Tile *t = &board->tiles[y][x];
                int dX = offsetX + x * tileSize; 
                int dY = offsetY + y * tileSize;

                // Dessin des couches (Sol + Pièce éventuelle)
                for (int i = 0; i < t->layerCount; i++) 
                {
                    int idx = t->layers[i];
                    DrawTexturePro(
                        gTileTextures[idx],
                        (Rectangle){0, 0, gTileTextures[idx].width, gTileTextures[idx].height},
                        (Rectangle){(float)dX, (float)dY, (float)tileSize, (float)tileSize},
                        (Vector2){0,0}, 0, WHITE
                    ); 
                }
                
                // Bordure grise discrète
                DrawRectangleLines(dX, dY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
            }
        }

        // INDICATEUR VISUEL D'ECHEC (Carré Rouge sous le Roi)
        if (board->state == STATE_PLAYING && IsKingInCheck(board, currentTurn)) 
        {
            int kingID = (currentTurn == 0) ? 10 : 11;
            
            for(int y = 0; y < BOARD_ROWS; y++) 
            {
                for(int x = 0; x < BOARD_COLS; x++) 
                {
                    const Tile *t = &board->tiles[y][x];
                    if(t->layerCount > 1 && t->layers[t->layerCount-1] == kingID) 
                    {
                        DrawRectangle(offsetX + x * tileSize, offsetY + y * tileSize, tileSize, tileSize, Fade(RED, 0.6f));
                    }
                }
            }
        }

        // DESSIN DES COUPS POSSIBLES (Aide visuelle)
        for (int i = 0; i < possibleMoveCount; i++) 
        {
            int x = possibleMoves[i][0]; 
            int y = possibleMoves[i][1];
            int dX = offsetX + x * tileSize; 
            int dY = offsetY + y * tileSize;
            
            const Tile *t = &board->tiles[y][x];
            
            // Si c'est un ennemi -> Carré rouge
            if (t->layerCount > 1 && GetPieceColor(t->layers[t->layerCount - 1]) != currentTurn) 
            {
                DrawRectangleLinesEx((Rectangle){(float)dX, (float)dY, (float)tileSize, (float)tileSize}, 5, Fade(RED, 0.6f));
            }
            // Si c'est vide -> Rond gris
            else 
            {
                DrawCircle(dX + tileSize/2, dY + tileSize/2, tileSize/8, Fade(DARKGRAY, 0.5f));
            }
        }

        // DESSIN DE LA SÉLECTION (Cadre Vert)
        if (selectedX != -1) 
        {
            DrawRectangleLinesEx(
                (Rectangle){(float)offsetX + selectedX * tileSize, (float)offsetY + selectedY * tileSize, (float)tileSize, (float)tileSize}, 
                4, 
                GREEN
            ); 
        }

        // DESSIN DES TIMERS
        int centerTextY = offsetY + boardH / 2 - FONT_SIZE / 2;
        
        Color whiteColor = (currentTurn == 0 && board->state == STATE_PLAYING) ? RAYWHITE : DARKGRAY;
        int whiteM = (int)board->timer.whiteTime / 60;
        int whiteS = (int)board->timer.whiteTime % 60;
        DrawText(TextFormat("BLANCS\n%02d:%02d", whiteM, whiteS), offsetX - MeasureText("BLANCS", FONT_SIZE) - TEXT_PADDING, centerTextY, FONT_SIZE, whiteColor); 

        Color blackColor = (currentTurn == 1 && board->state == STATE_PLAYING) ? RAYWHITE : DARKGRAY;
        int blackM = (int)board->timer.blackTime / 60;
        int blackS = (int)board->timer.blackTime % 60;
        DrawText(TextFormat("NOIRS\n%02d:%02d", blackM, blackS), offsetX + boardW + TEXT_PADDING, centerTextY, FONT_SIZE, blackColor); 
    }

    // MENU DE PROMOTION (Superposé)
    if (promotionPending == 1) 
    {
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.85f));
        
        const char *title = "PROMOTION !";
        const char *opts = "1: REINE   2: CAVALIER   3: TOUR   4: FOU";
        
        DrawText(title, screenW/2 - MeasureText(title, 40)/2, screenH/2 - 100, 40, YELLOW);
        DrawText(opts, screenW/2 - MeasureText(opts, 20)/2, screenH/2, 20, WHITE);
    }

    // ECRAN DE FIN DE PARTIE (Game Over)
    if (board->state == STATE_GAMEOVER)
    {
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.85f)); 
        
        const char *txt; 
        Color c;
        
        if (board->winner == -1) 
        { 
            txt = "MATCH NUL (PAT)"; 
            c = BLUE; 
        }
        else 
        { 
            txt = (board->winner == 0) ? "VICTOIRE BLANCS !" : "VICTOIRE NOIRS !"; 
            c = GREEN; 
        }
        
        // Affichage du vainqueur
        DrawText(txt, screenW/2 - MeasureText(txt, 50)/2, screenH/2 - 100, 50, c);
        
        // Bouton Rejouer
        const char *menuText = "CLIQUEZ / APPUYEZ SUR R POUR REJOUER";
        int mWidth = MeasureText(menuText, 30);
        
        DrawText(menuText, screenW/2 - mWidth/2, screenH/2, 30, YELLOW);
        DrawRectangleLines(screenW/2 - 160, screenH/2 - 5, 320, 40, YELLOW);
        
        DrawText("Appuyer sur ECHAP pour quitter", screenW/2 - MeasureText("Appuyer sur ECHAP pour quitter", 20)/2, screenH/2 + 100, 20, RAYWHITE);
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

    // Menu de difficulté IA
    else if (board->state == STATE_DIFFICULTY_MENU)
    {
        int centerW = screenW / 2;
        int centerH = screenH / 2;

        DrawRectangle(0, 0, screenW, screenH, BLACK);

        const char *titleText = "CHOIX DE LA DIFFICULTÉ IA";

        int titleWidth = MeasureText(titleText, 60);

        DrawText(titleText, centerW - titleWidth/2, centerH - 200, 60, RAYWHITE);

        const char *easyText = "Facile";
        DrawRectangleLines(centerW - 150, centerH - 80, 300, 50, GREEN);
        DrawText(easyText, centerW - MeasureText(easyText, 25)/2, centerH - 70,25, RAYWHITE);

        const char *mediumText = "Intermédiaire";
        DrawRectangleLines(centerW - 150, centerH - 10, 300, 50, YELLOW);
        DrawText(mediumText, centerW - MeasureText(mediumText, 25)/2, centerH, 25, RAYWHITE);

        const char *hardText = "Difficile";
        DrawRectangleLines(centerW - 150, centerH + 60, 300, 50, RED);
        DrawText(hardText, centerW - MeasureText(hardText, 25)/2, centerH + 70,25, RAYWHITE);

        const char *backText = "Appuyer sur ECHAP pour revenir au menu principal";
        DrawText(backText, centerW - MeasureText(backText, 20)/2, screenH - 50, 20, DARKGRAY);
    }
    // Menu de choix temps
    else if (board->state == STATE_TIME_MENU)
    {
        int centerW = screenW / 2;
        int centerH = screenH / 2;

        DrawRectangle(0, 0, screenW, screenH, BLACK);

        const char *titleText = "CHOIX DU TEMPS";

        int titleWidth = MeasureText(titleText, 60);

        DrawText(titleText, centerW - titleWidth/2, centerH - 200, 60, RAYWHITE);

        const char *dixminText = "10 minutes";
        DrawRectangleLines(centerW - 150, centerH - 80, 300, 50, YELLOW);
        DrawText(dixminText, centerW - MeasureText(dixminText, 25)/2, centerH - 70,25, RAYWHITE);

        const char *troisminText = "3 minutes";
        DrawRectangleLines(centerW - 150, centerH - 10, 300, 50, YELLOW);
        DrawText(troisminText, centerW - MeasureText(troisminText, 25)/2, centerH, 25, RAYWHITE);

        const char *uneminText = "1 minute";
        DrawRectangleLines(centerW - 150, centerH + 60, 300, 50, YELLOW);
        DrawText(uneminText, centerW - MeasureText(uneminText, 25)/2, centerH + 70,25, RAYWHITE);

        const char *backText = "Appuyer sur ECHAP pour revenir au menu principal";
        DrawText(backText, centerW - MeasureText(backText, 20)/2, screenH - 50, 20, DARKGRAY);
    }
}