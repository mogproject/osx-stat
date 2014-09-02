#include <stdio.h>
#include <unistd.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace std;

class CpuUsage {
 public:
  CpuUsage(int core): core_(core) {
    prev = updated_ticks_(core);
  }

  float get() {
    Ticks t = updated_ticks_(core_);
    unsigned long long int used = t.used() - prev.used();
    unsigned long long int total = t.total() - prev.total();
    prev = t;
    return (float)used / (float)total * 100.0f;
  }

 private:
  struct Ticks {
    unsigned long long int usertime;
    unsigned long long int nicetime;
    unsigned long long int systemtime;
    unsigned long long int idletime;

    unsigned long long int used() { return usertime + nicetime + systemtime; }
    unsigned long long int total() { return usertime + nicetime + systemtime + idletime; }
  } prev;

  int core_;

  Ticks updated_ticks_(int core) {
    unsigned int cpu_count;
    processor_cpu_load_info_t cpu_load;
    mach_msg_type_number_t cpu_msg_count;

    int rc =  host_processor_info(
      mach_host_self( ),
      PROCESSOR_CPU_LOAD_INFO,
      &cpu_count,
      (processor_info_array_t *) &cpu_load,
      &cpu_msg_count
    );
    if (rc != 0) {
      printf("Error: failed to scan processor info (rc=%d)\n", rc);
      exit(1);
    }

    if (core < 0 || cpu_count <= core) {
      printf("Error: invalid core number: %d\n", core);
      exit(1);
    }
    unsigned long long int usertime = cpu_load[core].cpu_ticks[CPU_STATE_USER];
    unsigned long long int nicetime = cpu_load[core].cpu_ticks[CPU_STATE_NICE];
    unsigned long long int systemtime = cpu_load[core].cpu_ticks[CPU_STATE_SYSTEM];
    unsigned long long int idletime = cpu_load[core].cpu_ticks[CPU_STATE_IDLE];

    Ticks t = {usertime, nicetime, systemtime, idletime};
    return t;
  }
};

void usage() {
  printf("sys_stat [CPU_NO]");
  exit(2);
}

char percent_to_char(float percent) {
    int x = (int)(percent / 2.0 + 0.5);
    x = max(0, min(50, x));
    return x <= 25 ? 'a' + x : 'A' + x - 26;
}

int main(int argc, char *argv[]) {
  // parse arguments
  if (argc > 2) usage();
  if (argc == 2) {
    istringstream iss(argv[1]);
    int core;
    if (iss >> core) {
      CpuUsage u(core);
      sleep(1);
      cout << percent_to_char(u.get());
    } else {
      usage();
    }
  }
  return 0;
}
