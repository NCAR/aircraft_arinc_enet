#include <cstdio>

class ChannelInfo
{
public:
  ChannelInfo() : channel(0), speed(0) { }

  unsigned int Channel()	{ return channel; }
  unsigned int Speed()		{ return speed; }

  bool SetChannel(const char info[])
  {
    if (info[0] < '0' || info[0] > '7')
    {
      fprintf(stderr, "Channel number out of range, %c\n", info[0]);
      return false;
    }
    if (info[1] != ',')
    {
      fprintf(stderr, "Parse exception, expected ',' in %s\n", info);
      return false;
    }
    if (info[2] < '0' || info[2] > '1')
    {
      fprintf(stderr, "Invalid speed of %c, 0=high, 1=low\n", info[2]);
      return false;
    }

    channel = info[0] - '0';
    if (info[2] == '0') speed = 100000;
    if (info[2] == '1') speed = 12500;
  }

private:
  unsigned int channel;
  unsigned int speed;
};
