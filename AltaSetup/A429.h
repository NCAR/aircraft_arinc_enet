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
  std::string Status();
  std::string RegisterDump();
  void CheckIRIG();
  void Close();

  bool	isOpen()	{ return _isOpen; }
  bool	isSetup()	{ return _isSetup; }
  int	Port()		{ return _port; }
  int	StatusPort()	{ return _statusPort; }
  int	failCounter()	{ return _failCounter; }

  void setEnetIP(const char ip[]);
  void setACserverIP(const char ip[]);
  void setPort(const char port[])	{ _port = atoi(port); }
  void setStatusPort(const char port[])	{ _statusPort = atoi(port); }


protected:

  // print human readable errors...
  void DisplayInitFailure(ADT_L0_UINT32);
  void DisplayBitFailure(ADT_L0_UINT32);

  bool _isOpen;
  bool _isSetup;
  bool _irigFail;

  // Ongoing status / detect stuff.
  bool _irigDetect;

  int  _failCounter;

  ADT_L0_UINT32	enetIP[4];
  ADT_L0_UINT32	acserverIP[4];
  unsigned int _port;
  unsigned int _statusPort;

  std::vector<int> _channelList;
};
