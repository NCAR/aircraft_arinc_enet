#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "A429.h"

A429 enet1;

void sighandler(int s)
{
  fprintf(stderr, "SigHandler: signal=%s cleaning up.\n", strsignal(s));
  enet1.Close();
  exit(0);
}

int main(int argc, char *argv[])
{

  signal(SIGINT, sighandler);
  signal(SIGFPE, sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGSEGV, sighandler);


  enet1.Open();
  enet1.Status();
  enet1.CalibrateIRIG();
  enet1.Status();
  enet1.StartChannel(4, 100000);
  enet1.StartChannel(5, 100000);
  enet1.StartChannel(6, 12500);
  enet1.StartChannel(7, 100000);




  while (1)
  {
    enet1.Status();
    sleep(3);
  }
}
