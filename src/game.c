#include "game.h"
#include <stdio.h>  // Pour TraceLog et printf
#include <stdlib.h> // Pour abs()

// ==========================================
// 0. IMPORTATIONS EXTERNES
// ==========================================
extern Texture2D gTileTextures[]; 
extern int gTileTextureCount; 

// ==========================================
// 1. VARIABLES GLOBALES
// ==========================================
// Gestion de la sélection
int selectedX = -1; 
int selectedY = -1; 
int currentTurn = 0; // 0 = Blancs, 1 = Noirs

// Gestion de la Promotion (Pion -> Reine/Tour/Etc)
int promotionPending = 0; // 1 si le jeu est en pause pour choisir une pièce
int promotionX = -1;
int promotionY = -1;
int promotionColor = -1;

// Gestion des coups possibles (pour l'affichage des ronds/cadres rouges)
#define MAX_MOVES 64 
static int possibleMoves[MAX_MOVES][2]; 
static int possibleMoveCount = 0; 

// ==========================================
// 2. FONCTIONS UTILITAIRES (HELPERS)
// ==========================================

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
    
    // Astuce : Les ID pairs sont blancs, les impairs sont noirs (dans ton chargement)
    if (textureID % 2 == 0)
    {
        return 0; // Blanc
    }
    else 
    {
        return 1; // Noir
    }
}

// ==========================================
// 3. LOGIQUE DE DÉPLACEMENT (PHYSIQUE)
// ==========================================

// Vérifie si le chemin est libre entre A et B (pour Tour, Fou, Reine)
static bool IsPathClear(const Board *board, int startX, int startY, int endX, int endY)
{
    // --- Cas 1 : Déplacement Horizontal ---
    if (startY == endY) 
    {
        int step;
        if (endX > startX) step = 1; else step = -1;

        // On parcourt les cases ENTRE le départ et l'arrivée
        for (int x = startX + step; x != endX; x += step)
        {
            if (board->tiles[startY][x].layerCount > 1) 
            {
                return false; // Obstacle trouvé
            }
        }
    }
    // --- Cas 2 : Déplacement Vertical ---
    else if (startX == endX)
    {
        int step;
        if (endY > startY) step = 1; else step = -1;

        for (int y = startY + step; y != endY; y += step)
        {
            if (board->tiles[y][startX].layerCount > 1) 
            {
                return false; // Obstacle trouvé
            }
        }
    }
    // --- Cas 3 : Déplacement Diagonal ---
    else if (abs(endX - startX) == abs(endY - startY)) 
    {
        int stepX;
        if (endX > startX) stepX = 1; else stepX = -1;

        int stepY;
        if (endY > startY) stepY = 1; else stepY = -1;

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

    // --- ANALYSE SELON LA PIÈCE ---

    // 1. TOUR (ID 12 Blanc, 13 Noir)
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
    // 2. FOU (ID 4 Blanc, 5 Noir)
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
    // 3. REINE (ID 8 Blanc, 9 Noir)
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
    // 4. ROI (ID 10 Blanc, 11 Noir)
    else if (pieceID == 10 || pieceID == 11) 
    {
        // A. Déplacement 1 case dans toutes les directions
        if (abs(dx) <= 1 && abs(dy) <= 1) 
        {
            ruleMatch = true;
        }
        // B. ROQUE
        else if (dy == 0 && (dx == 2 || dx == -2))
        {
            // Note : L'implémentation complète du Roque (vérification de si le roi est en échec,
            // ou passe par une case en échec) devrait se faire DANS HasLegalMoves ou GameLogicUpdate,
            // ici on ne vérifie que la règle de base (chemin clair et tours présentes).

            // On détermine la colonne de la Tour visée.
            int rookX = (dx == 2) ? startX + 3 : startX - 4; // dx=2 -> Tour en x+3 (7); dx=-2 -> Tour en x-4 (0)
            
            // Sécurité : la colonne est-elle dans le plateau ?
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
                        // Attention : la fonction IsPathClear va jusqu'à la destination (la Tour),
                        // ce qui est correct pour le roque puisque le roi ne doit pas passer par la colonne de la Tour.
                        if (IsPathClear(board, startX, startY, rookX, startY))
                        {
                            ruleMatch = true;
                        }
                    }
                }
            }
        }
    }
    // 5. CAVALIER (ID 2 Blanc, 3 Noir)
    else if (pieceID == 2 || pieceID == 3) 
    {
        // Mouvement en "L" (2 cases d'un côté, 1 case de l'autre)
        if ((abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1)) 
        {
            ruleMatch = true;
        }
    }
    // 6. PION (ID 6 Blanc, 7 Noir)
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
        
        // A. Capture en diagonale
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
        // B. Avance simple (1 case)
        else if (dx == 0 && dy == direction) 
        {
            // La case cible doit être vide
            if (targetTile->layerCount == 1) 
            {
                ruleMatch = true;
            }
        }
        // C. Double avance (Premier tour)
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

    // --- VÉRIFICATION COLLISION ALLIÉE ---
    // On ne peut pas manger ses propres pièces
    if (targetTile->layerCount > 1) 
    {
        int targetID = targetTile->layers[targetTile->layerCount - 1];
        int targetColor = GetPieceColor(targetID);
        
        if (targetColor != -1 && targetColor == currentTurnColor) 
        {
            return false; // Bloqué par un ami
        }
    }

    return true; 
}

// ==========================================
// 4. LOGIQUE DE SÉCURITÉ (ECHEC / MAT)
// ==========================================

// Détecte si le Roi d'une couleur donnée est menacé ACTUELLEMENT
static bool IsKingInCheck(const Board *board, int kingColor)
{
    int kingX = -1;
    int kingY = -1;
    int targetKingID = (kingColor == 0) ? 10 : 11; // 10=Blanc, 11=Noir

    // 1. On cherche où est le Roi
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
    
    // Si on ne trouve pas le roi (bug grave), on renvoie faux
    if (kingX == -1 || kingY == -1) return false;

    // 2. On regarde si un ennemi peut attaquer cette case (kingX, kingY)
    for (int y = 0; y < BOARD_ROWS; y++) 
    {
        for (int x = 0; x < BOARD_COLS; x++) 
        {
            const Tile *t = &board->tiles[y][x];
            
            // Si la case est vide, on passe
            if (t->layerCount <= 1) continue;

            int attackerID = t->layers[t->layerCount - 1];
            int attackerColor = GetPieceColor(attackerID);

            // Si c'est un allié, on passe
            if (attackerColor == -1 || attackerColor == kingColor) continue;

            // Si cet ennemi peut légalement aller sur la case du Roi
            // NOTE IMPORTANTE : IsMoveValid doit être pur et ne pas faire de vérification d'échec
            // car ici on simule l'attaque (sauf pour le roque qui est géré plus bas dans GameLogicUpdate)
            if (IsMoveValid(board, x, y, kingX, kingY)) 
            {
                return true; // Le Roi est en échec !
            }
        }
    }
    return false;
}

// Vérifie si le joueur a au moins UN coup légal qui sauve son Roi
// Utilisé pour détecter le MAT ou le PAT
static bool HasLegalMoves(const Board *board, int color)
{
    // On parcourt toutes les cases du plateau (Départ)
    for (int startY = 0; startY < BOARD_ROWS; startY++) 
    {
        for (int startX = 0; startX < BOARD_COLS; startX++) 
        {
            const Tile *t = &board->tiles[startY][startX];
            
            // Si c'est une pièce à nous
            if (t->layerCount > 1 && GetPieceColor(t->layers[t->layerCount-1]) == color) 
            {
                // On essaie toutes les cases du plateau (Arrivée)
                for (int endY = 0; endY < BOARD_ROWS; endY++) 
                {
                    for (int endX = 0; endX < BOARD_COLS; endX++) 
                    {
                        // 1. Est-ce que le mouvement est physiquement possible ?
                        if (IsMoveValid(board, startX, startY, endX, endY)) 
                        {
                            // 2. SIMULATION : On joue le coup sur une copie
                            Board temp = *board;
                            Tile *sOld = &temp.tiles[startY][startX];
                            Tile *sNew = &temp.tiles[endY][endX];
                            
                            int id = TilePop(sOld);
                            
                            // Gérer la capture
                            if (sNew->layerCount > 1) 
                            {
                                // On vérifie si on mange un allié (ce qui est normalement bloqué par IsMoveValid)
                                // Mais on le pop si c'est un ennemi
                                int targetColor = GetPieceColor(sNew->layers[sNew->layerCount - 1]);
                                if (targetColor != -1 && targetColor != color) 
                                {
                                    TilePop(sNew); // Mange si nécessaire
                                } 
                                // Si on tente de manger un allié, on ne pousse pas la pièce (la simulation est terminée)
                                else if (targetColor == color) 
                                {
                                    continue; // Ce coup est déjà invalide (sécurité)
                                }
                            }
                            
                            TilePush(sNew, id); // Place la pièce
                            
                            // 3. VERIFICATION : Est-ce que mon Roi est sauf après ce coup ?
                            if (!IsKingInCheck(&temp, color)) 
                            {
                                return true; // On a trouvé un coup valide ! Pas Mat.
                            }
                        }
                    }
                }
            }
        }
    }
    return false; // Aucun coup trouvé : C'est la fin (Mat ou Pat).
}

// ==========================================
// 5. INITIALISATION
// ==========================================

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

            // --- PLACEMENT DES PIECES ---
            
            // Pions Noirs (Ligne 1)
            if (y == 1) TilePush(t, 7); 
            
            // Pions Blancs (Ligne 6)
            if (y == 6) TilePush(t, 6); 

            // Pièces Nobles Noires (Ligne 0)
            if (y == 0) 
            { 
                if (x == 0 || x == 7) TilePush(t, 13); // Tour
                if (x == 1 || x == 6) TilePush(t, 3);  // Cavalier
                if (x == 2 || x == 5) TilePush(t, 5);  // Fou
                if (x == 3) TilePush(t, 9);            // Reine
                if (x == 4) TilePush(t, 11);           // Roi
            }
            
            // Pièces Nobles Blanches (Ligne 7)
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
    
    // Initialisation des variables de jeu
    board->timer.whiteTime = 600.0f; 
    board->timer.blackTime = 600.0f;
    board->state = STATE_PLAYING;
    board->winner = -1;
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

// ==========================================
// 6. GESTION DES ÉVÉNEMENTS (INPUTS)
// ==========================================

static void GameLogicUpdate(Board *board, float dt)
{
    // --- A. GESTION DE LA PROMOTION (Si un pion atteint le bout) ---
    if (promotionPending == 1) 
    {
        Tile *promTile = &board->tiles[promotionY][promotionX];
        bool selected = false;
        int newPieceIdx = -1;

        // Choix via clavier
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
            newPieceIdx = (promotionColor == 1) ? 9 : 8; // Reine
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

    // --- B. GESTION DE LA SOURIS (Jeu normal) ---
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 m = GetMousePosition(); 
        
        // Calculs pour savoir sur quelle case on clique
        int screenW = GetScreenWidth(); 
        int screenH = GetScreenHeight(); 
        
        // On s'assure que les cases sont carrées
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

            // -----------------------------
            // CAS 1 : JE SÉLECTIONNE UNE PIÈCE
            // -----------------------------
            if (selectedX == -1) 
            {
                if (clickedTile->layerCount > 1) 
                {
                    int pieceID = clickedTile->layers[clickedTile->layerCount - 1]; 
                    
                    // Je ne peux sélectionner que mes propres pièces
                    if (GetPieceColor(pieceID) == currentTurn) 
                    {
                        selectedX = x; 
                        selectedY = y;
                        TraceLog(LOG_INFO, "Selection de la piece en %d, %d", x, y); 
                        
                        // Calcul préventif des coups possibles (pour l'affichage)
                        possibleMoveCount = 0;
                        for (int py = 0; py < BOARD_ROWS; py++) 
                        {
                            for (int px = 0; px < BOARD_COLS; px++) 
                            {
                                // Si c'est un mouvement physique valide
                                if (IsMoveValid(board, selectedX, selectedY, px, py)) 
                                {
                                    // SIMULATION DE SÉCURITÉ
                                    Board temp = *board;
                                    Tile *sOld = &temp.tiles[selectedY][selectedX];
                                    Tile *sNew = &temp.tiles[py][px];
                                    
                                    int sPid = TilePop(sOld);
                                    
                                    // Gérer la capture (simulée)
                                    if(sNew->layerCount > 1) 
                                    {
                                        int targetColor = GetPieceColor(sNew->layers[sNew->layerCount - 1]);
                                        if (targetColor != -1 && targetColor != currentTurn) 
                                        {
                                            TilePop(sNew);
                                        } else if (targetColor == currentTurn) {
                                            // Ne devrait pas arriver ici si IsMoveValid est correct
                                            continue; 
                                        }
                                    }

                                    TilePush(sNew, sPid);
                                    
                                    // Si ce coup ne tue pas mon Roi, je l'ajoute à la liste affichée
                                    if (!IsKingInCheck(&temp, currentTurn)) 
                                    {
                                        possibleMoves[possibleMoveCount][0] = px;
                                        possibleMoves[possibleMoveCount][1] = py;
                                        possibleMoveCount++;
                                    }
                                }
                            }
                        }  
                    }
                }
            }
            // -----------------------------
            // CAS 2 : JE DÉPLACE LA PIÈCE SÉLECTIONNÉE
            // -----------------------------
            else 
            {
                bool moveAllowed = false;
                Tile *oldTile = &board->tiles[selectedY][selectedX]; 
                int startX = selectedX; 
                int startY = selectedY; 
                int endX = x; 
                int endY = y; 

                // Si je clique sur moi-même -> Annulation
                if (endX == startX && endY == startY) 
                {
                    selectedX = -1; 
                    selectedY = -1; 
                    possibleMoveCount = 0;
                    return; 
                }
                
                // 1. Vérification des règles physiques (Tour, Fou, etc.)
                if (IsMoveValid(board, startX, startY, endX, endY)) 
                {
                    moveAllowed = true;
                }

                // 2. Vérification de sécurité (Anti-Suicide du Roi)
                if (moveAllowed) 
                {
                    // On simule le coup
                    Board temp = *board; 
                    Tile *sOld = &temp.tiles[startY][startX];
                    Tile *sNew = &temp.tiles[endY][endX];
                    
                    int sID = TilePop(sOld);
                    
                    // Gérer la capture (simulée)
                    if (sNew->layerCount > 1) 
                    {
                        // On sait que la capture est valide (pas un allié) grâce à IsMoveValid
                        TilePop(sNew);
                    }
                    TilePush(sNew, sID);

                    // Si le Roi est en échec dans la simulation, on interdit le coup
                    if (IsKingInCheck(&temp, currentTurn)) 
                    {
                        TraceLog(LOG_WARNING, "MOUVEMENT INTERDIT : Votre Roi serait en echec !");
                        moveAllowed = false;
                    }
                    // Gérer les cas spéciaux du ROQUE
                    else if (abs(endX - startX) == 2 && endY == startY)
                    {
                        int pieceID = oldTile->layers[oldTile->layerCount - 1];
                        if (pieceID == 10 || pieceID == 11) // C'est un Roi
                        {
                            // On vérifie que le roi ne passe pas par une case attaquée.
                            // La case intermédiaire
                            int midX = startX + (endX > startX ? 1 : -1);
                            
                            // On crée une copie de la simulation
                            Board temp2 = *board; 
                            Tile *sOld2 = &temp2.tiles[startY][startX];
                            Tile *sMid2 = &temp2.tiles[startY][midX];
                            
                            // On déplace le roi sur la case intermédiaire
                            int sID2 = TilePop(sOld2);
                            if (sMid2->layerCount > 1) TilePop(sMid2); // Ne devrait pas arriver pour le roque
                            TilePush(sMid2, sID2);

                            // Si le Roi est en échec sur la case intermédiaire, le roque est interdit
                            if (IsKingInCheck(&temp2, currentTurn))
                            {
                                TraceLog(LOG_WARNING, "MOUVEMENT INTERDIT : Le roi passe par une case en echec (roque) !");
                                moveAllowed = false;
                            }
                        }
                    }
                }

                // 3. Exécution réelle du coup
                if (moveAllowed)
                {
                    int pieceID = oldTile->layers[oldTile->layerCount - 1];
                    
                    // --- GESTION SPÉCIALE : ROQUE ---
                    if ((pieceID == 10 || pieceID == 11) && abs(endX - startX) == 2 && endY == startY)
                    {
                        int rookStartX = (endX == startX + 2) ? startX + 3 : startX - 4; // Colonne 7 ou 0
                        int rookEndX = (endX == startX + 2) ? startX + 1 : startX - 1;   // Colonne 5 ou 3

                        Tile *rookOldTile = &board->tiles[startY][rookStartX];
                        Tile *rookNewTile = &board->tiles[startY][rookEndX];
                        int rookID = TilePop(rookOldTile); // Enlève la Tour
                        TilePush(rookNewTile, rookID);     // Déplace la Tour
                        
                        // Le déplacement du Roi sera géré juste après
                    }
                    
                    // Capture (si ennemi présent)
                    if (clickedTile->layerCount > 1) 
                    {
                        TilePop(clickedTile);
                    }

                    // Déplacement de la pièce sélectionnée (Roi ou autre)
                    TilePop(oldTile); 
                    
                    // --- GESTION SPÉCIALE : PROMOTION ---
                    // Pion blanc arrive en haut (0) OU Pion noir arrive en bas (7)
                    if ((pieceID == 6 && endY == 0) || (pieceID == 7 && endY == 7)) 
                    {
                        TilePush(clickedTile, pieceID);
                        
                        // On déclenche le mode Promotion
                        promotionPending = 1;
                        promotionX = endX; 
                        promotionY = endY;
                        promotionColor = (pieceID == 7) ? 1 : 0;
                        
                        // ATTENTION : On ne change pas le tour tout de suite !
                    }
                    else 
                    {
                        // Cas normal (y compris après roque)
                        TilePush(clickedTile, pieceID);
                        
                        // Changement de tour (si la partie continue)
                        if (board->state != STATE_GAMEOVER) 
                        {
                            currentTurn = 1 - currentTurn;
                        }
                    }

                    // Réinitialisation de la sélection
                    selectedX = -1; 
                    selectedY = -1; 
                    possibleMoveCount = 0; 
                }
                else 
                {
                    // Si le coup est invalide, mais qu'on a cliqué sur une autre pièce à nous
                    // On change simplement la sélection
                    if (clickedTile->layerCount > 1 && GetPieceColor(clickedTile->layers[clickedTile->layerCount-1]) == currentTurn) 
                    {
                        selectedX = -1; 
                        possibleMoveCount = 0;
                        GameLogicUpdate(board, dt); // Appel récursif pour sélectionner immédiatement
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

// ==========================================
// 7. MISE À JOUR PRINCIPALE (UPDATE)
// ==========================================

void GameUpdate(Board *board, float dt)
{
    if (board->state == STATE_PLAYING)
    {
        // 1. Gestion du Temps
        if (currentTurn == 0) {
            if (board->timer.whiteTime > 0.0f) board->timer.whiteTime -= dt; 
        } else {
            if (board->timer.blackTime > 0.0f) board->timer.blackTime -= dt; 
        }

        // 2. Vérification Défaite par Temps
        if (board->timer.whiteTime <= 0.0f) {
            board->state = STATE_GAMEOVER; 
            board->winner = 1; // Noirs gagnent
            return;
        }
        if (board->timer.blackTime <= 0.0f) {
            board->state = STATE_GAMEOVER; 
            board->winner = 0; // Blancs gagnent
            return;
        }
        
        // 3. Option d'Abandon (Forfait)
        if (IsKeyPressed(KEY_F)) {
            board->state = STATE_GAMEOVER; 
            board->winner = 1 - currentTurn; // L'adversaire gagne
            return;
        }

        // 4. DETECTION DE FIN DE PARTIE (ECHEC ET MAT / PAT)
        // On vérifie si le joueur actuel ne peut plus rien faire
        // (Uniquement si aucune promotion n'est en cours)
        if (promotionPending == 0 && !HasLegalMoves(board, currentTurn))
        {
            bool check = IsKingInCheck(board, currentTurn);
            
            board->state = STATE_GAMEOVER;
            
            if (check) 
            {
                // Pas de mouvement + Echec = MAT
                board->winner = 1 - currentTurn; 
                TraceLog(LOG_INFO, "ECHEC ET MAT !");
            } 
            else 
            {
                // Pas de mouvement + Pas Echec = PAT (Nul)
                board->winner = -1; 
                TraceLog(LOG_INFO, "PAT (Match Nul) !");
            }
            return;
        }

        // 5. Mise à jour de la logique de jeu (Souris, etc.)
        GameLogicUpdate(board, dt);
    }
    // Si la partie est terminée
    else if (board->state == STATE_GAMEOVER)
    {
        // Touche R pour recommencer
        if (IsKeyPressed(KEY_R)) 
        {
            GameInit(board);
        }
        
        // Clic sur le bouton "Rejouer" (centré à l'écran)
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
        {
            Vector2 m = GetMousePosition();
            int cW = GetScreenWidth() / 2; 
            int cH = GetScreenHeight() / 2;
            
            if (m.x > cW - 160 && m.x < cW + 160 && m.y > cH - 5 && m.y < cH + 35) 
            {
                GameInit(board);
            }
        }
    }
}

// ==========================================
// 8. DESSIN (DRAW)
// ==========================================

void GameDraw(const Board *board)
{
    int screenW = GetScreenWidth(); 
    int screenH = GetScreenHeight();
    
    // Calcul taille dynamique
    int tileSizeW = screenW / BOARD_COLS;
    int tileSizeH = screenH / BOARD_ROWS;
    int tileSize = (tileSizeW < tileSizeH) ? tileSizeW : tileSizeH;
    
    int offsetX = (screenW - (tileSize * BOARD_COLS)) / 2; 
    int offsetY = (screenH - (tileSize * BOARD_ROWS)) / 2; 

    // --- A. DESSIN DU PLATEAU ET DES PIÈCES ---
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
                    (Rectangle){dX, dY, tileSize, tileSize},
                    (Vector2){0,0}, 0, WHITE
                ); 
            }
            
            // Bordure grise discrète
            DrawRectangleLines(dX, dY, tileSize, tileSize, Fade(DARKGRAY, 0.3f));
        }
    }

    // --- B. INDICATEUR VISUEL D'ECHEC (Carré Rouge sous le Roi) ---
    if (board->state == STATE_PLAYING && IsKingInCheck(board, currentTurn)) 
    {
        int kingID = (currentTurn == 0) ? 10 : 11;
        
        // On cherche le roi pour le colorier
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

    // --- C. DESSIN DES COUPS POSSIBLES (Aide visuelle) ---
    for (int i = 0; i < possibleMoveCount; i++) 
    {
        int x = possibleMoves[i][0]; 
        int y = possibleMoves[i][1];
        int dX = offsetX + x * tileSize; 
        int dY = offsetY + y * tileSize;
        
        // Si c'est un ennemi -> Carré rouge
        if (board->tiles[y][x].layerCount > 1) 
        {
            DrawRectangleLinesEx((Rectangle){dX, dY, tileSize, tileSize}, 5, Fade(RED, 0.6f));
        }
        // Si c'est vide -> Rond gris
        else 
        {
            DrawCircle(dX + tileSize/2, dY + tileSize/2, tileSize/8, Fade(DARKGRAY, 0.5f));
        }
    }

    // --- D. DESSIN DE LA SÉLECTION (Cadre Vert) ---
    if (selectedX != -1) 
    {
        DrawRectangleLinesEx(
            (Rectangle){offsetX + selectedX * tileSize, offsetY + selectedY * tileSize, tileSize, tileSize}, 
            4, 
            GREEN
        ); 
    }

    // --- E. DESSIN DES TIMERS ---
    int centerTextY = offsetY + (tileSize * BOARD_ROWS) / 2 - 15;
    Color wC = (currentTurn == 0) ? RAYWHITE : DARKGRAY; // Blanc brille si c'est son tour
    Color bC = (currentTurn == 1) ? RAYWHITE : DARKGRAY; // Noir brille si c'est son tour
    
    DrawText(TextFormat("BLANCS\n%02d:%02d", (int)board->timer.whiteTime/60, (int)board->timer.whiteTime%60), 
             offsetX - 150, centerTextY, 30, wC); 
             
    DrawText(TextFormat("NOIRS\n%02d:%02d", (int)board->timer.blackTime/60, (int)board->timer.blackTime%60), 
             offsetX + tileSize * BOARD_COLS + 20, centerTextY, 30, bC); 

    // --- F. MENU DE PROMOTION (Superposé) ---
    if (promotionPending == 1) 
    {
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.85f));
        
        const char *title = "PROMOTION !";
        const char *opts = "1: REINE   2: CAVALIER   3: TOUR   4: FOU";
        
        DrawText(title, screenW/2 - MeasureText(title, 40)/2, screenH/2 - 100, 40, YELLOW);
        DrawText(opts, screenW/2 - MeasureText(opts, 20)/2, screenH/2, 20, WHITE);
    }

    // --- G. ECRAN DE FIN DE PARTIE (Game Over) ---
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
        DrawText("CLIQUEZ POUR REJOUER", screenW/2 - MeasureText("CLIQUEZ POUR REJOUER", 30)/2, screenH/2, 30, YELLOW);
        DrawRectangleLines(screenW/2 - 160, screenH/2 - 5, 320, 40, YELLOW);
        
        DrawText("Appuyer sur ECHAP pour quitter", screenW/2 - MeasureText("Appuyer sur ECHAP pour quitter", 20)/2, screenH/2 + 100, 20, RAYWHITE);
    }
}