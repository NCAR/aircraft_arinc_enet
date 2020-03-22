#include <ADT_L1.h>

#include <cstdlib>
#include <string>
#include <vector>

/**
 * Class to initialize and control Alta ENET ARINC appliance.
 */
class A429
{
public:
  A429();
  ~A429();


  /**
   * Open the device
   */
  void Open();
  void Setup();
  void CalibrateIRIG();
  void StartChannel(int channel, int speed);

  /**
   * Query ENET device status registers and assemble the status/hkp packet to
   * send.
   */
  std::string StatusPacket();

  /**
   * Create HTML packet to Multicast to status_listner.  Akin to the
   * printStatus() in the nidas DSMSensor class.
   * to send.
   */
  std::string StatusPagePacket();

  /**
   * Method to query ENET device for full memory dump, called when there is a
   * device failure.  May or may not be useful.
   */
  std::string RegisterDump();
  void CheckIRIG();
  void Close();

  bool	isOpen()	{ return _isOpen; }
  bool	isSetup()	{ return _isSetup; }
  int	Port()		{ return _port; }
  int	StatusPort()	{ return _port+1; }
  int	failCounter()	{ return _failCounter; }

  void setEnetIP(const char ip[]);
  void setACserverIP(const char ip[]);
  void setPort(const char port[])	{ _port = atoi(port); }


protected:

  // print human readable errors...
  void DisplayInitFailure(ADT_L0_UINT32);
  void DisplayBitFailure(ADT_L0_UINT32);

  bool _isOpen;
  bool _isSetup;
  bool _irigFail;

  // Ongoing status / detect stuff.
  bool _irigDetect, _irigLock;
  /// IRIG time as read from ENET device. "HH:MM:SS".
  char _timeStamp[32];
  ADT_L0_UINT32 _bitStatus, _globalCSR, _PErootCSR, _PErootSTS, _PEbitSTS;
  ADT_L0_UINT32 _transactions, _retries, _failures;

  int  _failCounter;

  ADT_L0_UINT32	_enetIP[4];
  ADT_L0_UINT32	_acserverIP[4];
  unsigned int _port;

  std::vector<int> _channelList;
};
