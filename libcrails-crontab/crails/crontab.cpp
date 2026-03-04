#include "crontab.hpp"
#include <boost/version.hpp>
#if BOOST_VERSION >= 108600
# include <boost/process.hpp>
namespace boost_process = boost::process;
#else
# include <boost/process/v2.hpp>
namespace boost_process = boost::process::v2;
#endif
#include <boost/asio.hpp>
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

static string drain_pipe(boost::asio::readable_pipe& pipe)
{
  string result;
  boost::system::error_code ec;
  char buffer[4096];
  size_t n;

  do
  {
    n = pipe.read_some(boost::asio::buffer(buffer), ec);
    if (n > 0)
      result.append(buffer, n);
  } while (!ec && n > 0);
  return result;
}

static void load_from_string(map<string,string>& variables, vector<Crontab::Task>& tasks, const string_view input)
{
  size_t n = 0;

  for (size_t i = 0 ; i < input.length() ; ++i)
  {
    if (input[i] == '\n' || input[i] == '\r')
    {
      regex       variable_pattern("^[a-zA-Z]+[a-zA-Z0-9_]*=");
      string_view part(&(input.data()[n]), i - n);

      if (part.length() > 0 && part[0] != '#')
      {
        if (regex_search(part.begin(), part.end(), variable_pattern))
          variables.insert(read_variable(part));
        else
          tasks.push_back(read_crontask(part));
      }
    }
  }
}

static void load_from_pipe(map<string,string>& variables, vector<Crontab::Task>& tasks, boost::asio::readable_pipe& pipe)
{
  string input = drain_pipe(pipe);

  load_from_string(variables, tasks, input);
}

static void save_to_stream(const map<string,string>& variables, const vector<Crontab::Task>& tasks, std::ostream& stream)
{
  for (const auto& entry : variables)
    stream << entry.first << '=' << quoted(entry.second) << '\n';
  for (const Crontab::Task& task : tasks)
    stream << task_to_string(task) << '\n';
}

static void save_to_pipe(const map<string,string>& variables, const vector<Crontab::Task>& tasks, boost::asio::writable_pipe& pipe)
{
  ostringstream stream;

  save_to_stream(variables, tasks, stream);
  boost::asio::write(pipe, boost::asio::buffer(stream.str()));
}

Crontab::Crontab()
{
}

void Crontab::load()
{
  boost::asio::io_context ios;
  boost::asio::readable_pipe std_out(ios);
  boost_process::process process(
    ios, "/usr/bin/crontab", {"-l"},
    boost_process::process_stdio{nullptr, std_out, {}}
  );

  process.wait();
  load_from_pipe(variables, tasks, std_out);
}

bool Crontab::save()
{
  if (tasks.size() > 0)
  {
    boost::asio::io_context ios;
    boost::asio::readable_pipe sink(ios);
    boost::asio::writable_pipe std_in(ios);
    boost::asio::connect_pipe(sink, std_in);
    boost_process::process process(
      ios, "/usr/bin/crontab", {},
      boost_process::process_stdio{sink, {}, {}}
    );

    save_to_pipe(variables, tasks, std_in);
    std_in.close();
    process.wait();
    return process.exit_code() == 0;
  }
  return destroy();
}

bool Crontab::destroy()
{
  boost::asio::io_context ios;
  boost_process::process process(ios, "/usr/bin/crontab", {"-r"});

  process.wait();
  return process.exit_code() == 0;
}

void Crontab::load_from_string(const string_view source)
{
  ::load_from_string(variables, tasks, source);
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
