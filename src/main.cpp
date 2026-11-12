// Програмний проєкт 3, варіант 8.
// Схема №8, nt = 4; кількість дій: a=5, b=6, c=4, d=8, e=9, f=6, g=6, h=9, i=7, j=6, k=4
// (дуг m і n на схемі №8 немає, тому m=2, n=1 не використовуються).
// Компілятор: g++ 15.2.0 (MSYS2 UCRT64), стандарт C++20.
//
// Потоки і синхронізація: лише std::jthread і std::latch. Виведення: лише std::osyncstream.
// Розподіл дій по потоках зафіксовано в таблицях plan_t1..plan_t4 (див. діаграму).
// Якщо кожна дія триває 1 с, обчислення займає 18 с = ceil(70 дій / 4 потоки).

#include <chrono>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <latch>
#include <span>
#include <syncstream>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

constexpr int nt = 4;

constexpr int na = 5, nb = 6, nc = 4, nd = 8, ne = 9, nf = 6;
constexpr int ng = 6, nh = 9, ni = 7, nj = 6, nk = 4;

std::latch done_a{na};
std::latch done_b{nb};
std::latch done_cg{nc + ng};
std::latch done_ed{ne + nd};
std::latch done_j{nj};
std::latch done_hi{nh + ni};

struct ActionSet {
    char name;
    int count;
    std::latch* start;
    std::latch* finish;
};

constexpr ActionSet set_a{'a', na, nullptr, &done_a};
constexpr ActionSet set_b{'b', nb, nullptr, &done_b};
constexpr ActionSet set_c{'c', nc, &done_a, &done_cg};
constexpr ActionSet set_d{'d', nd, &done_b, &done_ed};
constexpr ActionSet set_e{'e', ne, &done_a, &done_ed};
constexpr ActionSet set_f{'f', nf, &done_j, nullptr};
constexpr ActionSet set_g{'g', ng, &done_b, &done_cg};
constexpr ActionSet set_h{'h', nh, &done_cg, &done_hi};
constexpr ActionSet set_i{'i', ni, &done_ed, &done_hi};
constexpr ActionSet set_j{'j', nj, &done_cg, &done_j};
constexpr ActionSet set_k{'k', nk, &done_hi, nullptr};

constexpr const ActionSet* all_sets[] = {&set_a, &set_b, &set_c, &set_d, &set_e, &set_f,
                                         &set_g, &set_h, &set_i, &set_j, &set_k};

struct Block {
    const ActionSet* set;
    int from, to;
};

constexpr Block plan_t1[] = {
    {&set_a, 1, 2},
    {&set_c, 1, 4},
    {&set_j, 1, 6},
    {&set_f, 1, 6},
};

constexpr Block plan_t2[] = {
    {&set_a, 3, 4},
    {&set_e, 1, 2},
    {&set_g, 1, 2},
    {&set_e, 4, 8},
    {&set_i, 1, 4},
    {&set_h, 9, 9},
    {&set_k, 1, 2},
};

constexpr Block plan_t3[] = {
    {&set_a, 5, 5},
    {&set_b, 5, 6},
    {&set_e, 3, 3},
    {&set_g, 3, 4},
    {&set_d, 1, 4},
    {&set_e, 9, 9},
    {&set_i, 5, 7},
    {&set_h, 7, 8},
    {&set_k, 3, 3},
};

constexpr Block plan_t4[] = {
    {&set_b, 1, 4},
    {&set_g, 5, 6},
    {&set_d, 5, 8},
    {&set_h, 1, 6},
    {&set_k, 4, 4},
};

constexpr std::span<const Block> plans[] = {plan_t1, plan_t2, plan_t3, plan_t4};
static_assert(std::size(plans) == nt, "кожен з nt потоків повинен мати свій план");

consteval bool each_action_exactly_once()
{
    for (const auto& plan : plans)
        for (const Block& blk : plan)
            if (blk.from < 1 || blk.from > blk.to || blk.to > blk.set->count)
                return false;

    for (const ActionSet* s : all_sets)
        for (int i = 1; i <= s->count; ++i) {
            int times = 0;
            for (const auto& plan : plans)
                for (const Block& blk : plan)
                    if (blk.set == s && blk.from <= i && i <= blk.to)
                        ++times;
            if (times != 1)
                return false;
        }
    return true;
}
static_assert(each_action_exactly_once(), "розподіл дій по потоках неповний або з повтором");

void f(char x, int i)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::osyncstream(std::cout) << "З набору " << x << " виконано дію " << i << ".\n";
}

void worker(std::span<const Block> plan)
{
    for (const Block& blk : plan) {
        const ActionSet& s = *blk.set;
        if (s.start)
            s.start->wait();
        for (int i = blk.from; i <= blk.to; ++i) {
            f(s.name, i);
            if (s.finish)
                s.finish->count_down();
        }
    }
}

}

int main()
{
#ifdef _WIN32
    const UINT previous_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::osyncstream(std::cout) << "Обчислення розпочато.\n";
    const auto started = std::chrono::steady_clock::now();
    {
        std::jthread workers[nt];
        for (int t = 0; t < nt; ++t)
            workers[t] = std::jthread(worker, plans[t]);
    }
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;

    std::osyncstream(std::cout) << "Обчислення завершено. Тривалість обчислення " << std::fixed
                                << std::setprecision(3) << elapsed.count() << " секунд.\n";

#ifdef _WIN32
    SetConsoleOutputCP(previous_cp);
#endif
    return 0;
}
