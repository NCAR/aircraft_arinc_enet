#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "A429.h"

class ChannelInfo
{
public:
  ChannelInfo() : channel(0), speed(0) { }

  unsigned int Channel()	{ return channel; }
  unsigned int Speed()		{ return speed; }

  bool SetChannel(const char info[])
  {
    if (info[0] < '0' || info[0] > '7')
    {
      fprintf(stderr, "Channel number out of range, %c\n", info[0]);
      return false;
    }
    if (info[1] != ',')
    {
      fprintf(stderr, "Parse exception, expected ',' in %s\n", info);
      return false;
    }
    if (info[2] < '0' || info[2] > '1')
    {
      fprintf(stderr, "Invalid speed of %c, 0=high, 1=low\n", info[2]);
      return false;
    }

    channel = info[0] - '0';
    if (info[2] == '0') speed = 100000;
    if (info[2] == '1') speed = 12500;
  }

private:
  unsigned int channel;
  unsigned int speed;
};



A429 enet1;
std::vector<ChannelInfo> channelInfo;


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

  for (int i = 0; i < channelInfo.size(); ++i)
    enet1.StartChannel(channelInfo[i].Channel(), channelInfo[i].Speed());


  while (1)
  {
    enet1.Status();
    sleep(3);
  }
}
