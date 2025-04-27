#ifndef MANO_ASSEMBLER_HPP
#define MANO_ASSEMBLER_HPP

#include <cctype>
#include <cstdint>
#include <charconv>
#include <cstdio>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "emulator/emulator.hpp"

namespace mano {

class Assembler {
  public:
    std::optional<Emulator> assemble(const std::string_view code_str);

    const auto& get_errors() {
        return errors;
    }

    struct Error {
        std::string message;
        std::size_t line;
    };

  private:

    std::optional<std::string_view> get_next_token();

    template<typename IntegerType>
    std::optional<IntegerType> get_next_int(int base, IntegerType max = std::numeric_limits<IntegerType>::max()) {
        if (auto token_optional = get_next_token()) {
            auto token = *token_optional;
            IntegerType value = 0;
            auto result =
                std::from_chars(token.data(), token.data() + token.size(), value, base);

            if (value > max) {
                add_error("Integer operand is out of bounds, max: {}", max);
                return {};
            }

            if (result.ec == std::errc {}) {
                return value;
            } else {
                add_error("Could not parse the integer: {}", std::make_error_code(result.ec).message());
            }
        } else {
            add_error("Expected an integer after the instruction.");
        }

        return {};
    }

    bool is_token_label();
    bool line_end();
    
    template<typename ...Args>
    void add_error(std::format_string<Args...> str, Args&&... args) {
        errors.emplace_back(std::format(str, (std::forward<Args>(args))...),  current_line);
    }

    bool first_pass();
    bool second_pass();

  private:
    Memory memory;

    std::string_view code;
    std::size_t index;

    std::unordered_map<std::string_view, std::uint16_t> symbol_table;
    std::vector<Error> errors;

    std::size_t current_line;
    std::uint16_t lc;
};

} // namespace mano

#endif
