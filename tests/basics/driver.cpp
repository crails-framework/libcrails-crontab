#include <sstream>
#include <stdexcept>
#include <iostream>

#include <crails/crontab.hpp>

#undef NDEBUG
#include <cassert>

const std::string_view example_1(
  "* * * * * /bin/bash script.sh \n"
  "* * * * * ls -lah#name=task2\n"
  "* 30 16,18 * * pg_dump tintin #name=task3"
);

const std::string_view example_2(
  "LD_LIBRARY_PATH=/usr/local/lib\n"
  "\n"
  "QUOTED_VAR1=\"this value is within \\\"quotes\\\"\"\n"
  "QUOTED_VAR2='single \\'quotes\\''\n"
  "\n"
  "0 * * * * ls -lah #name=task1\n"
);

int main ()
{
  using namespace std;
  using namespace Crails;

  // Loads.
  //
  {
    Crontab crontab;
    optional<Crontab::Task> task1, task2, task3;

    crontab.load_from_string(example_1);
    assert((task1 = crontab.get_task("")).has_value());
    assert((task2 = crontab.get_task("task2")).has_value());
    assert((task3 = crontab.get_task("task3")).has_value());
    assert(task1->schedule == "* * * * *");
    assert(task1->command == "/bin/bash script.sh");
    assert(task2->schedule == "* * * * *");
    assert(task3->schedule == "* 30 16,18 * *");
    assert(task2->command == "ls -lah");
    assert(task3->command == "pg_dump tintin");
  }

  // Saves.
  //
  {
    Crontab crontab, crontab_load;
    string output;

    crontab.set_task("task1", "* * * * *", "systemctl stop service");
    crontab.set_task("task2", "1 2 3 4 5", "halt");
    crontab.set_task("task3", "* 30 15 * *", "echo \"tintin\"");
    output = crontab.save_to_string();
    assert(output.find("* * * * * systemctl stop service") == 0);
    crontab_load.load_from_string(output);
    assert(crontab_load.get_task("task1").has_value());
    assert(crontab_load.get_task("task2").has_value());
    assert(crontab_load.get_task("task3").has_value());
    assert(crontab_load.get_task("task3")->command == "echo \"tintin\"");
  }

  // Loads variables.
  //
  {
    Crontab crontab, crontab_load;
    optional<string> variable1, variable2, variable3;

    crontab.load_from_string(example_2);
    assert((variable1 = crontab.get_variable("LD_LIBRARY_PATH")).has_value());
    assert((variable2 = crontab.get_variable("QUOTED_VAR1")).has_value());
    assert((variable3 = crontab.get_variable("QUOTED_VAR2")).has_value());
    assert(*variable1 == "/usr/local/lib");
    assert(*variable2 == "this value is within \"quotes\"");
    assert(*variable3 == "single 'quotes'");
    crontab.set_variable("LD_LIBRARY_PATH", "/opt/lib");
    crontab.set_variable("NEW_VAR", "newval");
    crontab.remove_variable("QUOTED_VAR2");
    crontab_load.load_from_string(crontab.save_to_string());
    assert((variable1 = crontab.get_variable("LD_LIBRARY_PATH")).has_value());
    assert((variable2 = crontab.get_variable("QUOTED_VARS1")).has_value());
    assert(!(variable3 = crontab.get_variable("QUOTED_VARS2")).has_value());
    assert((variable3 = crontab.get_variable("NEW_VAR")).has_value());
    assert(*variable1 == "/usr/local/lib");
    assert(*variable2 == "this value is within \"quotes\"");
    assert(*variable3 == "newval");
  }

  // Iterates.
  //
  {
    Crontab crontab;
    optional<Crontab::Task> task1, task2, task3;

    crontab.load_from_string(example_1);
    for (Crontab::Task& task : crontab)
      task.schedule = "0 * * * *";
    assert((task1 = crontab.get_task("")).has_value());
    assert((task2 = crontab.get_task("task2")).has_value());
    assert((task3 = crontab.get_task("task3")).has_value());
    assert(task1->schedule == "0 * * * *");
    assert(task2->schedule == "0 * * * *");
    assert(task3->schedule == "0 * * * *");
  }

  return 0;
}
