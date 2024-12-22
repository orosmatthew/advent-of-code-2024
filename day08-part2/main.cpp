#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

struct Vector2i {
    int x;
    int y;

    Vector2i& operator+=(const Vector2i& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2i operator+(const Vector2i& other) const
    {
        return { x + other.x, y + other.y };
    }

    Vector2i& operator-=(const Vector2i& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2i operator-(const Vector2i& other) const
    {
        return { x - other.x, y - other.y };
    }

    bool operator==(const Vector2i& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector2i& other) const
    {
        return x != other.x || y != other.y;
    }

    bool operator<(const Vector2i& other) const
    {
        if (x == other.x) {
            return y < other.y;
        }
        return x < other.x;
    }
};

template <>
struct std::hash<Vector2i> {
    size_t operator()(const Vector2i& vector) const noexcept
    {
        const size_t hash1 = std::hash<int>()(vector.x);
        const size_t hash2 = std::hash<int>()(vector.y);
        return hash1 ^ hash2 << 1;
    }
};

class Map {
public:
    static Map parse(const std::string& data)
    {
        std::unordered_map<char, std::vector<Vector2i>> antennas;
        std::optional<int> width;
        int y = 0;
        for (int i = 0; i < data.size(); ++i) {
            const char c = data[i];
            if (c == '\n') {
                if (!width.has_value()) {
                    width = i;
                }
                ++y;
                continue;
            }
            if (c != '.') {
                const int x = width.has_value() ? i % (width.value() + 1) : i;
                if (auto it = antennas.find(c); it != antennas.end()) {
                    it->second.push_back({ x, y });
                }
                else {
                    std::vector<Vector2i> v;
                    v.push_back({ x, y });
                    antennas[c] = std::move(v);
                }
            }
        }
        Vector2i size { width.value(), y };
        return { std::move(antennas), size };
    }

    [[nodiscard]] std::unordered_set<Vector2i> antinodes() const
    {
        std::unordered_set<Vector2i> antinodes;
        for (const auto& positions : m_antennas | std::views::values) {
            for (int i = 0; i < positions.size(); ++i) {
                for (int j = i + 1; j < positions.size(); ++j) {
                    Vector2i a = positions[i];
                    Vector2i b = positions[j];
                    if (b < a) {
                        std::swap(a, b);
                    }
                    const Vector2i diff = b - a;
                    Vector2i antinode = a;
                    do {
                        antinodes.insert(antinode);
                        antinode -= diff;
                    } while (in_bounds(antinode));
                    antinode = b;
                    do {
                        antinodes.insert(antinode);
                        antinode += diff;
                    } while (in_bounds(antinode));
                }
            }
        }
        return antinodes;
    }

private:
    Map(std::unordered_map<char, std::vector<Vector2i>>&& antennas, const Vector2i& size)
        : m_antennas { std::move(antennas) }
        , m_size { size }
    {
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    std::unordered_map<char, std::vector<Vector2i>> m_antennas;
    Vector2i m_size {};
};

static int64_t solve(const std::string& data)
{
    const Map map = Map::parse(data);
    return static_cast<int64_t>(map.antinodes().size());
}

int main()
{
    const std::string data = read_data("./day08-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile int64_t result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%lld\n", solve(data));
#endif
}