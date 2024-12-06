#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
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
    int x;
    int y;

    Vector2i& operator+=(const Vector2i& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

class Map {
public:
    static Map parse(const std::string& string)
    {
        std::optional<int> width;
        std::optional<int> guard_index;
        std::vector<GridSquare> grid;
        int pos = 0;
        while (pos < string.length()) {
            while (string[pos] != '\n') {
                if (string[pos] == '.') {
                    grid.push_back({ .obstacle = false, .visited = false });
                }
                else if (string[pos] == '#') {
                    grid.push_back({ .obstacle = true, .visited = false });
                }
                else if (string[pos] == '^') {
                    grid.push_back({ .obstacle = false, .visited = true });
                    guard_index = static_cast<int>(grid.size()) - 1;
                }
                else {
                    assert(false); // Invalid grid char
                }
                ++pos;
            }
            if (!width.has_value()) {
                width = pos;
            }
            ++pos;
        }
        assert(guard_index.has_value()); // Map must contain guard
        const Vector2i guard_pos = { guard_index.value() % width.value(), guard_index.value() / width.value() };
        return Map { width.value(), std::move(grid), guard_pos };
    }

    int move_and_count_visited()
    {
        while (true) {
            if (const MoveResult result = move_until_stopped(); result == MoveResult::out_of_bounds) {
                break;
            }
            rotate_guard();
        }
        int count = 0;
        for (const auto& [obstacle, visited] : m_grid) {
            if (visited) {
                ++count;
            }
        }
        return count;
    }

private:
    struct GridSquare {
        bool obstacle;
        bool visited;
    };

    enum class Dir { north, east, south, west };

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

    Map(const int width, std::vector<GridSquare> grid, const Vector2i& guard_pos)
        : m_grid { std::move(grid) }
        , m_size { width, static_cast<int>(m_grid.size()) / width }
        , m_guard_pos { guard_pos }
        , m_guard_dir { Dir::north }
    {
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    [[nodiscard]] int index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    enum class MoveResult { out_of_bounds, obstacle };

    MoveResult move_until_stopped()
    {
        const Vector2i move_offset = dir_offset(m_guard_dir);
        Vector2i current = m_guard_pos;
        while (true) {
            current += move_offset;
            if (!in_bounds(current)) {
                return MoveResult::out_of_bounds;
            }
            auto& [obstacle, visited] = m_grid[index(current)];
            if (obstacle) {
                return MoveResult::obstacle;
            }
            visited = true;
            m_guard_pos = current;
        }
    }

    void rotate_guard()
    {
        switch (m_guard_dir) {
        case Dir::north:
            m_guard_dir = Dir::east;
            break;
        case Dir::east:
            m_guard_dir = Dir::south;
            break;
        case Dir::south:
            m_guard_dir = Dir::west;
            break;
        case Dir::west:
            m_guard_dir = Dir::north;
            break;
        }
    }

    std::vector<GridSquare> m_grid;
    Vector2i m_size;
    Vector2i m_guard_pos;
    Dir m_guard_dir;
};

static int solve(const std::string& data)
{
    auto map = Map::parse(data);
    return map.move_and_count_visited();
}

int main()
{
    const std::string data = read_data("./day6-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile int result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%d\n", solve(data));
#endif
}