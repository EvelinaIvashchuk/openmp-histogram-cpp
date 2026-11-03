// [КРОК 3] Клаузи shared / private / firstprivate / lastprivate.
// Передбачуваний вивід після області: x=10 y=20 z=70 w=28.
#include <omp.h>
#include <cstdio>

int main() {
    int x = 10, y = 20, z = 30, w = 0;
    std::printf("До області:    x=%d y=%d z=%d w=%d\n", x, y, z, w);

    #pragma omp parallel for num_threads(4) \
            private(x) firstprivate(y) lastprivate(z) shared(w)
    for (int i = 0; i < 8; ++i) {
        x = i;               // приватна копія: початкове значення невизначене, тому спершу присвоюємо
        y += i;              // приватна копія, ініціалізована 20; накопичує лише ітерації свого потоку
        z = i * 10;          // після циклу z = значення з логічно останньої ітерації i = 7
        #pragma omp atomic
        w += i;              // спільна змінна: потрібен atomic
        std::printf("T%d: i=%d x=%d y=%d z=%d\n", omp_get_thread_num(), i, x, y, z);
    }

    std::printf("Після області: x=%d y=%d z=%d w=%d\n", x, y, z, w);
    return 0;
}
