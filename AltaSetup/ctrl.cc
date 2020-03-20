
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <QUdpSocket>

#include "A429.h"
#include "ChannelInfo.h"
#include "MultiCast.h"



static QUdpSocket *udp;
static QHostAddress acserver(QString("192.168.84.2"));
static A429 enet1;
static std::vector<ChannelInfo> channelInfo;


void processArgs(int argc, char *argv[])
{
  int opt;

  while((opt = getopt(argc, argv, ":i:s:c:p:")) != -1)
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
      case 'p':
        enet1.setPort(optarg);
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
  std::string dump = enet1.RegisterDump();
  fprintf(stderr, "arinc_ctrl::sigAct: %s\n", dump.c_str());
  udp->writeDatagram(dump.c_str(), dump.length(), acserver, enet1.StatusPort());
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


void initializeSequence()
{
  enet1.Open();

  while (enet1.isSetup() == false)
  {
    sleep(3);
    enet1.Open();
  }

  enet1.Status();
  enet1.CalibrateIRIG();
  enet1.Status();

  for (size_t i = 0; i < channelInfo.size(); ++i)
    enet1.StartChannel(channelInfo[i].Channel(), channelInfo[i].Speed());
}


int main(int argc, char *argv[])
{
  MultiCastStatus mcStat;

  processArgs(argc, argv);
  setupSignals();

  initializeSequence();

  udp = new QUdpSocket();
/*
  try {
    mcStat = new MultiCastStatus();
  }
  catch(const nidas::util::IOException& e) {
    mcStat = 0;
  }
*/
//  if (mcStat)
    mcStat.sendStatus("----- started -----");


  int rc, cntr = 0;
  while (1)
  {
    std::string status = enet1.Status();
    if ((rc = udp->writeDatagram(status.c_str(), status.length(), acserver, enet1.StatusPort())) < 1)
      fprintf(stderr, "udp->writeDatagram of status packet failed, nBytes=%d\n", rc);

    if (cntr++ % 3 == 0)	// every 3 seconds multicast status to status_listener
      mcStat.sendStatus(enet1.StatusPageStatus());

    if (enet1.failCounter() > 5)
      initializeSequence();

    enet1.CheckIRIG();
    usleep(989000);
  }
}
