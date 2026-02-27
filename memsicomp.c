/*
 * memsicomp.c - Memoria compartida con alternancia estricta
 *
 * Este programa demuestra el uso de memoria compartida entre hilos
 * utilizando mmap() en lugar de variables globales. Se reserva una
 * página de memoria con MAP_SHARED | MAP_ANONYMOUS donde se almacena
 * una estructura con un acumulador (suma) y una variable de turno.
 *
 * Dos hilos se crean con pthreads y alternan su ejecución mediante
 * espera activa (busy waiting) sobre la variable "turno":
 *   - Hilo 0: espera su turno, suma +1 al acumulador y cede el turno.
 *   - Hilo 1: espera su turno, suma +2 al acumulador y cede el turno.
 *
 * De esta forma se garantiza alternancia estricta y se demuestra
 * cómo ambos hilos comparten y modifican la misma región de memoria.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <pthread.h>

#define TAMANIOPAGINA 4096
#define ITERACIONES 5

typedef struct {
    int suma;
    volatile int turno;
} MemoriaCompartida;

MemoriaCompartida *mem;

void *hilo0_func(void *arg) {
    for (int i = 0; i < ITERACIONES; i++) {
        while (mem->turno != 0);

        mem->suma += 1;
        printf("Hilo 0 sumó 1 -> suma = %d  (iteración %d)\n", mem->suma, i);
        sleep(1);

        mem->turno = 1;
    }
    pthread_exit(NULL);
}

void *hilo1_func(void *arg) {
    for (int i = 0; i < ITERACIONES; i++) {
        while (mem->turno != 1);

        mem->suma += 2;
        printf("Hilo 1 sumó 2 -> suma = %d  (iteración %d)\n", mem->suma, i);
        sleep(1);

        mem->turno = 0;
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    mem = mmap(NULL, TAMANIOPAGINA,
               PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        perror("mmap falló");
        exit(1);
    }

    mem->suma = 0;
    mem->turno = 0;

    printf("=== Memoria Compartida con Alternancia ===\n");
    printf("Valor inicial de suma: %d\n\n", mem->suma);

    pthread_t hilo0, hilo1;

    pthread_create(&hilo0, NULL, hilo0_func, NULL);
    pthread_create(&hilo1, NULL, hilo1_func, NULL);

    pthread_join(hilo0, NULL);
    pthread_join(hilo1, NULL);

    printf("\n=== Resultado Final ===\n");
    printf("Suma total en memoria compartida: %d\n", mem->suma);
    printf("Esperado: %d (hilo0: %d x 1) + (hilo1: %d x 2) = %d\n",
           ITERACIONES * 1 + ITERACIONES * 2,
           ITERACIONES, ITERACIONES,
           ITERACIONES * 1 + ITERACIONES * 2);

    munmap(mem, TAMANIOPAGINA);

    return 0;
}
