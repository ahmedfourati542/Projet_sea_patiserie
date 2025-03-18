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
Ingredient *ingredients; // Définition ici
int shmid_ingredients;   // Définition ici

// --- Prototypes de fonctions (extern pour les autres modes et main) ---
extern void afficher_gateaux();
extern void afficher_stock();
extern int verifier_stock(Ingredient *stock_ptr, int index);
extern void mise_a_jour_stock(Ingredient *stock_ptr, int index);

// --- Mode Multiprocessus ---
void commander_gateaux_multiprocess() {
    int pipefd[2], choix[MAX_CHOIX], nb_choix = 0;
    char input[100];
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

    int sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } init;
    init.val = 1;
    semctl(sem_id, 0, SETVAL, init);

    shmid_ingredients = shmget(IPC_PRIVATE, sizeof(Ingredient) * MAX_INGREDIENTS, 0666 | IPC_CREAT);
    ingredients = (Ingredient *)shmat(shmid_ingredients, NULL, 0);
    Ingredient initial_ingredients[MAX_INGREDIENTS] = {
        {"Farine", 25000}, {"Sucre", 25000}, {"Beurre", 25000},
        {"Œufs", 25000}, {"Chocolat", 25000}
    };
    memcpy(ingredients, initial_ingredients, sizeof(initial_ingredients));

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
        shmdt(ingredients);
        shmctl(shmid_ingredients, IPC_RMID, 0);
        semctl(sem_id, 0, IPC_RMID);
        return;
    }

    pipe(pipefd);
    debut_mode = time(NULL);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (fork() == 0) {
            close(pipefd[1]);
            int index;
            while (read(pipefd[0], &index, sizeof(int)) > 0) {
                struct sembuf op = {0, -1, 0};
                semop(sem_id, &op, 1);

                mise_a_jour_stock(ingredients, index);
                op.sem_op = 1;
                semop(sem_id, &op, 1);

                printf("[%d] ✅ Début de : %s\n", getpid(), gateaux[index].nom);
                sleep(gateaux[index].temps_preparation);
                printf("[%d] 🎂 %s prêt!\n", getpid(), gateaux[index].nom);
            }
            close(pipefd[0]);
            exit(0);
        }
    }

    close(pipefd[0]);
    for (int i = 0; i < nb_choix; i++) {
        write(pipefd[1], &choix[i], sizeof(int));
    }
    close(pipefd[1]);

    while (wait(NULL) > 0);
    temps_total_mode = difftime(time(NULL), debut_mode);
    printf("\n⏱️ Temps total d'exécution (Multiprocessus) : %.2f secondes\n", temps_total_mode);

    afficher_stock();

    shmdt(ingredients);
    shmctl(shmid_ingredients, IPC_RMID, 0);
    semctl(sem_id, 0, IPC_RMID);
}
