#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <string>

namespace Config {

/*! Получить опции из конфигурации. Получить можно только часть. Для неважных
опций передаётся nullptr. Функция потокобезопасная */
void GetOptions(std::string* screen, int* eyes_distance, bool* swap_color,
    bool* swap_layer, double* rotation);

/*! Сохранить опции в конфигурации. Функция потокобезопасная */
void SetOptions(std::string* screen, int* eyes_distance, bool* swap_color,
    bool* swap_layer, double* rotation);

/*! Получить имена из конфигурационного файла. */
void GetDevicesName(uint32_t* control_device, uint32_t* sensor_device);

/*! Сохранить имена из конфигурационного файла. */
void SetDevicesName(uint32_t control_device, uint32_t sensor_device);

/*! Очистить все сохранённые опции. Функция потокобезопасная */
void ClearOptions();

};  // namespace Config

#endif  // CONFIG_FILE_H
