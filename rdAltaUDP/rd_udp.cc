#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include "rd_udp.h"

static const int port = 56769;

/* -------------------------------------------------------------------- */
RdUDP::RdUDP()
{
  udp = new QUdpSocket(this);
  QHostAddress	host;

  host.setAddress(INADDR_ANY);
  printf("conn = %d\n",
	udp->bind(host, port, QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress));

  connect(udp, SIGNAL(readyRead()), this, SLOT(newData()));

}

/* -------------------------------------------------------------------- */
void RdUDP::newData()
{
  int nBytes = udp->readDatagram(buffer, 65000);

  const APMP_hdr *hSamp = (const APMP_hdr *)buffer;

  printf("UDP read of nBytes=%d\n", nBytes);
  if (ntohl(hSamp->mode) != 1 || (ntohl(hSamp->status) & 0xFFFF) != 0 || ntohl(hSamp->alta) != 0x414c5441)
  {
    fprintf(stderr, "Bad packet received, continuing.\n");
    fprintf(stderr, "Mode = %d, status = %u, alta = 0x%08x\n",
      ntohl(hSamp->mode), ntohl(hSamp->status) & 0xffff, ntohl(hSamp->alta));
    return;
  }

  int nFields = (ntohl(hSamp->payloadSize) - 16) / sizeof(rxp);
  static unsigned long long prevPE = 0;
  unsigned long long PE = ntohl(hSamp->PEtimeHigh); PE = ((PE << 32) | ntohl(hSamp->PEtimeLow)) / 50;

  decodeIRIG((unsigned char *)&hSamp->IRIGtimeLow);

  printf( "nFields=%3u status=0x%08x seqNum=%u, pSize=%u - PE %llu %llu IRIG julianDay=%x %s\n", nFields,
		ntohl(hSamp->status),
		ntohl(hSamp->seqNum),
		ntohl(hSamp->payloadSize),
		PE, PE - prevPE,
		ntohl(hSamp->IRIGtimeHigh), irigHHMMSS);


  const rxp *pSamp = (const rxp *) (buffer + sizeof(APMP_hdr));
  for (int i = 0; i < nFields; i++)
  {
    int channel = (ntohl(pSamp[i].control) & 0x0F000000) >> 24;
    unsigned long long ttime = decodeTIMER(pSamp[i]);
    if (channel < 8)
      printf("  %s.%-6llu  %d  %04o  %d  error=%d\n",
		irigHHMMSS, ttime-PE, channel,
		decodeLABEL(ntohl(pSamp[i].data)), ((ntohl(pSamp[i].data) & 0xFFFFFF00) >> 8),
		(ntohl(pSamp[i].control) & 0x80000000));
    else
      printf( "received channel number %d, outside 0-8, ignoring.\n", channel);
  }

  prevPE = PE;

}

/* -------------------------------------------------------------------- */
uint32_t RdUDP::decodeLABEL(uint32_t data)
{
  uint32_t RXPlabel = data & 0x000000FF;
  uint32_t tempLabel = 0;

  tempLabel |= (RXPlabel & 1) << 7;
  tempLabel |= (RXPlabel & 2) << 5;
  tempLabel |= (RXPlabel & 4) << 3;
  tempLabel |= (RXPlabel & 8) << 1;
  tempLabel |= (RXPlabel & 16) >> 1;
  tempLabel |= (RXPlabel & 32) >> 3;
  tempLabel |= (RXPlabel & 64) >> 5;
  tempLabel |= (RXPlabel & 128) >> 7;

  return tempLabel;
}

/* -------------------------------------------------------------------- */
unsigned long RdUDP::decodeIRIG(unsigned char *irig_bcd)
{
  int h, m, s;

  h = bcd_to_decimal(irig_bcd[1]);
  m = bcd_to_decimal(irig_bcd[2]);
  s = bcd_to_decimal(irig_bcd[3]);

  sprintf(irigHHMMSS, "%02d:%02d:%02d", h, m, s);
  return h * 3600 + m * 60 + s;
}

/* -------------------------------------------------------------------- */
unsigned long long RdUDP::decodeTIMER(const rxp& samp)
{
    unsigned long long ttime;
    static unsigned long long prevTime;

    /* Make 64-bit 20nsec/50Mhz Clock Ticks to 64-bit uSecs */
    ttime = ntohl(samp.timeHigh);
    ttime = ((ttime << 32) | ntohl(samp.timeLow)) / 50;

#ifdef DEBUG
    printf("  rxp irig %llu usec, dT=%lld\n", ttime, ttime - prevTime);
    printf("           %llu msec, %llu sec\n", (ttime/1000), (ttime/1000000));
    prevTime = ttime;
#endif

    return ttime;
}

