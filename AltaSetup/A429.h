#include <ADT_L1.h>

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
  void Status();
  void CheckIRIG();
  void Close();

  bool isOpen()		{ return _isOpen; }

  void setEnetIP(const char ip[]);
  void setACserverIP(const char ip[]);


protected:

  // print human readable errors...
  void DisplayInitFailure(ADT_L0_UINT32);
  void DisplayBitFailure(ADT_L0_UINT32);

  bool _isOpen;
  bool _irigFail;

  // Ongoing status / detect stuff.
  bool _irigDetect;

  ADT_L0_UINT32	enetIP[4];
  ADT_L0_UINT32	acserverIP[4];

  std::vector<int> _channelList;
};
