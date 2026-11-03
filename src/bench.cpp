// [КРОК 6] Вимірювання часу гістограми (reduction по масиву) для p = 1, 2, 4, 8, 16
// і трьох розмірів масиву. Прискорення S(p) = T_seq / T(p) – відносно найкращої послідовної
// версії, ефективність E(p) = S(p) / p. Кожен замір – медіана з 5 після прогріву.
//
//   ./bin/bench [n1 n2 ...]   (за замовчуванням 10⁸, 5·10⁸, 10⁹)
#include <omp.h>
#include <cstdio>
#include <cstdlib>
#include "common.hpp"

Hist hist_reduction(const std::vector<uint8_t>& a) {
    Hist h(BINS, 0);
    long long* hp = h.data();
    const long n = static_cast<long>(a.size());
    #pragma omp parallel for schedule(static) reduction(+ : hp[:BINS])
    for (long i = 0; i < n; ++i) ++hp[a[i]];
    return h;
}

template <class F>
double median_time(F f, int repeats) {
    f();                                                 // прогрів
    std::vector<double> t;
    for (int k = 0; k < repeats; ++k) {
        double t0 = omp_get_wtime();
        f();
        t.push_back(omp_get_wtime() - t0);
    }
    return median(t);
}

int main(int argc, char** argv) {
    std::vector<long> sizes;
    for (int i = 1; i < argc; ++i) sizes.push_back(std::atol(argv[i]));
    if (sizes.empty()) sizes = {100'000'000L, 500'000'000L, 1'000'000'000L};
    const int ps[] = {1, 2, 4, 8, 16};

    std::printf("Логічних ядер: %d; гістограма, reduction(+ : h[:256]), медіана з 5 після прогріву\n",
                omp_get_num_procs());
    for (long n : sizes) {
        std::vector<uint8_t> a = make_data(n);
        Hist ref;
        double t_seq = median_time([&] { ref = hist_seq(a); }, 5);
        std::printf("\nn = %ld, T_seq = %.1f мс\n", n, t_seq * 1e3);
        std::printf("%4s %10s %8s %8s   %s\n", "p", "T(p), мс", "S(p)", "E(p)", "Перевірка");
        for (int p : ps) {
            omp_set_num_threads(p);
            Hist h;
            double t = median_time([&] { h = hist_reduction(a); }, 5);
            std::printf("%4d %10.1f %8.2f %8.2f   %s\n", p, t * 1e3, t_seq / t, t_seq / t / p,
                        h == ref ? "збіг" : "ПОМИЛКА");
        }
    }
    return 0;
}
