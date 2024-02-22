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

  return 0;
}
