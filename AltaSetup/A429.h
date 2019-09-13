
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
  void Close();

protected:

  bool _isOpen;
  bool _irigFail;

  std::vector<int> _channelList;
};
