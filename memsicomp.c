/*
 * memsicomp.c - Memoria compartida con alternancia estricta usando fork()
 *
 * Este programa demuestra el uso de memoria compartida entre procesos
 * (padre e hijo) utilizando mmap() con MAP_SHARED | MAP_ANONYMOUS.
 * Se almacena una estructura con un acumulador (suma) y una variable
 * de turno en la región mapeada.
 *
 * Al hacer fork(), cada proceso tiene su propio espacio de memoria,
 * pero la región creada con mmap(MAP_SHARED) es visible para ambos.
 * Los procesos alternan su ejecución mediante espera activa sobre
 * la variable "turno" en memoria compartida:
 *   - Proceso padre: espera su turno, suma +1 al acumulador y cede el turno.
 *   - Proceso hijo:  espera su turno, suma +1 al acumulador y cede el turno.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>
#include <stdint.h>
#include <sys/mman.h>
#include <inttypes.h>

#define TAMANIOPAGINA 4096
#define ITERACIONES 5

typedef struct {
    int suma;
    volatile int turno;
} MemoriaCompartida;

int main(int argc, char **argv)
{
    MemoriaCompartida *mem = mmap(NULL, TAMANIOPAGINA,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        perror("mmap falló");
        exit(1);
    }

    mem->suma = 0;
    mem->turno = 0;

    printf("=== Memoria Compartida con Alternancia (fork) ===\n");
    printf("Valor inicial de suma: %d\n\n", mem->suma);

    pid_t pid = fork();

    if (pid == 0) {
        // Proceso hijo: suma +1 en cada turno
        for (int i = 0; i < ITERACIONES; i++) {
            while (mem->turno != 1);

            mem->suma += 1;
            printf("Hijo  sumó 1 -> suma = %d  (iteración %d)  PID: %d\n", mem->suma, i, getpid());

            mem->turno = 0;
        }
        exit(0);
    } else {
        // Proceso padre: suma +1 en cada turno
        for (int i = 0; i < ITERACIONES; i++) {
            while (mem->turno != 0);

            mem->suma += 1;
            printf("Padre sumó 1 -> suma = %d  (iteración %d)  PID: %d\n", mem->suma, i, getpid());

            mem->turno = 1;
        }

        wait(NULL);

        printf("\n=== Resultado Final ===\n");
        printf("Suma total en memoria compartida: %d\n", mem->suma);
        printf("Esperado: %d (padre: %d x 1) + (hijo: %d x 1) = %d\n",
               ITERACIONES * 1 + ITERACIONES * 1,
               ITERACIONES, ITERACIONES,
               ITERACIONES * 1 + ITERACIONES * 1);
    }

    munmap(mem, TAMANIOPAGINA);

    return 0;
}
