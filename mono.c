#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// --- Définitions des constantes ---
#define MAX_GATEAUX 20         // Nombre maximum de gâteaux disponibles
#define MAX_INGREDIENTS 5      // Nombre maximum d'ingrédients par gâteau
#define MAX_CHOIX 10           // Nombre maximum de gâteaux pouvant être choisis dans une commande
#define MAX_THREADS 5          // Nombre de threads utilisés (non utilisé dans cette version)
#define MAX_PROCESSES 4        // Nombre de processus utilisés (non utilisé dans cette version)

// --- Structures pour les gâteaux et les ingrédients ---
typedef struct {
    char nom[30];             // Nom du gâteau
    int ingredients[MAX_INGREDIENTS];  // Quantité d'ingrédients nécessaires pour chaque gâteau
    int temps_preparation;    // Temps de préparation du gâteau en secondes
} Gateau;

typedef struct {
    char nom[20];             // Nom de l'ingrédient
    int stock;                // Quantité d'unité disponible en stock
} Ingredient;

// --- Variables globales externes ---
extern Gateau gateaux[MAX_GATEAUX];  // Tableau des gâteaux disponibles
extern Ingredient *ingredients;      // Tableau des ingrédients disponibles

// --- Fonctions utilitaires ---
/**
 * Affiche la liste des gâteaux disponibles.
 */
void afficher_gateaux() {
    printf("\n🍰 Gâteaux disponibles :\n");
    for (int i = 0; i < MAX_GATEAUX; i++) {
        // Affiche chaque gâteau avec son temps de préparation
        printf("%d. %s (Temps: %d secondes)\n", i + 1, gateaux[i].nom, gateaux[i].temps_preparation);
    }
}

/**
 * Affiche le stock d'ingrédients après une commande.
 */
void afficher_stock() {
    printf("\n📦 Stock des ingrédients après la commande :\n");
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        // Affiche l'état du stock pour chaque ingrédient
        printf("- %s : %d unités\n", ingredients[i].nom, ingredients[i].stock);
    }
}

/**
 * Vérifie si le stock d'ingrédients est suffisant pour préparer un gâteau.
 * Retourne 1 si les ingrédients sont suffisants, sinon 0.
 */
int verifier_stock(Ingredient *stock_ptr, int index) {
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        if (gateaux[index].ingredients[i] > stock_ptr[i].stock) {
            return 0;  // Stock insuffisant pour l'ingrédient
        }
    }
    return 1;  // Stock suffisant pour tous les ingrédients
}

/**
 * Met à jour le stock des ingrédients après la préparation d'un gâteau.
 */
void mise_a_jour_stock(Ingredient *stock_ptr, int index) {
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        stock_ptr[i].stock -= gateaux[index].ingredients[i];  // Réduit le stock des ingrédients utilisés
    }
}

// --- Mode Mono (Séquentiel) pour commander des gâteaux ---
void commander_gateaux_mono() {
    char input[100];            // Stocke l'entrée de l'utilisateur (numéros des gâteaux)
    int choix[MAX_CHOIX];       // Tableau des gâteaux sélectionnés par l'utilisateur
    int nb_choix = 0;           // Compteur pour le nombre de gâteaux choisis
    time_t debut_mode;          // Variable pour mesurer le temps d'exécution
    double temps_total_mode;    // Temps total d'exécution

    // Affiche les gâteaux disponibles
    afficher_gateaux();
    
    // Demande à l'utilisateur de sélectionner les gâteaux
    printf("\n🔹 Entrez les numéros des gâteaux (séparés par des espaces) : ");
    fgets(input, sizeof(input), stdin);  // Récupère l'entrée de l'utilisateur

    // Sépare l'entrée en différents numéros et les valide
    char *token = strtok(input, " ");
    while (token != NULL && nb_choix < MAX_CHOIX) {
        int num = atoi(token);
        if (num >= 1 && num <= MAX_GATEAUX) {
            choix[nb_choix++] = num - 1;  // Sauvegarde l'indice du gâteau choisi (num-1 pour indexation à partir de 0)
        }
        token = strtok(NULL, " ");
    }

    // Si aucun gâteau n'a été sélectionné, retourne un message d'erreur
    if (nb_choix == 0) {
        printf("⚠️ Aucun gâteau sélectionné.\n");
        return;
    }

    // Affiche le temps de préparation des gâteaux sélectionnés
    printf("\n⏳ Temps de préparation individuel des gâteaux commandés :\n");
    for (int i = 0; i < nb_choix; i++) {
        int index = choix[i];
        printf("- %s : %d secondes\n", gateaux[index].nom, gateaux[index].temps_preparation);
    }

    // Démarre le chronomètre pour mesurer le temps total d'exécution
    debut_mode = time(NULL);

    // Pour chaque gâteau sélectionné
    for (int i = 0; i < nb_choix; i++) {
        int index = choix[i];

        // Vérifie si le stock d'ingrédients est suffisant pour préparer le gâteau
        if (!verifier_stock(ingredients, index)) {
            printf("⚠️ Commande annulée pour %s à cause du manque d'ingrédients.\n", gateaux[index].nom);
            continue;  // Passe au gâteau suivant si les ingrédients sont insuffisants
        }

        // Met à jour le stock après la préparation du gâteau
        mise_a_jour_stock(ingredients, index);

        // Affiche le début de la préparation du gâteau
        printf("[%d] ✅ Préparation de votre %s...\n", getpid(), gateaux[index].nom);
        sleep(gateaux[index].temps_preparation);  // Simule le temps de préparation du gâteau
        printf("[%d] 🎂 %s est prêt !\n", getpid(), gateaux[index].nom);
    }

    // Mesure et affiche le temps total d'exécution
    temps_total_mode = difftime(time(NULL), debut_mode);
    printf("\n⏱️ Temps total d'exécution (Mono) : %.2f secondes\n", temps_total_mode);

    // Affiche l'état final du stock d'ingrédients
    afficher_stock();
}
