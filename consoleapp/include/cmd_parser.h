#include <functional>
#include <stddef.h>
#include <limits>
#include <string>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>
#include <span>
#include <string_view>
#include "expected.h"

namespace cmd
{
    template <class CRTP>
    struct Common {
        std::string_view longname;
        std::optional<char32_t> shortname;
        std::string_view description;

        Common(std::string_view longname) : longname(longname), description()
        {}
        Common(std::string_view longname, char32_t shortname) : longname(longname), shortname(shortname), description()
        {}
        Common(std::string_view longname, std::optional<char32_t> shortname) : longname(longname), shortname(shortname), description()
        {}

        constexpr CRTP& set_longname(std::string_view longname) noexcept {
            this->longname = longname;
            return *this;
        }
        constexpr CRTP& set_shortname(std::string_view shortname) noexcept {
            this->shortname = shortname;
            return *this;
        }
        constexpr CRTP& set_description(std::string_view description) noexcept {
            this->description = description;
            return *this;
        }
    };
    template<class CRTP>
    struct MinMax {
        uint32_t min;
        uint32_t max;
        MinMax() : min(0), max(std::numeric_limits<decltype(this->max)>::max())
        {}
        MinMax(uint32_t min, uint32_t max) : min(min), max(max)
        {}

        constexpr CRTP& set_min(int min) noexcept {
            this->min = min;
            return *this;
        }
        constexpr CRTP& set_max(int max) noexcept {
            this->max = max;
            return *this;
        }
    };

    struct Argument : Common<Argument>, MinMax<Argument> {
        std::optional<std::string_view> metavar;
        std::optional<std::function<bool(std::string_view)>> validator;
        std::optional<std::string_view> default_value;

        Argument(std::string_view longname) : Common(longname), MinMax(), metavar()
        {}
        Argument(std::string_view longname, char32_t shortname) : Common(longname, shortname), MinMax<Argument>(), metavar()
        {}
        Argument(std::string_view longname, std::optional<char32_t> shortname, int min, int max) : Common(longname, shortname), MinMax<Argument>(min, max), metavar()
        {}
        Argument(const Argument&) = default;
        Argument(Argument&&) = default;
        Argument& operator=(const Argument&) = default;
        Argument& operator=(Argument&&) = default;
        ~Argument() = default;
        
        constexpr Argument& set_metavar(std::string_view metavar) noexcept {
            this->metavar = metavar;
            return *this;
        }
        Argument& set_validator(std::function<bool(std::string_view)> validator) noexcept {
            this->validator = validator;
            return *this;
        }
        constexpr Argument& set_default_value(std::string_view default_value) noexcept {
            this->default_value = default_value;
            return *this;
        }
    };
    struct Flag : Common<Flag>, MinMax<Flag> {
        Flag(std::string_view longname) : Common(longname), MinMax()
        {}
        Flag(std::string_view longname, char32_t shortname) : Common(longname, shortname), MinMax<Flag>()
        {}
        Flag(std::string_view longname, std::optional<char32_t> shortname, int min, int max) : Common(longname, shortname), MinMax<Flag>(min, max)
        {}
        Flag(const Flag&) = default;
        Flag(Flag&&) = default;
        Flag& operator=(const Flag&) = default;
        Flag& operator=(Flag&&) = default;
        ~Flag() = default;
    };


    struct Command : Common<Command> {
        std::vector<Argument> arguments;
        std::vector<Flag> flags;
        // std::vector<Command> subcommands;

        Command(std::string_view longname) : Common(longname)
        {}
        Command(std::string_view longname, char32_t shortname) : Common(longname, shortname)
        {}
        Command(const Command&) = default;
        Command(Command&&) = default;
        Command& operator=(const Command&) = default;
        Command& operator=(Command&&) = default;
        ~Command() = default;

        Command& add_argument(Argument argument) {
            arguments.push_back(argument);
            return *this;
        }
        Command& add_flag(Flag flag) {
            flags.push_back(flag);
            return *this;
        }
        Argument& make_argument(std::string_view longname, std::optional<std::string_view> shortname = std::nullopt) {
            return add_argument(Argument(longname)).arguments.back();
        }
        Flag& make_flag(std::string_view longname) {
            return add_flag(Flag(longname)).flags.back();
        }
        // Command& add_subcommand(Command subcommand) {
        //     subcommands.push_back(subcommand);
        //     return *this;
        // }
    };

    namespace result 
    {
        struct Argument {
            std::string_view name;
            std::string_view value;
        };
        struct Flag {
            std::string_view name;
            uint32_t occurrence;
        };
        using Parameter = std::variant<Argument, Flag>;
        struct Command {
            std::string_view name;
            std::vector<Parameter> parameters;
        };
        struct Result {
            std::string_view program;
            Command command;
            std::vector<Parameter> parameters;
        };
        struct Error {
            std::string_view argument;
            std::optional<std::string_view> value;
            enum class Type {
                Argument,
                Flag,
                Command,
                None,
                Unknown
            } type;
            enum class Code {
                MissingArgument,
                MissingCommand,
                MissingFlag,
                NoGlobalCommand,
                BadCommand,
                UnknownName,
                InvalidValue,
                OutOfBound,
                SyntaxError,
                BadString
            } code;
            std::string to_string();
        };
    }

    class Parser {
        std::optional<std::variant<Command, std::string_view>> global_command;
        std::vector<Command> commands;
        std::string_view program_name;
    public:
        template <class T>
        using Expected = expected<T, result::Error>;
        Parser& add_command(Command command) {
            commands.push_back(command);
            return *this;
        }
        Command& make_command(std::string_view longname) {
            return add_command(Command(longname)).commands.back();
        }
        Parser& set_global_command(Command command) {
            global_command = command;
            return *this;
        }
        auto parse(std::span<std::string_view> args) const -> Expected<result::Result>;
    private:
        auto get_global_command() const -> std::optional<std::reference_wrapper<const Command>> {
            if (!global_command.has_value()) 
                return std::nullopt;
            
            if (std::holds_alternative<Command>(global_command.value())) 
                return std::optional{std::ref(std::get<Command>(global_command.value()))};

            for (const auto& command : commands) {
                if (std::get<std::string_view>(global_command.value()) == command.longname) {
                    return std::optional{std::ref(command)};
                }
            }
            return std::nullopt;
        }
        template<class T>
        using PosExpected = Expected<std::tuple<T, std::span<std::string_view>::iterator>>;

        auto parse_argument(std::span<std::string_view> args) const -> PosExpected<result::Argument>;
        auto parse_flag(std::span<std::string_view> args) const -> PosExpected<result::Flag>;
        auto parse_command(std::span<std::string_view> args) const -> PosExpected<result::Command>;
    };
}