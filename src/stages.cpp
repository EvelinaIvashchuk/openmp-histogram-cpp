// [КРОК 5] Незалежні етапи обробки масиву: мінімум, максимум, середнє і гістограма.
// single – ініціалізацію виконує один потік; sections – кожен етап окремий потік;
// barrier – явна точка синхронізації; master – підсумок друкує лише головний потік.
//
//   ./bin/stages [n]   (за замовчуванням n = 10⁸)
#include <omp.h>
#include <cstdio>
#include <cstdlib>
#include "common.hpp"

int stage_min(const std::vector<uint8_t>& a) { return *std::min_element(a.begin(), a.end()); }
int stage_max(const std::vector<uint8_t>& a) { return *std::max_element(a.begin(), a.end()); }

double stage_mean(const std::vector<uint8_t>& a) {
    long long s = 0;
    for (uint8_t v : a) s += v;
    return static_cast<double>(s) / static_cast<double>(a.size());
}

int main(int argc, char** argv) {
    long n = argc > 1 ? std::atol(argv[1]) : 100'000'000L;
    std::vector<uint8_t> a;
    int mn = 0, mx = 0;
    double mean = 0, t_init = 0;
    Hist h;
    int who[4];                                          // який потік виконав кожен етап

    double t0 = omp_get_wtime();
    #pragma omp parallel num_threads(4)
    {
        #pragma omp single
        {
            std::printf("Ініціалізацію виконує потік %d із %d\n", omp_get_thread_num(), omp_get_num_threads());
            a = make_data(n);                            // вкладена паралельна область тут виконується одним потоком
            t_init = omp_get_wtime() - t0;
        }
        // після single – неявний бар'єр: усі чекають, поки масив буде готовий

        double ts = omp_get_wtime();
        #pragma omp sections
        {
            #pragma omp section
            { mn = stage_min(a); who[0] = omp_get_thread_num(); }      // етап 1
            #pragma omp section
            { mx = stage_max(a); who[1] = omp_get_thread_num(); }      // етап 2
            #pragma omp section
            { mean = stage_mean(a); who[2] = omp_get_thread_num(); }   // етап 3
            #pragma omp section
            { h = hist_seq(a); who[3] = omp_get_thread_num(); }        // етап 4
        }
        // після sections – теж неявний бар'єр

        #pragma omp barrier                              // явний бар'єр (тут він лише для наочності)
        #pragma omp master
        {
            std::printf("Етапи (sections): %.3f с; min – потік %d, max – потік %d, mean – потік %d, гістограма – потік %d\n",
                        omp_get_wtime() - ts, who[0], who[1], who[2], who[3]);
            std::printf("n=%ld: min=%d max=%d mean=%.4f, мода – кошик %ld\n", n, mn, mx, mean,
                        static_cast<long>(std::max_element(h.begin(), h.end()) - h.begin()));
        }
    }

    // ті самі етапи послідовно – для порівняння часу
    double t1 = omp_get_wtime();
    int mn2 = stage_min(a), mx2 = stage_max(a);
    double mean2 = stage_mean(a);
    Hist h2 = hist_seq(a);
    double t_seq = omp_get_wtime() - t1;
    std::printf("Ті самі етапи послідовно: %.3f с (ініціалізація %.3f с); результати %s\n", t_seq, t_init,
                mn == mn2 && mx == mx2 && mean == mean2 && h == h2 ? "збігаються" : "НЕ ЗБІГАЮТЬСЯ");
    return 0;
}
