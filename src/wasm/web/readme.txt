Структура сайта по умолчанию:

- /
    - const.js
    - emu80.css
    - emu80.js
    - emuframe.html
    - index.html
    - catalog/
      - platforms.json
      - platform1/
        - files.json
        - program1
        - program2
        ...
      - platform2/
          ...
      ...
      - options/
        - opt1.conf
        - opt2.conf
        ...
    - emu80.conf (из dist)
    - директории платформ (из dist)


Формат platforms.json:

[
    {
        "name": "rk86",                # внутреннее имя платформы
        "description": "Радио-86РК",   # отображаемое имя
        "extensions": ".rkr,.rk,.gam", # перечень расширений для фильтра открытия файла
        "defaultOptions": "crop.conf"  # опционально: дополнительный conf-файл из директории
                                       # catalog/options, применяемый по умолчанию к платформе
    },
    {
        "name": "orion",
        "description": "Пример платформы: Орион-128",
        "extensions": ".rko,.bru,.ord,.ori"
    },
    ...
]


Формат <platform.json>:

[
    {
        "name": "volcano.rkr",        # имя файйла из директории catalog/<platform>/files
        "description": "Игра Вулкан", # отображаемое имя
        "postconf": "16-9.conf"       # дополнительно применяемый conf-файл из catalog/options
    },
    {
        "name": "klad.rkr",
        "description": "Игра Клад"
    },
    ...
]
