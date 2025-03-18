#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Définitions ---
#define MAX_GATEAUX 20
#define MAX_INGREDIENTS 5
#define MAX_CHOIX 10
#define MAX_THREADS 5
#define MAX_PROCESSES 4

// --- Structures ---
typedef struct {
    char nom[30];
    int ingredients[MAX_INGREDIENTS];
    int temps_preparation;
} Gateau;

typedef struct {
    char nom[20];
    int stock;
} Ingredient;

// --- Variables globales ---
Gateau gateaux[MAX_GATEAUX] = {
    {"Tarte aux Fraises", {200, 100, 150, 50, 0}, 10},
    {"Mille-Feuille", {300, 200, 200, 100, 0}, 15},
    {"Éclair au Chocolat", {100, 150, 100, 50, 200}, 8},
    {"Opéra", {250, 200, 200, 150, 300}, 20},
    {"Tiramisu", {200, 250, 150, 100, 0}, 12},
    {"Brownie", {300, 200, 250, 100, 400}, 14},
    {"Chausson aux Pommes", {200, 150, 150, 50, 0}, 10},
    {"Macaron", {150, 200, 100, 50, 300}, 18},
    {"Galette des Rois", {400, 250, 200, 150, 0}, 22},
    {"Forêt-Noire", {500, 300, 250, 200, 500}, 25},
    {"Paris-Brest", {300, 200, 250, 100, 200}, 15},
    {"Clafoutis", {250, 200, 150, 50, 0}, 12},
    {"Gâteau Basque", {350, 250, 200, 100, 0}, 18},
    {"Moelleux au Chocolat", {200, 200, 250, 100, 500}, 14},
    {"Pithiviers", {300, 250, 200, 150, 0}, 20},
    {"Madeleine", {150, 150, 100, 50, 0}, 8},
    {"Pain d'épices", {400, 300, 250, 100, 0}, 16},
    {"Financier", {250, 200, 150, 50, 300}, 10},
    {"Baba au Rhum", {350, 250, 200, 100, 0}, 22},
    {"Sablé Breton", {200, 150, 150, 50, 0}, 9}
};

extern Ingredient *ingredients;
extern int shmid_ingredients;

// --- Prototypes de fonctions (définies dans d'autres fichiers) ---
extern void commander_gateaux_mono();
extern void commander_gateaux_multiprocess();
extern void commander_gateaux_multithread();
extern void afficher_gateaux();
extern void afficher_stock();
extern int verifier_stock(Ingredient *stock_ptr, int index);
extern void mise_a_jour_stock(Ingredient *stock_ptr, int index);

int main() {
    int mode;
    char continuer = 'o';

    printf("=== 🏪 Bienvenue dans la Pâtisserie ===\n");
    printf("🔧 Choisissez votre mode de préparation (1 pour Mono, 2 pour Multiprocessus, 3 pour Multithread) : ");
    scanf("%d", &mode);
    while (getchar() != '\n');

    Ingredient initial_ingredients[MAX_INGREDIENTS] = {
        {"Farine", 25000}, {"Sucre", 25000}, {"Beurre", 25000},
        {"Œufs", 25000}, {"Chocolat", 25000}
    };

    if (mode != 2) {
        ingredients = (Ingredient *)malloc(sizeof(Ingredient) * MAX_INGREDIENTS);
        memcpy(ingredients, initial_ingredients, sizeof(initial_ingredients));
    }

    while (continuer == 'o' || continuer == 'O') {
        switch (mode) {
            case 1:
                commander_gateaux_mono();
                break;
            case 2:
                commander_gateaux_multiprocess();
                break;
            case 3:
                commander_gateaux_multithread();
                break;
            default:
                printf("Mode invalide.\n");
                break;
        }

        printf("\nVoulez-vous commander un autre gâteau ? (o/n) : ");
        scanf(" %c", &continuer);
    }

    printf("👋 Merci pour votre visite ! À bientôt !\n");

    if (mode != 2 && ingredients != NULL) {
        free(ingredients);
    }

    return 0;
}
