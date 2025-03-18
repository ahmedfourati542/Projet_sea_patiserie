#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// --- Définitions (extern pour correspondre à main.c) ---
#define MAX_GATEAUX 20
#define MAX_INGREDIENTS 5
#define MAX_CHOIX 10
#define MAX_THREADS 5
#define MAX_PROCESSES 4

// --- Structures (extern pour correspondre à main.c) ---
typedef struct {
    char nom[30];
    int ingredients[MAX_INGREDIENTS];
    int temps_preparation;
} Gateau;

typedef struct {
    char nom[20];
    int stock;
} Ingredient;

// --- Variables globales (extern pour correspondre à main.c) ---
extern Gateau gateaux[MAX_GATEAUX];
extern Ingredient *ingredients;

// --- Fonctions utilitaires ---
void afficher_gateaux() {
    printf("\n🍰 Gâteaux disponibles :\n");
    for (int i = 0; i < MAX_GATEAUX; i++)
        printf("%d. %s (Temps: %d secondes)\n", i + 1, gateaux[i].nom, gateaux[i].temps_preparation);
}

void afficher_stock() {
    printf("\n📦 Stock des ingrédients après la commande :\n");
    for (int i = 0; i < MAX_INGREDIENTS; i++)
        printf("- %s : %d unités\n", ingredients[i].nom, ingredients[i].stock);
}

int verifier_stock(Ingredient *stock_ptr, int index) {
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        if (gateaux[index].ingredients[i] > stock_ptr[i].stock) {
            return 0;
        }
    }
    return 1;
}

void mise_a_jour_stock(Ingredient *stock_ptr, int index) {
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        stock_ptr[i].stock -= gateaux[index].ingredients[i];
    }
}

// --- Mode Mono ---
void commander_gateaux_mono() {
    char input[100];
    int choix[MAX_CHOIX];
    int nb_choix = 0;
    time_t debut_mode;
    double temps_total_mode;

    afficher_gateaux();
    printf("\n🔹 Entrez les numéros des gâteaux (séparés par des espaces) : ");
    fgets(input, sizeof(input), stdin);

    char *token = strtok(input, " ");
    while (token != NULL && nb_choix < MAX_CHOIX) {
        int num = atoi(token);
        if (num >= 1 && num <= MAX_GATEAUX) {
            choix[nb_choix++] = num - 1;
        }
        token = strtok(NULL, " ");
    }

    if (nb_choix == 0) {
        printf("⚠️ Aucun gâteau sélectionné.\n");
        return;
    }

    printf("\n⏳ Temps de préparation individuel des gâteaux commandés :\n");
    for (int i = 0; i < nb_choix; i++) {
        int index = choix[i];
        printf("- %s : %d secondes\n", gateaux[index].nom, gateaux[index].temps_preparation);
    }

    debut_mode = time(NULL);

    for (int i = 0; i < nb_choix; i++) {
        int index = choix[i];
        if (!verifier_stock(ingredients, index)) {
            printf("⚠️ Commande annulée pour %s à cause du manque d'ingrédients.\n", gateaux[index].nom);
            continue;
        }
        mise_a_jour_stock(ingredients, index);
        printf("[%d] ✅ Préparation de votre %s...\n", getpid(), gateaux[index].nom);
        sleep(gateaux[index].temps_preparation);
        printf("[%d] 🎂 %s est prêt !\n", getpid(), gateaux[index].nom);
    }

    temps_total_mode = difftime(time(NULL), debut_mode);
    printf("\n⏱️ Temps total d'exécution (Mono) : %.2f secondes\n", temps_total_mode);

    afficher_stock();
}
