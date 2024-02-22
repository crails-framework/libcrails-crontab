#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace Crails
{
  class Crontab
  {
  public:
    struct Task
    {
      std::string name, schedule, command;
      bool operator==(const std::string_view value) const { return name == value; }
    };

    Crontab();

    void load();
    bool save();
    bool destroy();
    void load_from_string(const std::string_view);
    std::string save_to_string() const;

    std::optional<Task> get_task(const std::string_view name) const;
    void set_task(const std::string_view name, const std::string_view schedule, const std::string_view command);
    void remove_task(const std::string_view name);

  private:
    std::vector<Task> tasks;
  };
}
