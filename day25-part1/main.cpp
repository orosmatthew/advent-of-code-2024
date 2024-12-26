#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <ranges>
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

enum class SchematicType { lock, key };

using SchematicHeights = std::array<int8_t, 5>;

struct Schematic {
    SchematicType type;
    SchematicHeights heights;
};

static Schematic parse_schematic(const std::string& data, int& pos)
{
    auto type = SchematicType::lock;
    for (int i = 0; i < 5; ++i) {
        if (data[pos + i] != '#') {
            type = SchematicType::key;
            break;
        }
    }
    std::array<std::optional<int8_t>, 5> height_opts;
    for (int y = 0; y < 7; ++y) {
        for (int x = 0; x < 5; ++x) {
            if (type == SchematicType::lock) {
                if (!height_opts[x].has_value() && data[pos] != '#') {
                    height_opts[x] = y - 1;
                }
            }
            else {
                if (!height_opts[x].has_value() && data[pos] == '#') {
                    height_opts[x] = y;
                }
            }
            ++pos;
        }
        ++pos; // "\n"
    }
    std::array<int8_t, 5> heights {};
    for (int i = 0; i < 5; ++i) {
        assert(height_opts[i].has_value());
        heights[i] = height_opts[i].value();
    }
    return { type, heights };
}

static bool overlap(const SchematicHeights& lock, const SchematicHeights& key)
{
    for (int i = 0; i < 5; ++i) {
        if (lock[i] >= key[i]) {
            return true;
        }
    }
    return false;
}

static uint64_t solve(const std::string& data)
{
    std::vector<SchematicHeights> locks;
    std::vector<SchematicHeights> keys;
    for (int pos = 0; pos < data.size(); ++pos) {
        if (auto [type, heights] = parse_schematic(data, pos); type == SchematicType::lock) {
            locks.push_back(heights);
        }
        else {
            keys.push_back(heights);
        }
    }
    uint64_t count = 0;
    for (const SchematicHeights& lock : locks) {
        for (const SchematicHeights& key : keys) {
            if (!overlap(lock, key)) {
                ++count;
            }
        }
    }
    return count;
}

int main()
{
    const std::string data = read_data("./day25-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 10000;
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