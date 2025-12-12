# 1. Le compilateur Cross-Platform (Windows sur Mac)
CC = x86_64-w64-mingw32-gcc

# 2. Chemins vers la Raylib WINDOWS (celle que tu as téléchargée)
RAYLIB_PATH = ./raylib_win

# 3. Options de compilation (Include headers)
# On inclut le dossier src, le dossier include local, et les headers de raylib_win
CFLAGS = -Wall -Wextra -Isrc -Iinclude -I$(RAYLIB_PATH)/include

# 4. Options de Link (Bibliothèques)
# -L... : Où trouver le fichier .a de la lib
# -lraylib : La lib elle-même
# -lopengl32 -lgdi32 -lwinmm : Les libs Windows OBLIGATOIRES pour Raylib
# -static : Pour tout inclure dans le .exe
LDFLAGS = -L$(RAYLIB_PATH)/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static

# 5. Nom de l'exécutable
EXEC = mon_jeu.exe

# 6. Sources : On prend tous les .c dans le dossier src/
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# Règle pour compiler les .c en .o
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f src/*.o $(EXEC)