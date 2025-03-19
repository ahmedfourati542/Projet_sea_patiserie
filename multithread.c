#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// --- Définitions des constantes ---
#define MAX_GATEAUX 20         // Nombre maximum de gâteaux
#define MAX_INGREDIENTS 5      // Nombre maximum d'ingrédients pour chaque gâteau
#define MAX_CHOIX 10           // Nombre maximum de gâteaux pouvant être choisis
#define MAX_THREADS 5          // Nombre de threads pouvant être créés pour traiter les commandes
#define MAX_PROCESSES 4        // (non utilisé dans ce fichier, mais probablement pour un mode multiprocessus)

// --- Structures des gâteaux et des ingrédients ---
typedef struct {
    char nom[30];             // Nom du gâteau
    int ingredients[MAX_INGREDIENTS];  // Quantités d'ingrédients nécessaires pour préparer le gâteau
    int temps_preparation;    // Temps de préparation du gâteau
} Gateau;

typedef struct {
    char nom[20];             // Nom de l'ingrédient
    int stock;                // Quantité en stock de l'ingrédient
} Ingredient;

// --- Variables globales externes ---
extern Gateau gateaux[MAX_GATEAUX];  // Liste des gâteaux disponibles
extern Ingredient *ingredients;      // Liste des ingrédients disponibles

// --- Variables globales pour le multithreading ---
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex pour protéger l'accès à la file de commandes
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;   // Condition variable pour gérer la synchronisation des threads
pthread_mutex_t stock_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex pour protéger l'accès au stock d'ingrédients
int cake_queue[MAX_CHOIX];         // File d'attente des gâteaux commandés
int queue_head = 0;                // Index du début de la file
int queue_tail = 0;                // Index de la fin de la file
int tasks_remaining = 0;           // Nombre de tâches restantes à traiter

// --- Prototypes de fonctions externes ---
extern void afficher_gateaux();  // Affiche la liste des gâteaux
extern void afficher_stock();    // Affiche le stock des ingrédients
extern int verifier_stock(Ingredient *stock_ptr, int index);  // Vérifie si les ingrédients sont suffisants
extern void mise_a_jour_stock(Ingredient *stock_ptr, int index); // Met à jour le stock d'ingrédients

// --- Fonction exécutée par chaque thread pour préparer un gâteau ---
void* preparer_gateau_thread(void* arg) {
    while (1) {
        int index;

        // Consommer une commande de la file d'attente (attente si vide)
        pthread_mutex_lock(&queue_mutex);
        while (queue_head == queue_tail) {
            pthread_cond_wait(&queue_cond, &queue_mutex);  // Attente de nouvelles commandes
        }
        index = cake_queue[queue_head % MAX_CHOIX];
        queue_head++;
        pthread_mutex_unlock(&queue_mutex);

        // Vérifier si c'est un signal de terminaison
        if (index == -1) {
            break;  // Terminer le thread
        }

        // Traiter la commande
        pthread_mutex_lock(&stock_mutex);
        if (verifier_stock(ingredients, index)) {  // Si les ingrédients sont suffisants
            mise_a_jour_stock(ingredients, index);  // Mettre à jour le stock
            pthread_mutex_unlock(&stock_mutex);

            // Simuler la préparation du gâteau
            printf("[Thread %lu] ➡️ Début du traitement de : %s\n", pthread_self(), gateaux[index].nom);
            sleep(gateaux[index].temps_preparation);  // Simuler le temps de préparation
            printf("[Thread %lu] 🎂 %s prêt!\n", pthread_self(), gateaux[index].nom);
        } else {
            pthread_mutex_unlock(&stock_mutex);  // Déverrouiller le stock si insuffisant
            printf("[Thread %lu] ❌ Stock insuffisant pour : %s\n", pthread_self(), gateaux[index].nom);
        }

        // Mettre à jour le nombre de tâches restantes
        pthread_mutex_lock(&queue_mutex);
        tasks_remaining--;
        if (tasks_remaining == 0) {
            pthread_cond_signal(&queue_cond);  // Réveiller le thread principal si toutes les tâches sont terminées
        }
        pthread_mutex_unlock(&queue_mutex);
    }
    return NULL;
}

// --- Fonction principale pour commander les gâteaux en utilisant des threads ---
void commander_gateaux_multithread() {
    char input[100];
    int choix[MAX_CHOIX], nb_choix = 0;
    time_t debut_mode;
    pthread_t threads[MAX_THREADS];  // Tableau pour les threads

    afficher_gateaux();  // Affiche la liste des gâteaux
    printf("\n🔹 Entrez les numéros des gâteaux (séparés par des espaces) : ");
    fgets(input, sizeof(input), stdin);  // Récupérer les choix de l'utilisateur

    // Parser l'entrée de l'utilisateur et vérifier les choix
    char *token = strtok(input, " ");
    while (token != NULL && nb_choix < MAX_CHOIX) {
        int num = atoi(token);
        if (num >= 1 && num <= MAX_GATEAUX) {
            choix[nb_choix++] = num - 1;  // Stocker l'indice du gâteau choisi
        }
        token = strtok(NULL, " ");
    }

    if (nb_choix == 0) {
        printf("⚠️ Aucun gâteau sélectionné.\n");
        return;
    }

    // Vérifier le stock global avant de procéder à la commande
    pthread_mutex_lock(&stock_mutex);
    int total_ingredients[MAX_INGREDIENTS] = {0};
    for (int i = 0; i < nb_choix; i++) {
        for (int j = 0; j < MAX_INGREDIENTS; j++) {
            total_ingredients[j] += gateaux[choix[i]].ingredients[j];
        }
    }

    int can_proceed = 1;
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        if (total_ingredients[i] > ingredients[i].stock) {  // Vérifier si le stock est suffisant
            can_proceed = 0;
            break;
        }
    }

    if (!can_proceed) {
        printf("❌ Stock insuffisant pour la commande.\n");
        pthread_mutex_unlock(&stock_mutex);
        return;
    }
    pthread_mutex_unlock(&stock_mutex);

    // Ajouter les commandes à la file d'attente
    pthread_mutex_lock(&queue_mutex);
    for (int i = 0; i < nb_choix; i++) {
        cake_queue[queue_tail % MAX_CHOIX] = choix[i];
        queue_tail++;
    }
    tasks_remaining = nb_choix;
    pthread_mutex_unlock(&queue_mutex);

    // Démarrer les threads pour traiter les commandes
    debut_mode = time(NULL);
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, preparer_gateau_thread, NULL);
    }

    // Réveiller les threads en envoyant un signal de condition
    pthread_cond_broadcast(&queue_cond);

    // Attendre que toutes les tâches soient terminées
    pthread_mutex_lock(&queue_mutex);
    while (tasks_remaining > 0) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);

    // Envoyer un signal de terminaison aux threads
    pthread_mutex_lock(&queue_mutex);
    for (int i = 0; i < MAX_THREADS; i++) {
        cake_queue[queue_tail % MAX_CHOIX] = -1;  // Signaler la fin des tâches
        queue_tail++;
    }
    pthread_mutex_unlock(&queue_mutex);
    pthread_cond_broadcast(&queue_cond);

    // Attendre que tous les threads terminent
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Afficher le temps total d'exécution
    double temps_total_mode = difftime(time(NULL), debut_mode);
    printf("\n⏱️ Temps total d'exécution (Multithread) : %.2f secondes\n", temps_total_mode);

    // Afficher le stock final après les commandes
    afficher_stock();
}
