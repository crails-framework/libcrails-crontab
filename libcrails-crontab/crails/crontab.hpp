#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
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

    typedef std::vector<Task>::iterator iterator;
    typedef std::vector<Task>::const_iterator const_iterator;

    Crontab();

    void load();
    bool save();
    bool destroy();
    void load_from_string(const std::string_view);
    std::string save_to_string() const;

    std::optional<Task> get_task(const std::string_view name) const;
    std::optional<std::string> get_variable(const std::string_view name) const;
    void set_task(const std::string_view name, const std::string_view schedule, const std::string_view command);
    void remove_task(const std::string_view name);
    void set_variable(const std::string_view name, const std::string_view value);
    void remove_variable(const std::string_view name);

    std::vector<Task>::iterator begin() { return tasks.begin(); }
    std::vector<Task>::iterator end() { return tasks.end(); }
    std::vector<Task>::iterator erase(std::vector<Task>::iterator it) { return tasks.erase(it); }
    std::vector<Task>::const_iterator cbegin() const { return std::cbegin(tasks); }
    std::vector<Task>::const_iterator cend() const { return std::cend(tasks); }

  private:
    std::map<std::string,std::string> variables;
    std::vector<Task> tasks;
  };
}
