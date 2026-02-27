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

// Estructura para la memoria compartida entre hilos
typedef struct {
    int suma;           // Valor acumulado compartido
    volatile int turno; // Control de alternancia: 0 = hilo 0, 1 = hilo 1
} MemoriaCompartida;

// Puntero global a la memoria compartida (mapeada con mmap)
MemoriaCompartida *mem;

void *hilo0_func(void *arg) {
    for (int i = 0; i < ITERACIONES; i++) {
        // Sección de entrada: espera activa hasta que sea su turno
        while (mem->turno != 0);

        // Sección crítica: suma su valor
        mem->suma += 1;
        printf("Hilo 0 sumó 1 -> suma = %d  (iteración %d)\n", mem->suma, i);
        sleep(1);

        // Sección de salida: cede el turno al hilo 1
        mem->turno = 1;
    }
    pthread_exit(NULL);
}

void *hilo1_func(void *arg) {
    for (int i = 0; i < ITERACIONES; i++) {
        // Sección de entrada: espera activa hasta que sea su turno
        while (mem->turno != 1);

        // Sección crítica: suma su valor
        mem->suma += 2;
        printf("Hilo 1 sumó 2 -> suma = %d  (iteración %d)\n", mem->suma, i);
        sleep(1);

        // Sección de salida: cede el turno al hilo 0
        mem->turno = 0;
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    // Crear memoria compartida con mmap (NO una variable global)
    mem = mmap(NULL, TAMANIOPAGINA,
               PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        perror("mmap falló");
        exit(1);
    }

    // Inicializar valores en la memoria compartida
    mem->suma = 0;
    mem->turno = 0;  // Empieza el hilo 0

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

    // Liberar la memoria compartida
    munmap(mem, TAMANIOPAGINA);

    return 0;
}
