#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <ranges>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

enum class StripeColor { white, blue, black, red, green };
using Towel = std::vector<StripeColor>;
using TowelView = std::span<const StripeColor>;

struct TowelViewCmp {
    bool operator()(const TowelView& a, const TowelView& b) const
    {
        return std::ranges::lexicographical_compare(a, b);
    }
};

std::optional<StripeColor> parse_color(const char c)
{
    switch (c) {
    case 'w':
        return StripeColor::white;
    case 'u':
        return StripeColor::blue;
    case 'b':
        return StripeColor::black;
    case 'r':
        return StripeColor::red;
    case 'g':
        return StripeColor::green;
    default:
        return std::nullopt;
    }
}

[[maybe_unused]] static void print_towel(const TowelView towel)
{
    for (const StripeColor color : towel) {
        switch (color) {
        case StripeColor::white:
            std::cout << 'w';
            break;
        case StripeColor::blue:
            std::cout << 'u';
            break;
        case StripeColor::black:
            std::cout << 'b';
            break;
        case StripeColor::red:
            std::cout << 'r';
            break;
        case StripeColor::green:
            std::cout << 'g';
            break;
        }
    }
}

static Towel parse_towel(const std::string& data, int& pos)
{
    Towel towel;
    while (true) {
        std::optional<StripeColor> stripe = parse_color(data[pos]);
        if (!stripe.has_value()) {
            return towel;
        }
        towel.push_back(stripe.value());
        ++pos;
    }
}

static std::vector<Towel> parse_available_towels(const std::string& data, int& pos)
{
    std::vector<Towel> towels;
    while (true) {
        towels.push_back(parse_towel(data, pos));
        if (data[pos] == '\n') {
            break;
        }
        pos += 2; // ", "
    }
    return towels;
}

static std::vector<Towel> parse_desired_towels(const std::string& data, int& pos)
{
    std::vector<Towel> towels;
    while (pos < data.size()) {
        towels.push_back(parse_towel(data, pos));
        ++pos; // "\n";
    }
    return towels;
}

static uint64_t desired_towel_combinations_count( // NOLINT(*-no-recursion)
    const std::span<const Towel> available_towels,
    TowelView desired_towel,
    std::map<TowelView, uint64_t, TowelViewCmp>& cache)
{
    if (desired_towel.empty()) {
        return 1;
    }
    if (const auto it = cache.find(desired_towel); it != cache.end()) {
        return it->second;
    }
    uint64_t count = 0;
    for (const Towel& towel : available_towels) {
        if (towel.size() > desired_towel.size()) {
            continue;
        }
        bool match = true;
        for (int i = 0; i < towel.size(); ++i) {
            if (towel[i] != desired_towel[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            const TowelView next_towel { desired_towel.begin() + static_cast<int64_t>(towel.size()),
                                         desired_towel.end() };
            count += desired_towel_combinations_count(available_towels, next_towel, cache);
        }
    }
    cache[desired_towel] = count;
    return count;
}

static uint64_t solve(const std::string& data)
{
    int pos = 0;
    const std::vector<Towel> available_towels = parse_available_towels(data, pos);
    pos += 2; // "\n\n"
    const std::vector<Towel> desired_towels = parse_desired_towels(data, pos);
    uint64_t count = 0;
    std::map<TowelView, uint64_t, TowelViewCmp> cache;
    for (const Towel& desired_towel : desired_towels) {
        count += desired_towel_combinations_count(available_towels, desired_towel, cache);
    }
    return count;
}

int main()
{
    const std::string data = read_data("./day19-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile auto result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data));
#endif
}