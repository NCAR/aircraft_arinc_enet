#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "A429.h"

A429 enet1;


void processArgs(int argc, char *argv[])
{
  int opt;

  while((opt = getopt(argc, argv, ":i:s:")) != -1)
  {
    switch(opt)
    {
            case 's':
                enet1.setACserverIP(optarg);
                break;
            case 'i':
                enet1.setEnetIP(optarg);
                break;
            case ':':
                printf("option needs a value\n");
                break;
            case '?':
                printf("unknown option: %c\n", optopt);
                break;
    }
  }
}

void sigAction(int sig, siginfo_t* siginfo, void* vptr)
{
  fprintf(stderr, "arinc_ctrl::SigHandler: signal=%s cleaning up.\n", strsignal(sig));
  enet1.Close();
  exit(0);
}

void setupSignals()
{
  // set up a sigaction to respond to ctrl-C
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGHUP);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, (sigset_t*)0);


  struct sigaction act;
  sigemptyset(&sigset);
  act.sa_mask = sigset;
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = sigAction;
  sigaction(SIGHUP, &act, (struct sigaction *)0);
  sigaction(SIGINT, &act, (struct sigaction *)0);
  sigaction(SIGTERM,&act, (struct sigaction *)0);

}


int main(int argc, char *argv[])
{
  processArgs(argc, argv);
  setupSignals();


  enet1.Open();
  if (enet1.isOpen() == false)
    exit(1);

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
