# psvr-player
Проигрыватель 3D фильмов для гарнитуры PS VR, подключенной к компьютеру.

Отличия проигрывателя:  
- коррекция геометрических и цветовых искажений;
- поддержка основных видов 3D фильмов;
- минималистичный интерфейс.

Текущее состояние: в разработке.

## Сборка

1. Установите программы для сборки: git, cmake, g++, xxd  
**sudo apt install git cmake g++ xxd**  
2. Установите библиотеки для сборки: libvlccore-dev, libvlc-dev, libhidapi-dev, libglfw3-dev, libglm-dev:  
**sudo apt install libvlccore-dev libvlc-dev libhidapi-dev libglfw3-dev libglm-dev**  
3. Загрузите код из github или gitflic:  
В домашней директории запустите команду:  
**git clone https://github.com/evgenykislov/psvr-player.git -b main**  
3. Соберите psvr-player и установите его:  
**cd psvr-player**  
**cmake -B build**  
**cmake --build build**  
**sudo cmake --install build**  
