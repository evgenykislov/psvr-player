#ifndef PSVRHELMETHID_H
#define PSVRHELMETHID_H

/*! Класс для обработки hid-устройств vr-шлема psvr */
class PsvrHelmetHid {
 public:
  PsvrHelmetHid();
  ~PsvrHelmetHid();

 private:
  PsvrHelmetHid(const PsvrHelmetHid&) = delete;
  PsvrHelmetHid(PsvrHelmetHid&&) = delete;
  PsvrHelmetHid& operator=(const PsvrHelmetHid&) = delete;
  PsvrHelmetHid& operator=(PsvrHelmetHid&&) = delete;
};

#endif  // PSVRHELMETHID_H
