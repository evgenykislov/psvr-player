#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <string>

/*! Выдать имя конфигурационного файла. Функция сделана для единообразия */
std::string GetConfigFileName();

/*! Получить опции из конфигурации. получить можно только часть. Для неважных
опций передаётся nullptr */
void GetOptions(std::string* screen, int* eyes_distance);

/*! Сохранить опции в конфигурации */
void SetOptions(std::string* screen, int* eyes_distance);


#endif  // CONFIG_FILE_H
