#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

// --- Définitions (extern pour correspondre à main.c) ---
#define MAX_GATEAUX 20      // Nombre maximal de gâteaux
#define MAX_INGREDIENTS 5   // Nombre d'ingrédients disponibles
#define MAX_CHOIX 10        // Nombre maximal de gâteaux que l'utilisateur peut sélectionner
#define MAX_THREADS 5       // Nombre maximal de threads (défini ailleurs dans le code pour d'autres modes)
#define MAX_PROCESSES 4     // Nombre maximal de processus pour la gestion des gâteaux

// --- Structures (extern pour correspondre à main.c) ---
typedef struct {
    char nom[30];           // Nom du gâteau
    int ingredients[MAX_INGREDIENTS];  // Quantité d'ingrédients nécessaires pour ce gâteau
    int temps_preparation;  // Temps nécessaire pour préparer ce gâteau
} Gateau;

typedef struct {
    char nom[20];   // Nom de l'ingrédient
    int stock;      // Quantité disponible de cet ingrédient
} Ingredient;

// --- Variables globales (extern pour correspondre à main.c) ---
extern Gateau gateaux[MAX_GATEAUX];   // Tableau de gâteaux
Ingredient *ingredients;               // Pointeur vers la mémoire partagée des ingrédients
int shmid_ingredients;                 // Identifiant de la mémoire partagée pour les ingrédients

// --- Prototypes de fonctions (extern pour les autres modes et main) ---
extern void afficher_gateaux();         // Fonction pour afficher les gâteaux disponibles
extern void afficher_stock();           // Fonction pour afficher le stock d'ingrédients
extern int verifier_stock(Ingredient *stock_ptr, int index);  // Fonction pour vérifier si un gâteau peut être fabriqué
extern void mise_a_jour_stock(Ingredient *stock_ptr, int index);  // Fonction pour mettre à jour le stock d'ingrédients

// --- Mode Multiprocessus ---
void commander_gateaux_multiprocess() {
    int pipefd[2], choix[MAX_CHOIX], nb_choix = 0;
    char input[100];
    time_t debut_mode;          // Variable pour stocker le temps de début de l'exécution
    double temps_total_mode;    // Variable pour stocker le temps total d'exécution

    // Afficher les gâteaux disponibles
    afficher_gateaux();
    printf("\n🔹 Entrez les numéros des gâteaux (séparés par des espaces) : ");
    fgets(input, sizeof(input), stdin);  // Lecture de l'entrée utilisateur

    // Découper l'entrée en numéros de gâteaux
    char *token = strtok(input, " ");
    while (token != NULL && nb_choix < MAX_CHOIX) {
        int num = atoi(token);   // Convertir chaque numéro de gâteau en entier
        if (num >= 1 && num <= MAX_GATEAUX) {
            choix[nb_choix++] = num - 1;  // Ajouter l'indice du gâteau sélectionné
        }
        token = strtok(NULL, " ");
    }

    // Vérifier que des gâteaux ont bien été choisis
    if (nb_choix == 0) {
        printf("⚠️ Aucun gâteau sélectionné.\n");
        return;
    }

    // Créer un sémaphore pour la synchronisation des processus
    int sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } init;
    init.val = 1;  // Initialiser le sémaphore à 1 (accès exclusif)
    semctl(sem_id, 0, SETVAL, init);

    // Créer une mémoire partagée pour stocker les ingrédients
    shmid_ingredients = shmget(IPC_PRIVATE, sizeof(Ingredient) * MAX_INGREDIENTS, 0666 | IPC_CREAT);
    ingredients = (Ingredient *)shmat(shmid_ingredients, NULL, 0);
    Ingredient initial_ingredients[MAX_INGREDIENTS] = {
        {"Farine", 25000}, {"Sucre", 25000}, {"Beurre", 25000},
        {"Œufs", 25000}, {"Chocolat", 25000}
    };
    memcpy(ingredients, initial_ingredients, sizeof(initial_ingredients));  // Initialiser le stock des ingrédients

    // Vérifier si le stock est suffisant pour la commande
    int total_ingredients[MAX_INGREDIENTS] = {0};
    for (int i = 0; i < nb_choix; i++) {
        for (int j = 0; j < MAX_INGREDIENTS; j++) {
            total_ingredients[j] += gateaux[choix[i]].ingredients[j];  // Additionner les ingrédients nécessaires
        }
    }

    // Vérification du stock
    int can_proceed = 1;
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        if (total_ingredients[i] > ingredients[i].stock) {
            can_proceed = 0;  // Si un ingrédient est insuffisant, ne pas procéder
            break;
        }
    }

    if (!can_proceed) {
        printf("❌ Stock insuffisant pour la commande.\n");
        shmdt(ingredients);          // Détacher la mémoire partagée
        shmctl(shmid_ingredients, IPC_RMID, 0);  // Supprimer la mémoire partagée
        semctl(sem_id, 0, IPC_RMID);  // Supprimer le sémaphore
        return;
    }

    // Créer un pipe pour la communication entre les processus
    pipe(pipefd);
    debut_mode = time(NULL);  // Enregistrer l'heure de début

    // Créer les processus pour préparer les gâteaux
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (fork() == 0) {  // Si c'est un processus enfant
            close(pipefd[1]);  // Fermer le côté écriture du pipe
            int index;
            while (read(pipefd[0], &index, sizeof(int)) > 0) {  // Lire les indices des gâteaux à préparer
                struct sembuf op = {0, -1, 0};  // Prendre le sémaphore
                semop(sem_id, &op, 1);

                mise_a_jour_stock(ingredients, index);  // Mettre à jour le stock d'ingrédients
                op.sem_op = 1;  // Libérer le sémaphore
                semop(sem_id, &op, 1);

                // Préparer le gâteau
                printf("[%d] ✅ Début de : %s\n", getpid(), gateaux[index].nom);
                sleep(gateaux[index].temps_preparation);  // Simuler le temps de préparation
                printf("[%d] 🎂 %s prêt!\n", getpid(), gateaux[index].nom);
            }
            close(pipefd[0]);  // Fermer le côté lecture du pipe
            exit(0);  // Fin du processus
        }
    }

    // Envoyer les indices des gâteaux à préparer aux processus enfants
    close(pipefd[0]);
    for (int i = 0; i < nb_choix; i++) {
        write(pipefd[1], &choix[i], sizeof(int));
    }
    close(pipefd[1]);  // Fermer le pipe

    // Attendre que tous les processus se terminent
    while (wait(NULL) > 0);

    // Calculer et afficher le temps total d'exécution
    temps_total_mode = difftime(time(NULL), debut_mode);
    printf("\n⏱️ Temps total d'exécution (Multiprocessus) : %.2f secondes\n", temps_total_mode);

    afficher_stock();  // Afficher le stock final des ingrédients

    // Libérer les ressources
    shmdt(ingredients);          // Détacher la mémoire partagée
    shmctl(shmid_ingredients, IPC_RMID, 0);  // Supprimer la mémoire partagée
    semctl(sem_id, 0, IPC_RMID);  // Supprimer le sémaphore
}
