#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
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

enum class Dir { north = 0, east = 1, south = 2, west = 3 };
constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west };

static int dir_index(const Dir dir)
{
    return static_cast<int>(dir);
}

static Dir index_dir(const int index)
{
    assert(index >= 0 && index <= 3);
    return static_cast<Dir>(index);
}

[[maybe_unused]] static std::string dir_str(const Dir dir)
{
    switch (dir) {
    case Dir::north:
        return "^";
    case Dir::east:
        return ">";
    case Dir::south:
        return "v";
    case Dir::west:
        return "<";
    }
    std::unreachable();
}

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

static std::array<Dir, 2> rotate_dirs(const Dir dir)
{
    switch (dir) {
    case Dir::north:
        return { Dir::west, Dir::east };
    case Dir::east:
        return { Dir::north, Dir::south };
    case Dir::south:
        return { Dir::east, Dir::west };
    case Dir::west:
        return { Dir::south, Dir::north };
    }
    std::unreachable();
}

static Dir opposite_dir(const Dir dir)
{
    switch (dir) {
    case Dir::north:
        return Dir::south;
    case Dir::east:
        return Dir::west;
    case Dir::south:
        return Dir::north;
    case Dir::west:
        return Dir::east;
    }
    std::unreachable();
}

class Maze {
public:
    static Maze parse(const std::string& data)
    {
        std::vector<bool> walls;
        std::optional<int> width;
        std::optional<Vector2i> start_pos;
        std::optional<Vector2i> end_pos;
        int y = 0;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == '\n') {
                if (!width.has_value()) {
                    width = i;
                }
                ++y;
                continue;
            }
            switch (data[i]) {
            case 'S':
                assert(!start_pos.has_value());
                start_pos = { i % (width.value() + 1), y };
                walls.push_back(false);
                break;
            case 'E':
                assert(!end_pos.has_value());
                end_pos = { i % (width.value() + 1), y };
                walls.push_back(false);
                break;
            case '.':
                walls.push_back(false);
                break;
            case '#':
                walls.push_back(true);
                break;
            default:
                assert(false); // Invalid char
            }
        }
        return { std::move(walls), { width.value(), y }, start_pos.value(), end_pos.value() };
    }

    [[nodiscard]] uint64_t solve_min_points() const
    {
        const std::vector<DijkstraState> grid = dijkstra_final_state();
        // print_dijkstra(grid);
        return 1;
    }

private:
    Maze(std::vector<bool>&& walls, const Vector2i& size, const Vector2i& start_pos, const Vector2i& end_pos)
        : m_walls { std::move(walls) }
        , m_size { size }
        , m_start_pos { start_pos }
        , m_end_pos { end_pos }
    {
    }

    [[nodiscard]] size_t index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    [[nodiscard]] size_t d_index(const Vector2i& pos, const Dir dir) const
    {
        return dir_index(dir) * m_size.x * m_size.y + pos.y * m_size.x + pos.x;
    }

    struct DijkstraState {
        Vector2i pos;
        Dir dir;
        bool explored;
        uint64_t min_score;
        std::array<DijkstraState*, 6> prev_states;
    };

    bool dijkstra_impl(std::vector<DijkstraState>& grid, std::vector<DijkstraState*>& queue) const
    {
        DijkstraState* next_state = nullptr;
        uint64_t min_score = std::numeric_limits<uint64_t>::max();
        size_t next_queue_index = 0;
        for (size_t i = 0; i < queue.size(); ++i) {
            if (!queue[i]->explored && queue[i]->min_score <= min_score) {
                next_state = queue[i];
                min_score = queue[i]->min_score;
                next_queue_index = i;
            }
        }
        if (next_state == nullptr) {
            return false;
        }
        auto update_score
            = [next_state](DijkstraState& neighbor_state, const std::optional<Dir> move_dir = std::nullopt) {
                  const uint64_t neighbor_score = next_state->min_score + (move_dir.has_value() ? 1 : 1000);
                  if (neighbor_score < neighbor_state.min_score) {
                      neighbor_state.min_score = neighbor_score;
                  }
                  if (neighbor_score == neighbor_state.min_score) {
                      neighbor_state.prev_states = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
                  }
                  if (neighbor_score <= neighbor_state.min_score) {
                      if (move_dir.has_value()) {
                          neighbor_state.prev_states[dir_index(move_dir.value())] = next_state;
                      }
                      else {
                          neighbor_state.prev_states[4] = next_state;
                      }
                  }
              };
        for (const Dir dir : rotate_dirs(next_state->dir)) {
            DijkstraState& neighbor_state = grid[d_index(next_state->pos, dir)];
            update_score(neighbor_state);
        }
        const Vector2i neighbor_pos = next_state->pos + dir_offset(next_state->dir);
        DijkstraState& neighbor_state = grid[d_index(neighbor_pos, next_state->dir)];
        if (!m_walls[index(neighbor_pos)]) {
            update_score(neighbor_state, next_state->dir);
        }
        next_state->explored = true;
        queue.erase(queue.begin() + static_cast<int64_t>(next_queue_index));
        return true;
    }

    [[nodiscard]] std::vector<DijkstraState> dijkstra_final_state() const
    {
        std::vector<DijkstraState> grid;
        grid.reserve(m_size.x * m_size.y * 4);
        for (const Dir dir : dirs) {
            for (int y = 0; y < m_size.y; ++y) {
                for (int x = 0; x < m_size.x; ++x) {
                    grid.push_back(
                        { .pos = { x, y },
                          .dir = dir,
                          .explored = false,
                          .min_score = std::numeric_limits<uint64_t>::max(),
                          .prev_states = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } });
                }
            }
        }
        DijkstraState& start_state = grid[d_index(m_start_pos, Dir::east)];
        start_state.min_score = 0;
        std::vector<DijkstraState*> queue;
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (!m_walls[index({ x, y })]) {
                    for (const Dir dir : dirs) {
                        queue.push_back(&grid[d_index({ x, y }, dir)]);
                    }
                }
            }
        }
        while (dijkstra_impl(grid, queue)) { }
        return grid;
    }

    // [[nodiscard]] uint64_t best_paths_grid_count(const std::vector<DijkstraState>& grid) const
    // {
    //     if (score > grid[index(m_end_pos)].min_score) {
    //         return 0;
    //     }
    //     if (pos == m_end_pos) {
    //         return 1;
    //     }
    //     uint64_t count = 0;
    //     for (constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west }; const Dir dir : dirs) {
    //         const Vector2i neighbor_pos = pos + dir_offset(dir);
    //         const size_t neighbor_index = index(neighbor_pos);
    //         if (std::optional<Dir> neighbor_dir = grid[neighbor_index].dir;
    //             neighbor_dir.has_value() && neighbor_dir.value() != opposite_dir(dir)) {
    //             const uint64_t neighbor_count
    //                 = best_paths_grid_count(grid, neighbor_pos, score + (neighbor_dir == dir ? 1 : 1001));
    //             if (neighbor_count != 0) {
    //                 count += 1;
    //             }
    //         }
    //     }
    //     return count;
    // }

    // [[nodiscard]] uint64_t best_paths_grid_count(const std::vector<DijkstraState>& grid) const
    // {
    //     std::vector<bool> visited;
    //     visited.resize(m_size.x * m_size.y, false);
    //     std::vector<Vector2i> queue;
    //     queue.push_back(m_end_pos + dir_offset(opposite_dir(grid[index(m_end_pos)].dir.value())));
    //     uint64_t count = 1;
    //     std::vector<Vector2i> best_positions;
    //     while (!queue.empty()) {
    //         ++count;
    //         const Vector2i pos = queue[queue.size() - 1];
    //         best_positions.push_back(pos);
    //         visited[index(pos)] = true;
    //         queue.pop_back();
    //         for (constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west }; const Dir dir : dirs) {
    //             const Vector2i neighbor_pos = pos + dir_offset(dir);
    //             const size_t neighbor_index = index(neighbor_pos);
    //             if (visited[neighbor_index]) {
    //                 continue;
    //             }
    //             if (std::optional<Dir> neighbor_dir = grid[neighbor_index].dir;
    //                 neighbor_dir.has_value() && neighbor_dir.value() != dir) {
    //                 queue.push_back(neighbor_pos);
    //             }
    //         }
    //     }
    //     // print_dijkstra(grid);
    //     // for (int y = 0; y < m_size.y; ++y) {
    //     //     for (int x = 0; x < m_size.x; ++x) {
    //     //         if (const size_t i = index({ x, y }); m_walls[i]) {
    //     //             std::cout << "#";
    //     //         }
    //     //         else if (std::ranges::find(best_positions, Vector2i { x, y }) != best_positions.end()) {
    //     //             std::cout << "O";
    //     //         }
    //     //         else {
    //     //             std::cout << ".";
    //     //         }
    //     //     }
    //     //     std::cout << std::endl;
    //     // }
    //     return count;
    // }

    std::vector<bool> m_walls;
    Vector2i m_size;
    Vector2i m_start_pos;
    Vector2i m_end_pos;
};

static uint64_t solve(const std::string& data)
{
    const Maze maze = Maze::parse(data);
    return maze.solve_min_points();
}

int main()
{
    const std::string data = read_data("./day16-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 10;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile uint64_t result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data));
#endif
}