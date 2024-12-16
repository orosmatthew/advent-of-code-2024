#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
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

enum class Dir { north, east, south, west };

[[maybe_unused]] std::string dir_str(const Dir dir)
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
                if (at({ x, y }) == GridState::box_start) {
                    result += x + y * 100;
                }
            }
        }
        return result;
    }

    [[maybe_unused]] void print() const
    {
        std::cout << "\n";
        if (m_move_index != 0) {
            std::cout << "Index: " << m_move_index - 1 << ", Move: " << dir_str(m_moves[m_move_index - 1]) << "\n";
        }
        for (int y = 0; y < m_grid_size.y; ++y) {
            for (int x = 0; x < m_grid_size.x; ++x) {
                if (Vector2i { x, y } == m_robot_pos) {
                    std::cout << "@";
                    continue;
                }
                switch (at({ x, y })) {
                case GridState::empty:
                    std::cout << ".";
                    break;
                case GridState::wall:
                    std::cout << "#";
                    break;
                case GridState::box_start:
                    std::cout << "[";
                    break;
                case GridState::box_end:
                    std::cout << "]";
                    break;
                }
            }
            std::cout << std::endl;
        }
    }

private:
    enum class GridState { empty, wall, box_start, box_end };

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
                    grid.push_back(GridState::wall);
                    break;
                case '.':
                    grid.push_back(GridState::empty);
                    grid.push_back(GridState::empty);
                    break;
                case 'O':
                    grid.push_back(GridState::box_start);
                    grid.push_back(GridState::box_end);
                    break;
                case '@':
                    robot_pos = { (pos % (width.value() + 1)) * 2, y };
                    grid.push_back(GridState::empty);
                    grid.push_back(GridState::empty);
                    break;
                default:
                    assert(false); // Invalid char
                }
            }
            ++pos;
        }
        grid_size = { width.value() * 2, ++y };
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
                moves.push_back(Dir::north);
                break;
            case '>':
                moves.push_back(Dir::east);
                break;
            case 'v':
                moves.push_back(Dir::south);
                break;
            case '<':
                moves.push_back(Dir::west);
                break;
            default:
                assert(false);
            }
        }
        return moves;
    }

    [[nodiscard]] GridState at(const Vector2i& pos) const
    {
        return m_grid[grid_index(pos)];
    }

    [[nodiscard]] GridState& at(const Vector2i& pos)
    {
        return m_grid[grid_index(pos)];
    }

    bool move()
    {
        if (m_move_index >= m_moves.size()) {
            return false;
        }
        const Dir dir = m_moves[m_move_index++];
        const Vector2i offset = dir_offset(dir);
        const Vector2i next_pos = m_robot_pos + offset;
        const GridState next_state = at(next_pos);
        if (next_state == GridState::empty) {
            m_robot_pos = next_pos;
            return true;
        }
        if (next_state == GridState::wall) {
            return true;
        }
        if (!can_box_move(next_pos, dir)) {
            return true;
        }
        move_box(next_pos, dir);
        m_robot_pos = next_pos;
        return true;
    }

    [[nodiscard]] bool can_box_move(Vector2i box_pos, const Dir dir) const // NOLINT(*-no-recursion)
    {
        assert(at(box_pos) == GridState::box_start || at(box_pos) == GridState::box_end);
        if (at(box_pos) == GridState::box_end) {
            box_pos -= Vector2i { 1, 0 };
        }
        if (dir == Dir::east || dir == Dir::west) {
            const Vector2i next_pos = box_pos + (dir == Dir::east ? Vector2i { 2, 0 } : Vector2i { -1, 0 });
            const GridState next_state = at(next_pos);
            if (next_state == GridState::empty) {
                return true;
            }
            if (next_state == GridState::wall) {
                return false;
            }
            return can_box_move(next_pos, dir);
        }
        const Vector2i offset = dir_offset(dir);
        const std::pair next_positions { box_pos + offset, box_pos + Vector2i { 1, 0 } + offset };
        const std::pair next_states { at(next_positions.first), at(next_positions.second) };
        if (next_states.first == GridState::empty && next_states.second == GridState::empty) {
            return true;
        }
        if (next_states.first == GridState::wall || next_states.second == GridState::wall) {
            return false;
        }
        if (next_states.first == GridState::box_start) {
            return can_box_move(next_positions.first, dir);
        }
        const std::pair move_results
            = { next_states.first == GridState::box_end ? can_box_move(next_positions.first, dir) : true,
                next_states.second == GridState::box_start ? can_box_move(next_positions.second, dir) : true };
        return move_results.first && move_results.second;
    }

    void move_box(Vector2i box_pos, const Dir dir) // NOLINT(*-no-recursion)
    {
        assert(at(box_pos) == GridState::box_start || at(box_pos) == GridState::box_end);
        if (at(box_pos) == GridState::box_end) {
            box_pos -= Vector2i { 1, 0 };
        }
        const Vector2i offset = dir_offset(dir);
        auto move_self = [this, box_pos, offset, dir] {
            at(box_pos + offset) = GridState::box_start;
            at(box_pos + Vector2i { 1, 0 } + offset) = GridState::box_end;
            if (dir == Dir::east) {
                at(box_pos) = GridState::empty;
            }
            else if (dir == Dir::west) {
                at(box_pos - offset) = GridState::empty;
            }
            else {
                at(box_pos) = GridState::empty;
                at(box_pos + Vector2i { 1, 0 }) = GridState::empty;
            }
        };
        if (dir == Dir::east || dir == Dir::west) {
            const Vector2i next_pos = box_pos + (dir == Dir::east ? Vector2i { 2, 0 } : Vector2i { -1, 0 });
            const GridState next_state = at(next_pos);
            assert(next_state != GridState::wall);
            if (next_state == GridState::box_start || next_state == GridState::box_end) {
                move_box(next_pos, dir);
            }
            assert(
                (dir == Dir::east ? at(box_pos + Vector2i { 1, 0 } + offset) : at(box_pos + offset))
                == GridState::empty);
            move_self();
            return;
        }
        const std::pair next_positions { box_pos + offset, box_pos + Vector2i { 1, 0 } + offset };
        const std::pair next_states { at(next_positions.first), at(next_positions.second) };
        assert(next_states.first != GridState::wall && next_states.second != GridState::wall);
        if (next_states.first == GridState::empty && next_states.second == GridState::empty) {
            move_self();
            return;
        }
        if (next_states.first == GridState::box_start) {
            move_box(next_positions.first, dir);
            assert(
                at(box_pos + offset) == GridState::empty
                && at(box_pos + Vector2i { 1, 0 } + offset) == GridState::empty);
            move_self();
            return;
        }
        if (next_states.first == GridState::box_end) {
            move_box(next_positions.first, dir);
        }
        if (next_states.second == GridState::box_start) {
            move_box(next_positions.second, dir);
        }
        assert(
            at(box_pos + offset) == GridState::empty && at(box_pos + Vector2i { 1, 0 } + offset) == GridState::empty);
        move_self();
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
    const std::string data = read_data("./day15-part2/input.txt");

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