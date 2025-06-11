#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <string>

/*! Выдать имя конфигурационного файла. Функция сделана для единообразия */
std::string GetConfigFileName();

/*! Получить опции из конфигурации. получить можно только часть. Для неважных
опций передаётся nullptr */
void GetOptions(std::string* screen, int* eyes_distance, bool* swap_color,
    bool* swap_layer, double* rotation);

/*! Сохранить опции в конфигурации */
void SetOptions(std::string* screen, int* eyes_distance, bool* swap_color,
    bool* swap_layer, double* rotation);

/*! Очистить все сохранённые опции */
void ClearOptions();

#endif  // CONFIG_FILE_H
