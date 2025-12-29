#include <Arduino.h>
class Display {
public:
  virtual ~Display() = default;

  virtual bool begin() = 0;
  virtual void update(const char* mode, int midiChannel, const DmxMessageHistory* history,
                      int historySize,
                      int historyHead) = 0;
};