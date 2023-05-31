#include <string>
#include <iostream>
#include <list>
#include <sstream>
#include <stack>

struct Command
{
public:
    int counter = 0;

    enum Mode
    {
        single,
        grouped
    };

    Command(std::string name, Mode mode = Mode::single)
    {
        this->name = name;
        this->mode = mode;
    }

    Command(const Command &command)
    {
        this->name = command.name;
        this->mode = command.mode;
        this->subcommands = std::list<Command>(command.subcommands);
    }

    Command(Command &&command)
    {
        this->name = command.name;
        this->mode = command.mode;
        this->subcommands = std::list<Command>(command.subcommands);
    }

    std::string get_name()
    {
        return name;
    }
    
    Command& get_last()
    {
        return subcommands.back();
    }

    bool is_group_command() const
    {
        return mode == Mode::grouped;
    }

    bool is_group_nested() const
    {
        return mode == Mode::grouped &&
               subcommands.size() > 0 &&
               subcommands.back().is_group_command();
    }

    void append_command(const Command &command)
    {
        subcommands.push_back(command);
    }

    std::ostringstream operator()() const
    {
        std::ostringstream out;
        if (subcommands.size() == 0)
            out << name;
        else
        {
            out << (*subcommands.begin()).name;

            for (auto it = std::next(subcommands.begin()); it != subcommands.end(); ++it)
            {
                if (!(*it).is_group_command())
                    out << ", " << (*it).name;
                else
                    out << ", " << (*it)().str();
            }
        }
        return out;
    };

    bool operator<(const Command &command) const
    {
        return this->name < command.name &&
               this->subcommands < command.subcommands;
    }

    bool operator==(const Command &command) const
    {
        return !(*this < command) && !(command < *this);
    }

    bool operator!=(const Command &command) const
    {
        return *this < command || command < *this;
    }

private:
    std::string name;
    Mode mode;
    std::list<Command> subcommands;
};

class Command_worker
{
public:
    Command_worker(const size_t bulk_size) : bulk_size(bulk_size)
    {
    }

    template <class CharType, class TraitsType>
    void perform_reading(std::basic_istream<CharType, TraitsType> &stream)
    {
        std::string line;

        while (std::getline(stream, line))
        {
            if (line == "\n" || line == "")
            {
                if (commands.size() != 0 && !commands.back().is_group_command())
                {
                    perform_if_needed(true);
                }
                break;
            }

            if (line == "{") // Start embedded command
            {
                start_embedded_command();
            }
            else if (line == "}") // Finish embedded or ignore if N-times embedded command
            {
                finish_embedded_command_if_needed();
            }
            else // Append common or embedded command
            {
                Command* new_command = new Command(line);
                if (commands.size() != 0 && commands.back().is_group_command())
                {
                    add_to_current_group_command(*new_command);
                }
                else
                {
                    add_command(*new_command);
                }
            }
        }
    }

private:
    const size_t bulk_size;
    size_t bulk_counter = 0;
    std::list<Command> commands;
    Command *current_group_command = nullptr;

    void start_embedded_command()
    {
        // execute previously stored commands if non grouped
        if (commands.size() != 0 && !commands.back().is_group_command())
        {
            perform_if_needed(true);
        }

        std::string uuid_str("grouped_" + std::to_string(std::rand()));
        Command *command = new Command(uuid_str, Command::Mode::grouped);
        add_group_command(*command);
    }

    void finish_embedded_command_if_needed()
    {
        if (!commands.back().is_group_command())
        {
            return;
        }

        if (commands.back().is_group_nested())
        {
            current_group_command = nullptr;
            current_group_command = &commands.back();
        }
        else
        {
            perform_if_needed(true);
            current_group_command = nullptr;
        }
    }

    void add_command(const Command &command)
    {
        commands.push_back(command);
        bulk_counter++;
        perform_if_needed();
    }

    void add_group_command(const Command &command)
    {
        if (!command.is_group_command())
        {
            return;
        }

        if (current_group_command == nullptr)
        {
            commands.push_back(command);
            current_group_command = &commands.back();
        }
        else
        {
            add_to_current_group_command(command);
            current_group_command = &current_group_command->get_last();
        }
    }

    void add_to_current_group_command(const Command &command)
    {
        if (!current_group_command->is_group_command())
            return;
        current_group_command->append_command(command);
    }

    void perform_if_needed(const bool force = false)
    {
        if (bulk_counter == bulk_size || force)
        {
            (*this)();
            bulk_counter = 0;
            commands.clear();
        }
    }

    void operator()() const
    {
        std::ostringstream out;
        if (!commands.back().is_group_command())
        {
            if (commands.size() == 0)
                return;

            out << "bulk: " << (*commands.begin())().str();

            for (auto it = std::next(commands.begin(), 1); it != commands.end(); ++it)
            {
                out << ", " << (*it)().str();
            }
        }
        else
        {
            out << "bulk: " << commands.back()().str();
        }
        std::cout << out.str() << std::endl;
    }
};

int main()
{
    Command_worker command_worker(3);
    command_worker.perform_reading(std::cin);
    return 0;
}
