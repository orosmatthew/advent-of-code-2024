#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <ranges>
#include <span>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static uint64_t parse_int(const std::string& string, int& pos)
{
    uint64_t result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
}

class Computer {
public:
    struct Registers {
        uint64_t a;
        uint64_t b;
        uint64_t c;
    };

    static Computer parse(const std::string& data)
    {
        int pos = 0;
        pos += 12; // "Register A: "
        Registers registers {};
        registers.a = parse_int(data, pos);
        pos += 13; // "\nRegister B: "
        registers.b = parse_int(data, pos);
        pos += 13; // "\nRegister C: "
        registers.c = parse_int(data, pos);
        pos += 11; // "\n\nProgram: "
        std::vector<uint64_t> program;
        do {
            program.push_back(parse_int(data, pos));
            ++pos; // ","
            program.push_back(parse_int(data, pos));
            ++pos; // "," or "\n"
        } while (pos < data.size());
        std::vector<Instruction> instructions;
        return { registers, program_to_instructions(program) };
    }

    static Computer from_registers_program(Registers registers, const std::span<const uint64_t> program)
    {
        return { registers, program_to_instructions(program) };
    }

    void replace_registers_program_and_reset(const Registers& registers, const std::span<const uint64_t> program)
    {
        m_registers = registers;
        m_instructions = program_to_instructions(program);
        m_instruction_pointer = 0;
        m_output.clear();
        m_ran = false;
    }

    [[nodiscard]] std::span<const uint64_t> output() const
    {
        return m_output;
    }

    std::string output_str()
    {
        run();
        std::string str;
        for (const uint64_t value : m_output) {
            str.append(std::to_string(value));
            str.append(",");
        }
        str.pop_back();
        return str;
    }

private:
    enum class RegisterType { a, b, c };

    enum class InstructionType { adv, bxl, bst, jnz, bxc, out, bdv, cdv };

    static InstructionType opcode_to_instruction_type(const uint64_t opcode)
    {
        assert(opcode <= 8);
        constexpr std::array types { InstructionType::adv, InstructionType::bxl, InstructionType::bst,
                                     InstructionType::jnz, InstructionType::bxc, InstructionType::out,
                                     InstructionType::bdv, InstructionType::cdv };
        return types[opcode];
    }

    enum class OperandType { literal, combo, ignore };

    static OperandType instruction_operand_type(const InstructionType type)
    {
        switch (type) {
        case InstructionType::bxl:
        case InstructionType::jnz:
            return OperandType::literal;
        case InstructionType::adv:
        case InstructionType::bst:
        case InstructionType::out:
        case InstructionType::bdv:
        case InstructionType::cdv:
            return OperandType::combo;
        case InstructionType::bxc:
            return OperandType::ignore;
        }
        std::unreachable();
    }

    using Operand = std::variant<uint64_t, RegisterType>;

    static Operand parse_combo_operand(const uint64_t operand)
    {
        assert(operand <= 6);
        switch (operand) {
        case 0:
        case 1:
        case 2:
        case 3:
            return operand;
        case 4:
            return RegisterType::a;
        case 5:
            return RegisterType::b;
        case 6:
            return RegisterType::c;
        default:
            assert(false);
        }
        std::unreachable();
    }

    struct Instruction {
        InstructionType type {};
        std::optional<Operand> operand;
    };

    static std::vector<Instruction> program_to_instructions(const std::span<const uint64_t> program)
    {
        assert(program.size() % 2 == 0);
        std::vector<Instruction> instructions;
        for (int i = 0; i < program.size(); i += 2) {
            Instruction instruction;
            instruction.type = opcode_to_instruction_type(program[i]);
            uint64_t operand = program[i + 1];
            switch (instruction_operand_type(instruction.type)) {
            case OperandType::literal:
                instruction.operand = operand;
                break;
            case OperandType::combo:
                instruction.operand = parse_combo_operand(operand);
                break;
            case OperandType::ignore:
                instruction.operand = std::nullopt;
                break;
            }
            instructions.push_back(instruction);
        }
        return instructions;
    }

    [[nodiscard]] uint64_t combo_operand_value(const Operand operand) const
    {
        if (const auto* value = std::get_if<uint64_t>(&operand)) {
            return *value;
        }
        switch (std::get<RegisterType>(operand)) {
        case RegisterType::a:
            return m_registers.a;
        case RegisterType::b:
            return m_registers.b;
        case RegisterType::c:
            return m_registers.c;
        }
        std::unreachable();
    }

    [[nodiscard]] std::optional<uint64_t> operand_value(
        const InstructionType type, const std::optional<Operand>& operand) const
    {
        if (!operand.has_value()) {
            return std::nullopt;
        }
        switch (instruction_operand_type(type)) {
        case OperandType::literal:
            return std::get<uint64_t>(operand.value());
        case OperandType::combo:
            return combo_operand_value(operand.value());
        case OperandType::ignore:
            return std::nullopt;
        }
        std::unreachable();
    }

    void run()
    {
        if (m_ran) {
            return;
        }
        while (m_instruction_pointer / 2 < m_instructions.size()) {
            const auto& [type, instruction_operand] = m_instructions[m_instruction_pointer / 2];
            const std::optional<uint64_t> operand = operand_value(type, instruction_operand);
            switch (type) {
            case InstructionType::adv: {
                const auto num = static_cast<double>(m_registers.a);
                const double denom = std::pow(2.0, operand.value());
                m_registers.a = static_cast<uint64_t>(num / denom);
                m_instruction_pointer += 2;
            } break;
            case InstructionType::bxl:
                m_registers.b ^= operand.value();
                m_instruction_pointer += 2;
                break;
            case InstructionType::bst:
                m_registers.b = operand.value() % 8;
                m_instruction_pointer += 2;
                break;
            case InstructionType::jnz:
                if (m_registers.a == 0) {
                    m_instruction_pointer += 2;
                    break;
                }
                assert(operand.value() % 2 == 0);
                m_instruction_pointer = operand.value();
                break;
            case InstructionType::bxc:
                m_registers.b ^= m_registers.c;
                m_instruction_pointer += 2;
                break;
            case InstructionType::out:
                m_output.push_back(operand.value() % 8);
                m_instruction_pointer += 2;
                break;
            case InstructionType::bdv: {
                const auto num = static_cast<double>(m_registers.a);
                const double denom = std::pow(2.0, operand.value());
                m_registers.b = static_cast<uint64_t>(num / denom);
                m_instruction_pointer += 2;
            } break;
            case InstructionType::cdv: {
                const auto num = static_cast<double>(m_registers.a);
                const double denom = std::pow(2.0, operand.value());
                m_registers.c = static_cast<uint64_t>(num / denom);
                m_instruction_pointer += 2;
            } break;
            }
        }
    }

    Computer(const Registers& registers, const std::vector<Instruction>& instructions)
        : m_registers { registers }
        , m_instructions { instructions }
    {
    }

    Registers m_registers;
    std::vector<Instruction> m_instructions;
    uint64_t m_instruction_pointer = 0;
    std::vector<uint64_t> m_output;
    bool m_ran = false;
};

static std::string solve(const std::string& data)
{
    Computer computer = Computer::parse(data);
    return computer.output_str();
}

int main()
{
    const std::string data = read_data("./day17-part2/sample.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile std::string result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%s\n", solve(data).c_str());
#endif
}