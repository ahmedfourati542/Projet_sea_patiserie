#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

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

// --- Variables globales pour le multithreading ---
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t stock_mutex = PTHREAD_MUTEX_INITIALIZER;
int cake_queue[MAX_CHOIX];
int queue_head = 0;
int queue_tail = 0;
int tasks_remaining = 0;

// --- Prototypes de fonctions (extern pour les autres modes et main) ---
extern void afficher_gateaux();
extern void afficher_stock();
extern int verifier_stock(Ingredient *stock_ptr, int index);
extern void mise_a_jour_stock(Ingredient *stock_ptr, int index);

// --- Mode Multithread ---
void* preparer_gateau_thread(void* arg) {
    while (1) {
        int index;

        // Consommer une commande de la file d'attente
        pthread_mutex_lock(&queue_mutex);
        while (queue_head == queue_tail) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        index = cake_queue[queue_head % MAX_CHOIX];
        queue_head++;
        pthread_mutex_unlock(&queue_mutex);

        // Vérifier si c'est un signal de terminaison
        if (index == -1) {
            break; // Terminer le thread
        }

        // Traiter la commande
        pthread_mutex_lock(&stock_mutex);
        if (verifier_stock(ingredients, index)) {
            mise_a_jour_stock(ingredients, index);
            pthread_mutex_unlock(&stock_mutex);

            printf("[Thread %lu] ➡️ Début du traitement de : %s\n", pthread_self(), gateaux[index].nom);
            sleep(gateaux[index].temps_preparation);
            printf("[Thread %lu] 🎂 %s prêt!\n", pthread_self(), gateaux[index].nom);
        } else {
            pthread_mutex_unlock(&stock_mutex);
            printf("[Thread %lu] ❌ Stock insuffisant pour : %s\n", pthread_self(), gateaux[index].nom);
        }

        // Mettre à jour le nombre de tâches restantes
        pthread_mutex_lock(&queue_mutex);
        tasks_remaining--;
        if (tasks_remaining == 0) {
            pthread_cond_signal(&queue_cond); // Réveiller le thread principal
        }
        pthread_mutex_unlock(&queue_mutex);
    }
    return NULL;
}

void commander_gateaux_multithread() {
    char input[100];
    int choix[MAX_CHOIX], nb_choix = 0;
    time_t debut_mode;
    pthread_t threads[MAX_THREADS];

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

    // Vérifier le stock global
    pthread_mutex_lock(&stock_mutex);
    int total_ingredients[MAX_INGREDIENTS] = {0};
    for (int i = 0; i < nb_choix; i++) {
        for (int j = 0; j < MAX_INGREDIENTS; j++) {
            total_ingredients[j] += gateaux[choix[i]].ingredients[j];
        }
    }

    int can_proceed = 1;
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        if (total_ingredients[i] > ingredients[i].stock) {
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

    // Démarrer les threads
    debut_mode = time(NULL);
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, preparer_gateau_thread, NULL);
    }

    // Réveiller les threads
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
        cake_queue[queue_tail % MAX_CHOIX] = -1;
        queue_tail++;
    }
    pthread_mutex_unlock(&queue_mutex);
    pthread_cond_broadcast(&queue_cond);

    // Attendre que tous les threads terminent
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Afficher le temps total
    double temps_total_mode = difftime(time(NULL), debut_mode);
    printf("\n⏱️ Temps total d'exécution (Multithread) : %.2f secondes\n", temps_total_mode);

    // Afficher le stock final
    afficher_stock();
}
