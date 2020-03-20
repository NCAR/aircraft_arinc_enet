#include <MultiCast.h>

#include <nidas/core/SocketAddrs.h>


// Can we use loopback on the gv server?
const std::string MultiCastStatus::DATA_NETWORK = "192.168.84";

MultiCastStatus::MultiCastStatus()
{
  maddr = nidas::util::Inet4Address::getByName(NIDAS_MULTICAST_ADDR);
  msaddr = nidas::util::Inet4SocketAddress(maddr,NIDAS_STATUS_PORT_UDP);

  // Set to proper interface if this computer has more than one.
  std::list<nidas::util::Inet4NetworkInterface> itf = msock.getInterfaces();
  std::list<nidas::util::Inet4NetworkInterface>::const_iterator itfi;
  for (itfi = itf.begin(); itfi != itf.end(); ++itfi)
    if (itfi->getAddress().getHostAddress().compare(0, DATA_NETWORK.size(), DATA_NETWORK) == 0)
      msock.setInterface(maddr,*itfi);
}

MultiCastStatus::~MultiCastStatus()
{
  msock.close();
}

void MultiCastStatus::sendStatus(const char timeStamp[])
{
  std::string statstr;
  statstr = "<?xml version=\"1.0\"?><group><name>ARINC</name><clock>";
  statstr += timeStamp;
  statstr += "</clock></group>\n";
  try {
    msock.sendto(statstr.c_str(), statstr.length()+1, 0, msaddr);
  }
  catch (const nidas::util::IOException& ioe) {
    fprintf(stderr, "arinc_ctrl::sendStatus: Network unreachable.\n");
  }
}

