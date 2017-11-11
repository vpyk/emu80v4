# Emu80 4.x emulator source code

For license info, see the file "COPYING.txt"

## Ссылки
* Сайт проекта: [http://emu80.org](http://emu80.org) (временно не обновляется)
* Обсуждение на форуме zx-pk.ru: [http://zx-pk.ru/threads/27488-emu80-v-4.html](http://zx-pk.ru/threads/27488-emu80-v-4.html)
* Обсуждение на форуме nedopc.org: [http://www.nedopc.org/forum/viewtopic.php?f=43&t=17234](http://www.nedopc.org/forum/viewtopic.php?f=43&t=17234)

## Сборка и установка под Linux:

#### Требования и зависимости:
* g++ v. 4.8 и выше
* libSDL v. 2.0.5 и выше
* wxWidgets v. 3.0 и выше

#### Порядок компиляции и установки:
    git clone https://github.com/vpyk/emu80v4.git
    cd emu80v4    
    make
    make install

Установка производится в поддиректорию `emu80` в домашней директории пользователя: `~/emu80`. После установки директория с программой может быть перемещена в любое другое место. Для нормальной работы директория, в которую установлен эмулятор, должна быть доступна для записи.

Кроме того, с исходным текстом поставляется файл проекта `src/Emu80lnx.cbp` для среды разработки Code::Blocks.

## Сборка под Windows

Для сборки под Windows поставляется файл проекта `src/Emu80.cbp` для среды разработки Code::Blocks.
