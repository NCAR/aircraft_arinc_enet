#include <QApplication>

#include <csignal>

#include "rd_udp.h"

RdUDP *rdr = 0;

void sighandler(int s)
{
  delete rdr;
  exit(0);
}


int main(int argc, char *argv[])
{
  QApplication app(argc, argv, false);

  rdr = new RdUDP();

  signal(SIGINT, sighandler);
  signal(SIGFPE, sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGSEGV, sighandler);

  app.exec();
}
