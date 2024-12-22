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

struct Vector2i {
    int64_t x;
    int64_t y;

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

    Vector2i operator*(const int64_t value) const
    {
        return { x * value, y * value };
    }

    Vector2i operator/(const int64_t value) const
    {
        return { x / value, y / value };
    }

    Vector2i& operator%=(const Vector2i& other)
    {
        x %= other.x;
        y %= other.y;
        return *this;
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

enum class Dir { north, east, south, west };
constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west };

static Vector2i dir_offset(const Dir dir)
{
    switch (dir) {
    case Dir::north:
        return { 0, -1 };
    case Dir::east:
        return { 1, 0 };
    case Dir::south:
        return { 0, 1 };
    case Dir::west:
        return { -1, 0 };
    }
    std::unreachable();
}

class Map {
public:
    struct Cheat {
        Vector2i start;
        Vector2i end;
        int64_t picoseconds_saved;
    };

    static Map parse(const std::string& data)
    {
        std::vector<bool> walls;
        std::optional<int> width;
        std::optional<Vector2i> start;
        std::optional<Vector2i> end;
        int y = 0;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == '\n') {
                if (!width.has_value()) {
                    width = i;
                }
                ++y;
                continue;
            }
            const int x = width.has_value() ? i % (width.value() + 1) : i;
            switch (data[i]) {
            case '#':
                walls.push_back(true);
                break;
            case '.':
                walls.push_back(false);
                break;
            case 'S':
                assert(!start.has_value());
                start = { x, y };
                walls.push_back(false);
                break;
            case 'E':
                assert(!end.has_value());
                end = { x, y };
                walls.push_back(false);
                break;
            default:
                assert(false);
            }
        }
        assert(width.has_value() && start.has_value() && end.has_value());
        assert(walls.size() == width.value() * y);
        return { std::move(walls), { width.value(), y }, start.value(), end.value() };
    }

    [[nodiscard]] std::vector<Cheat> cheats() const
    {
        std::vector<Vector2i> traversed_positions;
        std::vector<std::optional<int64_t>> time_grid;
        traverse(traversed_positions, time_grid);
        std::vector<Cheat> cheats;
        for (const Vector2i& pos : traversed_positions) {
            for (const Dir dir : dirs) {
                const Vector2i offset = dir_offset(dir);
                const Vector2i cheat_start = pos + offset;
                if (!m_walls[index(cheat_start)]) {
                    continue;
                }
                const Vector2i cheat_end = cheat_start + offset;
                if (!in_bounds(cheat_end)) {
                    continue;
                }
                assert(time_grid[index(pos)].has_value());
                const int64_t current_time = time_grid[index(pos)].value();
                const std::optional<int64_t> cheat_end_time = time_grid[index(cheat_end)];
                if (!cheat_end_time.has_value()) {
                    continue;
                }
                const int64_t time_saved = cheat_end_time.value() - current_time - 2;
                if (time_saved <= 0) {
                    continue;
                }
                cheats.push_back({ .start = cheat_start, .end = cheat_end, .picoseconds_saved = time_saved });
            }
        }
        return cheats;
    }

private:
    Map(std::vector<bool> walls, const Vector2i& size, const Vector2i& start, const Vector2i& end)
        : m_walls { std::move(walls) }
        , m_size { size }
        , m_start { start }
        , m_end { end }
    {
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    [[nodiscard]] size_t index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    void traverse(std::vector<Vector2i>& positions, std::vector<std::optional<int64_t>>& time_grid) const
    {
        positions.clear();
        time_grid.clear();
        time_grid.resize(m_size.x * m_size.y, std::nullopt);
        positions.push_back(m_start);
        time_grid[index(m_start)] = 0;
        std::optional<Vector2i> prev;
        Vector2i current = m_start;
        int64_t time_count = 1;
        while (current != m_end) {
            for (const Dir dir : dirs) {
                const Vector2i neighbor_pos = current + dir_offset(dir);
                if (prev.has_value() && neighbor_pos == prev.value()) {
                    continue;
                }
                const size_t neighbor_index = index(neighbor_pos);
                if (m_walls[neighbor_index]) {
                    continue;
                }
                positions.push_back(neighbor_pos);
                time_grid[neighbor_index] = time_count;
                prev = current;
                current = neighbor_pos;
                break;
            }
            ++time_count;
        }
    }

    std::vector<bool> m_walls;
    Vector2i m_size;
    Vector2i m_start;
    Vector2i m_end;
};

static uint64_t solve(const std::string& data)
{
    const Map map = Map::parse(data);
    const std::vector<Map::Cheat> cheats = map.cheats();
    uint64_t cheats_count = 0;
    for (const Map::Cheat& cheat : cheats) {
        if (cheat.picoseconds_saved >= 100) {
            ++cheats_count;
        }
    }
    return cheats_count;
}

int main()
{
    const std::string data = read_data("./day20-part1/input.txt");

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