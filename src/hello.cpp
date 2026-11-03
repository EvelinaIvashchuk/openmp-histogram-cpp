// [КРОК 1] Програма Hello: номери потоків і три способи задати їхню кількість.
//
//   ./bin/hello                     – кількість за замовчуванням (= кількість логічних ядер)
//   OMP_NUM_THREADS=3 ./bin/hello   – спосіб 1: змінна середовища
//   ./bin/hello 4                   – спосіб 2: omp_set_num_threads(4) перед областю
//   ./bin/hello 4 2                 – спосіб 3: клауза num_threads(2) – має пріоритет над omp_set_num_threads(4)
#include <omp.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    std::printf("Логічних ядер: %d, потоків за замовчуванням: %d\n",
                omp_get_num_procs(), omp_get_max_threads());

    if (argc > 1) {
        omp_set_num_threads(std::atoi(argv[1]));            // спосіб 2: виклик функції
        std::printf("omp_set_num_threads(%s)\n", argv[1]);
    }
    int clause = argc > 2 ? std::atoi(argv[2]) : 0;         // 0 – клаузу не задано

    #pragma omp parallel num_threads(clause > 0 ? clause : omp_get_max_threads())
    {
        int id = omp_get_thread_num();                      // номер потоку в команді
        int n = omp_get_num_threads();                      // розмір команди
        #pragma omp critical                                // рядки з різних потоків не перемішуються
        std::printf("Привіт від потоку %d із %d\n", id, n);
    }
    if (clause > 0) std::printf("num_threads(%d) переважила omp_set_num_threads\n", clause);
    return 0;
}
