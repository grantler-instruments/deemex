struct DmxMessageHistory {
  uint32_t timestamp;
  uint16_t channel;  // 1–512
  uint8_t value;     // 0–255
};
