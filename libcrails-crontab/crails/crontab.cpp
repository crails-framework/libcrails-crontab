#include "crontab.hpp"
#include <boost/process.hpp>
#include <sstream>
#include <regex>
#include <iomanip>

using namespace Crails;
using namespace std;

static pair<string,string> read_variable(const string_view line)
{
  int separator = line.find('=');
  string key(line.substr(0, separator));
  string value(line.substr(separator + 1));

  if (value[0] == '"' || value[0] == '\'')
  {
    istringstream stream(value);
    stream >> quoted(value, value[0]);
  }
  return {key, value};
}

static Crontab::Task read_crontask(const string_view line)
{
  int i = -1;
  int command_begin, command_end;
  int schedule_part_count = 0;
  Crontab::Task task;

  while (++i < line.length())
  {
    if (line[i] == ' ')
    {
      schedule_part_count++;
      if (schedule_part_count == 5)
        break ;
    }
  }
  task.schedule = string(line.substr(0, i));
  command_begin = ++i;
  while (++i < line.length())
  {
    if (line[i] == '#')
    {
      string_view part = line.substr(i);
      if (part.find("#name=") == 0)
      {
        task.name = string(part.substr(6));
        break ;
      }
    }
    else if (line[i] != ' ')
    {
      command_end = i;
    } 
  }
  task.command = string(line.substr(command_begin, ++command_end - command_begin));
  return task;
}

static string task_to_string(const Crontab::Task& task)
{
  string crontask = task.schedule + ' ' + task.command;

  return task.name.length() == 0
    ? crontask
    : (crontask + " #name=" + task.name);
}

template<typename STREAM>
static void load_from_stream(map<string,string>& variables, vector<Crontab::Task>& tasks, STREAM& stream)
{
  string line;

  tasks.clear();
  while (getline(stream, line))
  {
    if (line.length() > 0 && line[0] != '#')
    {
      regex variable_pattern("^[a-zA-Z]+[a-zA-Z0-9_]*=");

      if (regex_search(line, variable_pattern))
        variables.insert(read_variable(line));
      else
        tasks.push_back(read_crontask(line));
    }
  }
}

template<typename STREAM>
static void save_to_stream(const map<string,string>& variables, const vector<Crontab::Task>& tasks, STREAM& stream)
{
  for (const auto& entry : variables)
    stream << entry.first << '=' << quoted(entry.second) << '\n';
  for (const Crontab::Task& task : tasks)
    stream << task_to_string(task) << '\n';
}

Crontab::Crontab()
{
}

void Crontab::load()
{
  boost::process::ipstream stream;
  boost::process::child process("crontab -l", boost::process::std_out > stream);

  load_from_stream(variables, tasks, stream);
  process.wait(); 
}

bool Crontab::save()
{
  if (tasks.size() > 0)
  {
    boost::process::opstream stream;
    boost::process::child process("crontab", boost::process::std_in < stream);

    save_to_stream(variables, tasks, stream);
    process.terminate();
    return process.exit_code() == 0;
  }
  return destroy();
}

bool Crontab::destroy()
{
  boost::process::child process("crontab -r");

  process.wait();
  return process.exit_code() == 0;
}

void Crontab::load_from_string(const string_view source)
{
  istringstream stream(string(source), ios_base::in);

  load_from_stream(variables, tasks, stream);
}

string Crontab::save_to_string() const
{
  ostringstream stream;

  save_to_stream(variables, tasks, stream);
  return stream.str();
}

optional<Crontab::Task> Crontab::get_task(const string_view name) const
{
  auto it = find(tasks.begin(), tasks.end(), name);

  return it != tasks.end() ? optional<Task>(*it) : optional<Task>();
}

void Crontab::set_task(const string_view name, const string_view schedule, const string_view command)
{
  auto it = find(tasks.begin(), tasks.end(), name);

  if (it != tasks.end())
  {
    it->schedule = string(schedule);
    it->command = string(command);
  }
  else
    tasks.push_back({string(name), string(schedule), string(command)});
}

void Crontab::remove_task(const string_view name)
{
  auto it = find(tasks.begin(), tasks.end(), name);

  if (it != tasks.end())
    tasks.erase(it);
}

optional<string> Crontab::get_variable(const string_view name) const
{
  auto it = variables.find(string(name));

  if (it != variables.end())
    return it->second;
  return {};
}

void Crontab::set_variable(const string_view name, const string_view value)
{
  variables.insert_or_assign(string(name), string(value));
}

void Crontab::remove_variable(const string_view name)
{
  auto it = variables.find(string(name));
  
  if (it != variables.end())
    variables.erase(it);
}
