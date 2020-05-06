#ifndef _BLE_CUSTOM_CHARACTERISTIC
#define _BLE_CUSTOM_CHARACTERISTIC

#include <cstdint>

class BleCustomCharacteristicValueType {
 public:
  virtual ~BleCustomCharacteristicValueType() = default;

  virtual uint8_t* data() const                                  = 0;
  virtual uint16_t max_size_in_bytes() const                     = 0;
  virtual uint16_t size_in_bytes() const                         = 0;
  virtual bool     replace(const uint8_t* buf, const size_t len) = 0;
};

#endif  // !_BLE_CUSTOM_CHARACTERISTIC