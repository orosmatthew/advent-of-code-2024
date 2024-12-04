#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
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
    int x;
    int y;

    Vector2i& operator+=(const Vector2i& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

class WordSearch {
public:
    static WordSearch parse(const std::string& string)
    {
        std::optional<int> width;
        std::vector<char> board;
        int pos = 0;
        while (pos < string.length()) {
            while (string[pos] != '\n') {
                board.push_back(string[pos]);
                ++pos;
            }
            if (!width.has_value()) {
                width = pos;
            }
            ++pos;
        }
        return WordSearch { width.value(), std::move(board) };
    }

    [[nodiscard]] int search_word_count(const std::string& word) const
    {
        constexpr std::array dirs { Dir::north, Dir::northeast, Dir::east, Dir::southeast,
                                    Dir::south, Dir::southwest, Dir::west, Dir::northwest };
        int count = 0;
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (at({ x, y }).value() != word[0]) {
                    continue;
                }
                for (const Dir dir : dirs) {
                    if (word_at(word, { x, y }, dir)) {
                        ++count;
                    }
                }
            }
        }
        return count;
    }

private:
    enum class Dir { north, northeast, east, southeast, south, southwest, west, northwest };

    WordSearch(const int width, std::vector<char> board)
        : m_board { std::move(board) }
        , m_size { width, static_cast<int>(m_board.size()) / width }
    {
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    [[nodiscard]] std::optional<char> at(const Vector2i& pos) const
    {
        if (!in_bounds(pos)) {
            return std::nullopt;
        }
        return m_board[index(pos)];
    }

    [[nodiscard]] int index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    static Vector2i word_dir_offset(const Dir dir)
    {
        switch (dir) {
        case Dir::north:
            return { 0, -1 };
        case Dir::northeast:
            return { 1, -1 };
        case Dir::east:
            return { 1, 0 };
        case Dir::southeast:
            return { 1, 1 };
        case Dir::south:
            return { 0, 1 };
        case Dir::southwest:
            return { -1, 1 };
        case Dir::west:
            return { -1, 0 };
        case Dir::northwest:
            return { -1, -1 };
        }
        std::unreachable();
    }

    [[nodiscard]] bool word_at(const std::string& word, const Vector2i& pos, const Dir dir) const
    {
        const Vector2i offset = word_dir_offset(dir);
        Vector2i current = pos;
        for (int i = 0; i < word.length(); ++i) {
            if (std::optional<char> c = at(current); !c.has_value() || word[i] != c.value()) {
                return false;
            }
            current += offset;
        }
        return true;
    }

    std::vector<char> m_board;
    Vector2i m_size;
};

static int solve(const std::string& data)
{
    const WordSearch search = WordSearch::parse(data);
    return search.search_word_count("XMAS");
}

int main()
{
    const std::string data = read_data("./day4-part1/input.txt");

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