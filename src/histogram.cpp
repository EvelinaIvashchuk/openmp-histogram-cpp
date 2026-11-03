// [КРОК 2] Варіант 5: гістограма масиву значень 0…255.
// Послідовна версія, паралельна з гонкою на масиві кошиків і чотири способи її усунути:
// critical, atomic, reduction(+ : h[:256]) і приватні гістограми зі злиттям.
//
//   ./bin/histogram race [n] [p]   – лише версія з гонкою (запускати 3–5 разів)
//   ./bin/histogram [n] [p]        – усі версії: час (медіана з 3; версії довші за 5 с – один запуск),
//                                    прискорення, перевірка
// За замовчуванням n = 10⁸, p = 8.
#include <omp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "common.hpp"

// Гонка: усі потоки збільшують спільні кошики без захисту – частина інкрементів втрачається
Hist hist_race(const std::vector<uint8_t>& a) {
    Hist h(BINS, 0);
    const long n = static_cast<long>(a.size());
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < n; ++i) ++h[a[i]];
    return h;
}

// 1) critical – коректно, але кожна ітерація проходить через один спільний замок
Hist hist_critical(const std::vector<uint8_t>& a) {
    Hist h(BINS, 0);
    const long n = static_cast<long>(a.size());
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < n; ++i) {
        #pragma omp critical
        ++h[a[i]];
    }
    return h;
}

// 2) atomic – апаратна атомарна операція над одним кошиком, без загального замка
Hist hist_atomic(const std::vector<uint8_t>& a) {
    Hist h(BINS, 0);
    const long n = static_cast<long>(a.size());
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < n; ++i) {
        #pragma omp atomic
        ++h[a[i]];
    }
    return h;
}

// 3) reduction по масиву: у кожного потоку приватна копія 256 кошиків, після циклу копії сумуються
Hist hist_reduction(const std::vector<uint8_t>& a) {
    Hist h(BINS, 0);
    long long* hp = h.data();
    const long n = static_cast<long>(a.size());
    #pragma omp parallel for schedule(static) reduction(+ : hp[:BINS])
    for (long i = 0; i < n; ++i) ++hp[a[i]];
    return h;
}

// 4) те саме вручну: приватна гістограма в кожного потоку і злиття в critical – один раз на потік
Hist hist_private(const std::vector<uint8_t>& a) {
    Hist h(BINS, 0);
    const long n = static_cast<long>(a.size());
    #pragma omp parallel
    {
        long long local[BINS] = {0};                     // локальна змінна – приватна для потоку
        #pragma omp for schedule(static) nowait
        for (long i = 0; i < n; ++i) ++local[a[i]];
        #pragma omp critical
        for (int b = 0; b < BINS; ++b) h[b] += local[b];
    }
    return h;
}

int main(int argc, char** argv) {
    int arg = 1;
    bool race_only = argc > 1 && std::strcmp(argv[1], "race") == 0;
    if (race_only) ++arg;
    long n = argc > arg ? std::atol(argv[arg]) : 100'000'000L;
    int p = argc > arg + 1 ? std::atoi(argv[arg + 1]) : 8;
    omp_set_num_threads(p);

    std::vector<uint8_t> a = make_data(n);                // генерування в час не входить
    Hist ref = hist_seq(a);                               // еталон

    if (race_only) {
        double t0 = omp_get_wtime();
        Hist h = hist_race(a);
        double t = omp_get_wtime() - t0;
        int wrong;
        long long lost;
        compare(ref, h, wrong, lost);
        std::printf("n=%ld p=%d  гонка: %.3f с, неправильних кошиків %d з %d, втрачено %lld значень\n",
                    n, p, t, wrong, BINS, lost);
        return 0;
    }

    struct Version { const char* name; Hist (*run)(const std::vector<uint8_t>&); };
    Version versions[] = {
        {"послідовна", hist_seq},
        {"parallel for без захисту", hist_race},
        {"critical", hist_critical},
        {"atomic", hist_atomic},
        {"reduction(+ : h[:256])", hist_reduction},
        {"приватні гістограми", hist_private},
    };
    hist_reduction(a);                                    // прогрів: створення потоків пулу

    std::printf("Гістограма: n = %ld, p = %d, медіана з 3 запусків\n", n, p);
    std::printf("%s %9s %8s   %s\n", pad("Версія", 26).c_str(), "Час, с", "S", "Перевірка");
    double t_seq = 0;
    for (const Version& v : versions) {
        std::vector<double> times;
        Hist h;
        for (int k = 0; k < 3; ++k) {
            double t0 = omp_get_wtime();
            h = v.run(a);
            times.push_back(omp_get_wtime() - t0);
            if (times.back() > 5.0) break;                // повільну версію не повторюємо
        }
        double t = median(times);
        if (v.run == hist_seq) t_seq = t;
        int wrong;
        long long lost;
        compare(ref, h, wrong, lost);
        char check[96];
        if (wrong == 0) std::snprintf(check, sizeof check, "збіг");
        else std::snprintf(check, sizeof check, "ПОМИЛКА: %d кошиків, втрачено %lld", wrong, lost);
        std::printf("%s %9.3f %8.2f   %s\n", pad(v.name, 26).c_str(), t, t_seq / t, check);
    }
    return 0;
}
