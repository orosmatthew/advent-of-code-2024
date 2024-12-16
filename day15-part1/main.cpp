#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
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

enum class Dir { dir_north, dir_east, dir_south, dir_west };

static Vector2i dir_offset(const Dir dir)
{
    switch (dir) {
    case Dir::dir_north:
        return { 0, -1 };
    case Dir::dir_east:
        return { 1, 0 };
    case Dir::dir_south:
        return { 0, 1 };
    case Dir::dir_west:
        return { -1, 0 };
    }
    std::unreachable();
}

class Warehouse {
public:
    static Warehouse parse(const std::string& data)
    {
        int pos = 0;
        std::vector<GridState> grid;
        Vector2i grid_size {};
        Vector2i robot_pos {};
        parse_grid(data, pos, grid, grid_size, robot_pos);
        pos += 2; // "\n\n"
        std::vector<Dir> moves = parse_moves(data, pos);
        return { std::move(grid), grid_size, robot_pos, std::move(moves) };
    }

    uint64_t gps_sum_after_moves()
    {
        while (move()) { }
        uint64_t result = 0;
        for (int y = 0; y < m_grid_size.y; ++y) {
            for (int x = 0; x < m_grid_size.x; ++x) {
                if (m_grid[grid_index({ x, y })] == GridState::box) {
                    result += x + y * 100;
                }
            }
        }
        return result;
    }

    [[maybe_unused]] void print() const
    {
        std::cout << "\n";
        for (int y = 0; y < m_grid_size.y; ++y) {
            for (int x = 0; x < m_grid_size.x; ++x) {
                if (Vector2i { x, y } == m_robot_pos) {
                    std::cout << "@";
                    continue;
                }
                switch (const GridState state = m_grid[grid_index({ x, y })]) {
                case GridState::empty:
                    std::cout << ".";
                    break;
                case GridState::wall:
                    std::cout << "#";
                    break;
                case GridState::box:
                    std::cout << "O";
                    break;
                }
            }
            std::cout << "\n";
        }
    }

private:
    enum class GridState { empty, wall, box };

    static void parse_grid(
        const std::string& data, int& pos, std::vector<GridState>& grid, Vector2i& grid_size, Vector2i& robot_pos)
    {
        grid.clear();
        std::optional<int> width;
        int y = 0;
        while (!(data[pos] == '\n' && data[pos + 1] == '\n')) {
            if (data[pos] == '\n') {
                if (!width.has_value()) {
                    width = pos;
                }
                ++y;
            }
            else {
                switch (data[pos]) {
                case '#':
                    grid.push_back(GridState::wall);
                    break;
                case '.':
                    grid.push_back(GridState::empty);
                    break;
                case 'O':
                    grid.push_back(GridState::box);
                    break;
                case '@':
                    robot_pos = { (pos % (width.value() + 1)), y };
                    grid.push_back(GridState::empty);
                    break;
                default:
                    assert(false); // Invalid char
                }
            }
            ++pos;
        }
        grid_size = { width.value(), ++y };
    }

    static std::vector<Dir> parse_moves(const std::string& data, int& pos)
    {
        std::vector<Dir> moves;
        for (; pos < data.size(); ++pos) {
            if (data[pos] == '\n') {
                continue;
            }
            switch (data[pos]) {
            case '^':
                moves.push_back(Dir::dir_north);
                break;
            case '>':
                moves.push_back(Dir::dir_east);
                break;
            case 'v':
                moves.push_back(Dir::dir_south);
                break;
            case '<':
                moves.push_back(Dir::dir_west);
                break;
            default:
                assert(false);
            }
        }
        return moves;
    }

    bool move()
    {
        if (m_move_index >= m_moves.size()) {
            return false;
        }
        const Dir dir = m_moves[m_move_index++];
        const Vector2i offset = dir_offset(dir);
        const Vector2i next_pos = m_robot_pos + offset;
        const size_t next_index = grid_index(next_pos);
        const GridState next_state = m_grid[next_index];
        if (next_state == GridState::empty) {
            m_robot_pos = next_pos;
            return true;
        }
        if (next_state == GridState::wall) {
            return true;
        }
        Vector2i last_box_pos = next_pos;
        while (true) {
            const Vector2i next_check_pos = last_box_pos + offset;
            const GridState next_check_state = m_grid[grid_index(next_check_pos)];
            if (next_check_state == GridState::wall) {
                return true;
            }
            if (next_check_state == GridState::empty) {
                break;
            }
            last_box_pos = next_check_pos;
        }
        m_grid[grid_index(last_box_pos + offset)] = GridState::box;
        m_grid[next_index] = GridState::empty;
        m_robot_pos += offset;
        return true;
    }

    Warehouse(
        std::vector<GridState>&& grid, const Vector2i& grid_size, const Vector2i& robot_pos, std::vector<Dir>&& moves)
        : m_grid { std::move(grid) }
        , m_grid_size { grid_size }
        , m_robot_pos { robot_pos }
        , m_moves { std::move(moves) }
    {
    }

    [[nodiscard]] size_t grid_index(const Vector2i& pos) const
    {
        return pos.y * m_grid_size.x + pos.x;
    }

    std::vector<GridState> m_grid;
    Vector2i m_grid_size;
    Vector2i m_robot_pos;
    std::vector<Dir> m_moves;
    size_t m_move_index = 0;
};

static uint64_t solve(const std::string& data)
{
    Warehouse warehouse = Warehouse::parse(data);
    return warehouse.gps_sum_after_moves();
}

int main()
{
    const std::string data = read_data("./day15-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 10000;
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