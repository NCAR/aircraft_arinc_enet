#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "rd_udp.h"

static const int port = 56769;

/* -------------------------------------------------------------------- */
RdUDP::RdUDP() : _numAPMPpackets(0), _modeError(0), _statusError(0), _magicCookieError(0),
	_prevAPMPseqNum(0), _APMPseqError(0)
{
  udp = new QUdpSocket(this);
  QHostAddress	host;

  host.setAddress(INADDR_ANY);
  printf("conn = %d\n",
	udp->bind(host, port, QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress));

  for (int i = 0; i < 8; ++i)
    _prevRXPseqNum[i] = -1;

  connect(udp, SIGNAL(readyRead()), this, SLOT(newData()));

}

RdUDP::~RdUDP()
{
  // Print statistics...
  printf("\n\nTotal number APMP packets = %u\n", _numAPMPpackets);
  printf("  mode errors=%u, status errors=%u, alignment errors=%u, sequence errors=%u\n",
	_modeError, _statusError, _magicCookieError, _APMPseqError);

  for (int i = 0; i < 8; ++i)
  {
    if (_numRXP[i] > 0) {
      printf("\n\nTotal RXP packets for channel %u = %u\n", i, _numRXP[i]);
      printf("  decode errors=%u, sequence errors=%u\n",
		_rxpDecodeError[i], _rxpSeqError[i]);
    }
  }

}

/* -------------------------------------------------------------------- */
void RdUDP::newData()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int nBytes = udp->readDatagram(buffer, 65000);
  swapPacket((uint32_t *)buffer, 12);		// APMP header has 12 words to swap.

  const APMP_hdr *hSamp = (const APMP_hdr *)buffer;

  struct tm *gm = gmtime(&tv.tv_sec);
  printf("\n%02d:%02d:%02d.%ld  UDP read of nBytes=%d\n", gm->tm_hour, gm->tm_min, gm->tm_sec, tv.tv_usec /1000, nBytes);

  // Check packet health.
  ++_numAPMPpackets;
  if (hSamp->mode != 1) {
    _modeError++;
    fprintf(stderr, "Bad mode received, Mode = %d, status = %u, alta = 0x%08x\n",
      hSamp->mode, hSamp->status & 0xffff, hSamp->alta);
    return;
  }

  if ((hSamp->status & 0xFFFF) != 0) {
    _statusError++;
    fprintf(stderr, "Bad status received, Mode = %d, status = %u, alta = 0x%08x\n",
      hSamp->mode, hSamp->status & 0xffff, hSamp->alta);
    return;
  }

  if (hSamp->alta != 0x414c5441) {
    _magicCookieError++;
    fprintf(stderr, "Bad magic cookie  received, Mode = %d, status = %u, alta = 0x%08x\n",
      hSamp->mode, hSamp->status & 0xffff, hSamp->alta);
    return;
  }
  if (_prevAPMPseqNum > 0 && hSamp->seqNum != _prevAPMPseqNum+1) {
    _APMPseqError++;
    fprintf(stderr, "APMP sequence anomaly : prevSeq=%d, thisSeq=%d\n", _prevAPMPseqNum+1, hSamp->seqNum);
  }
  _prevAPMPseqNum = hSamp->seqNum;

  int nFields = (hSamp->payloadSize - 16) / sizeof(rxp);
  swapPacket((uint32_t *)&buffer[sizeof(APMP_hdr)], nFields * 4);

  static unsigned long long prevPE = 0;
  unsigned long long PE = hSamp->PEtimeHigh; PE = ((PE << 32) | hSamp->PEtimeLow) / 50;

  decodeIRIG((unsigned char *)&hSamp->IRIGtimeLow);

  printf( "nFields=%3u status=0x%08x seqNum=%u, pSize=%u - PE %llu %llu IRIG julianDay=%x %s\n", nFields,
		hSamp->status,
		hSamp->seqNum,
		hSamp->payloadSize,
		PE, PE - prevPE,
		hSamp->IRIGtimeHigh, irigHHMMSS);


  const rxp *pSamp = (const rxp *) (buffer + sizeof(APMP_hdr));
  for (int i = 0; i < nFields; i++)
  {
    int channel = (pSamp[i].control & 0x0F000000) >> 24;
    int seqNum = (pSamp[i].control & 0x00FF0000) >> 16;
    _numRXP[channel]++;

    if (pSamp[i].control & 0x80000000) _rxpDecodeError[channel]++;

    if (_prevRXPseqNum[channel] != -1 &&
       (seqNum != 0 && _prevRXPseqNum[channel] != 255) && // rollover, counter is 8 bit
        seqNum != _prevRXPseqNum[channel] + 1)
    {
      _rxpSeqError[channel]++;
      fprintf(stderr, "RXP sequence anomaly : prevSeq=%d, thisSeq=%d\n",
		_prevRXPseqNum[channel], seqNum);
    }
    _prevRXPseqNum[channel] = seqNum;

    unsigned long long ttime = decodeTIMER(pSamp[i]);
    if (channel < 8)
      printf("  %s.%-6llu  %d  %04o  %d  error=%d\n",
		irigHHMMSS, (ttime-PE)/1000, channel,
		decodeLABEL(pSamp[i].data), ((pSamp[i].data & 0xFFFFFF00) >> 8),
		(pSamp[i].control & 0x80000000));
    else
      printf( "received channel number %d, outside 0-7, ignoring.\n", channel);
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

  h = bcd_to_decimal(irig_bcd[2]);
  m = bcd_to_decimal(irig_bcd[1]);
  s = bcd_to_decimal(irig_bcd[0]);

  sprintf(irigHHMMSS, "%02d:%02d:%02d", h, m, s);
  return h * 3600 + m * 60 + s;
}

/* -------------------------------------------------------------------- */
unsigned long long RdUDP::decodeTIMER(const rxp& samp)
{
    unsigned long long ttime;
    static unsigned long long prevTime;

    /* Make 64-bit 20nsec/50Mhz Clock Ticks to 64-bit uSecs */
    ttime = samp.timeHigh;
    ttime = ((ttime << 32) | samp.timeLow) / 50;

#ifdef DEBUG
    printf("  rxp irig %llu usec, dT=%lld\n", ttime, ttime - prevTime);
    printf("           %llu msec, %llu sec\n", (ttime/1000), (ttime/1000000));
    prevTime = ttime;
#endif

    return ttime;
}

/* -------------------------------------------------------------------- */
void RdUDP::swapPacket(uint32_t *p, size_t n)
{
  for (size_t i = 0; i < n; ++i)
    p[i] = ntohl(p[i]);
}

