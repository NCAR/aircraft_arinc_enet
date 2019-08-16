#include <QUdpSocket>
#include <QHostAddress>

#include "AltaEnet.h"

class RdUDP : public QObject
{
  Q_OBJECT

public:
  RdUDP();

protected slots:
  void	newData();

private:
  uint32_t		decodeLABEL(uint32_t);
  unsigned long		decodeIRIG(unsigned char *);
  unsigned long long	decodeTIMER(const rxp&);
  int			bcd_to_decimal(unsigned char x)	{ return x - 6 * (x >> 4); }

  QUdpSocket		*udp;
  char			buffer[65000];
  char			irigHHMMSS[32];

};

