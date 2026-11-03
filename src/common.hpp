// Спільні допоміжні функції для програм ЛР4.
#pragma once
#include <omp.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// Гістограма значень 0…255: 256 кошиків-лічильників
using Hist = std::vector<long long>;
constexpr int BINS = 256;

// Детермінований «випадковий» хеш індексу (splitmix64): ті самі дані при кожному запуску
inline uint64_t mix(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

// Масив n значень 0…255. Середнє двох випадкових байтів дає «трикутний» розподіл з піком біля 127.
// Генерування паралельне, але в час обчислень не входить.
inline std::vector<uint8_t> make_data(long n) {
    std::vector<uint8_t> a(n);
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < n; ++i) {
        uint64_t h = mix(static_cast<uint64_t>(i));
        a[i] = static_cast<uint8_t>(((h >> 56) + ((h >> 48) & 0xFF)) / 2);
    }
    return a;
}

// Послідовна (еталонна) версія
inline Hist hist_seq(const std::vector<uint8_t>& a) {
    Hist h(BINS, 0);
    for (uint8_t v : a) ++h[v];
    return h;
}

// Скільки кошиків відрізняється від еталона і на скільки втрачено значень загалом
inline void compare(const Hist& ref, const Hist& h, int& wrong_bins, long long& lost) {
    wrong_bins = 0;
    lost = 0;
    for (int b = 0; b < BINS; ++b) {
        if (h[b] != ref[b]) ++wrong_bins;
        lost += ref[b] - h[b];
    }
}

// Медіана вектора часів
inline double median(std::vector<double> t) {
    std::sort(t.begin(), t.end());
    return t[t.size() / 2];
}

// Доповнює рядок пробілами до width символів (printf("%-Ns") рахує байти, а кирилиця – 2 байти на літеру)
inline std::string pad(const std::string& s, int width) {
    int chars = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++chars;
    return s + std::string(chars < width ? width - chars : 0, ' ');
}
