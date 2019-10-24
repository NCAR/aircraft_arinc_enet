
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "A429.h"
#include "ChannelInfo.h"



static A429 enet1;
static std::vector<ChannelInfo> channelInfo;


void processArgs(int argc, char *argv[])
{
  int opt;

  while((opt = getopt(argc, argv, ":i:s:c:")) != -1)
  {
    ChannelInfo ci;
    switch(opt)
    {
      case 'c':
        enet1.setACserverIP(optarg);
        break;
      case 's':
        if (ci.SetChannel(optarg)) channelInfo.push_back(ci);
        break;
      case 'i':
        enet1.setEnetIP(optarg);
        break;
      case ':':
        fprintf(stderr, "option needs a value\n");
        break;
      case '?':
        fprintf(stderr, "unknown option: %c\n", optopt);
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

  for (int i = 0; i < channelInfo.size(); ++i)
    enet1.StartChannel(channelInfo[i].Channel(), channelInfo[i].Speed());


  while (1)
  {
    enet1.Status();
    enet1.CheckIRIG();
    sleep(3);
  }
}
