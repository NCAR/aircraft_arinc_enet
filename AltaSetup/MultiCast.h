
#include <nidas/util/Socket.h>
#include <nidas/core/Sample.h>

/**
 * Multicast to status_listener
 */
class MultiCastStatus
{
public:
  MultiCastStatus();
  ~MultiCastStatus();

  void sendStatus(const char ts[]);

private:
  nidas::util::MulticastSocket msock;
  nidas::util::Inet4Address maddr;
  nidas::util::Inet4SocketAddress msaddr;

  static const std::string DATA_NETWORK;
};

