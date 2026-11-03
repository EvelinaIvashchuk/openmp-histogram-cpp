// [КРОК 4] Порівняння способів планування на задачі з нерівномірними ітераціями:
// підрахунок простих чисел до n пробним діленням (вартість ітерації зростає як √i).
// Ітерації гістограми однакові за вартістю, тому для цього кроку взято прості числа.
//
//   OMP_SCHEDULE="dynamic,1000" ./bin/primes_schedule [n]   – один запуск з schedule(runtime)
//   ./bin/primes_schedule all [n] [p]                        – таблиця для всіх способів (медіана з 3)
// За замовчуванням n = 2·10⁷, p = 8.
#include <omp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "common.hpp"

bool is_prime(long n) {
    if (n < 2) return false;
    for (long d = 2; d * d <= n; ++d)
        if (n % d == 0) return false;
    return true;
}

long count_seq(long n) {
    long cnt = 0;
    for (long i = 2; i <= n; ++i)
        if (is_prime(i)) ++cnt;
    return cnt;
}

// Спосіб планування задається змінною середовища OMP_SCHEDULE або omp_set_schedule()
long count_primes(long n) {
    long cnt = 0;
    #pragma omp parallel for reduction(+ : cnt) schedule(runtime)
    for (long i = 2; i <= n; ++i)
        if (is_prime(i)) ++cnt;
    return cnt;
}

double timed(long (*f)(long), long n, long& result, int repeats) {
    std::vector<double> t;
    for (int k = 0; k < repeats; ++k) {
        double t0 = omp_get_wtime();
        result = f(n);
        t.push_back(omp_get_wtime() - t0);
    }
    return median(t);
}

int main(int argc, char** argv) {
    bool all = argc > 1 && std::strcmp(argv[1], "all") == 0;
    int arg = all ? 2 : 1;
    long n = argc > arg ? std::atol(argv[arg]) : 20'000'000L;

    if (!all) {
        double t0 = omp_get_wtime();
        long c = count_primes(n);
        omp_sched_t kind;
        int chunk;
        omp_get_schedule(&kind, &chunk);
        std::printf("primes(%ld) = %ld, time = %.3f s, threads = %d, schedule kind = %d, chunk = %d\n",
                    n, c, omp_get_wtime() - t0, omp_get_max_threads(), static_cast<int>(kind), chunk);
        return 0;
    }

    int p = argc > arg + 1 ? std::atoi(argv[arg + 1]) : 8;
    omp_set_num_threads(p);
    struct S { const char* name; omp_sched_t kind; int chunk; };
    S kinds[] = {
        {"static", omp_sched_static, 0},                 // 0 – порція за замовчуванням (рівні блоки)
        {"static, 1000", omp_sched_static, 1000},
        {"static, 100000", omp_sched_static, 100000},
        {"dynamic, 1", omp_sched_dynamic, 1},
        {"dynamic, 1000", omp_sched_dynamic, 1000},
        {"guided, 1000", omp_sched_guided, 1000},
    };

    long ref;
    double t_seq = timed(count_seq, n, ref, 3);
    std::printf("Прості числа до %ld: %ld; p = %d, медіана з 3 запусків\n", n, ref, p);
    std::printf("%s %8s %7s %7s   %s\n", pad("schedule", 18).c_str(), "Час, с", "S", "E", "Перевірка");
    std::printf("%s %8.3f %7.2f %7s   %s\n", pad("послідовна", 18).c_str(), t_seq, 1.0, "–", "еталон");
    for (const S& s : kinds) {
        omp_set_schedule(s.kind, s.chunk);
        long c;
        double t = timed(count_primes, n, c, 3);
        std::printf("%s %8.3f %7.2f %7.2f   %s\n", pad(s.name, 18).c_str(), t, t_seq / t, t_seq / t / p,
                    c == ref ? "збіг" : "ПОМИЛКА");
    }
    return 0;
}
