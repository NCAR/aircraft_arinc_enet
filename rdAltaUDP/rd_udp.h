#include <QUdpSocket>
#include <QHostAddress>

#include "AltaEnet.h"

#include <map>

class RdUDP : public QObject
{
  Q_OBJECT

public:
  RdUDP();
  ~RdUDP();

protected slots:
  void	newData();

private:
  uint32_t	decodeLABEL(uint32_t);
  unsigned long	decodeIRIG(unsigned char *);
  long long	decodeTIMER(const rxp&);
  int		bcd_to_decimal(unsigned char x)	{ return x - 6 * (x >> 4); }
  void		swapPacket(uint32_t *, size_t);

  QUdpSocket	*udp;
  char		buffer[65000];
  char		irigHHMMSS[32];

  int		_numAPMPpackets, _modeError, _statusError, _magicCookieError;
  uint32_t	_prevAPMPseqNum, _APMPseqError;
  std::map<int, int>  _numRXP, _rxpDecodeError, _prevRXPseqNum, _rxpSeqError;
};

