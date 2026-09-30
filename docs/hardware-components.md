# Компоненти TiltTrainer

Бурденюк Дмитро, ІО-33.

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Фізичної плати немає. Нижче окремо описано наявний HAL-шаблон і запропоновані підключення для ЛР2. Нові піни ще не внесено в CubeMX або Wokwi.

## Поточна конфігурація

Параметри звірено з `firmware/firmware.ioc`, `Core/Src/main.c`, `Core/Src/stm32c0xx_hal_msp.c`, `Core/Inc/main.h`, linker script та `diagram.json`.

| Вузол | Реальні параметри шаблону | Джерело для апаратних можливостей |
|---|---|---|
| MCU | STM32C031C6, Cortex-M0+, `-mcpu=cortex-m0plus -mthumb`; linker script задає 32 КіБ Flash та 12 КіБ RAM | [ST DS13867 Rev 4, с. 1](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=1) |
| Clock | HSI 48 МГц, дільники HSI/SYSCLK/AHB/APB1 дорівнюють 1; `FLASH_LATENCY_1`. Зовнішні HSE/LSE у виконуваній ініціалізації не запускаються | `SystemClock_Config()`; [DS13867, с. 17](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=17) |
| UART2 | TX PA2, RX PA3, AF1; 115200, 8 біт, без парності, 1 стоп-біт, без flow control. `printf` передає байти через `__io_putchar()` і `HAL_UART_Transmit()` | [DS13867, табл. 13, с. 35](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=35) |
| LED | PA5, push-pull, початковий HIGH; перемикання приблизно раз на секунду. Повний цикл світіння/погашення близько 2 с | [ST UM2953 Rev 2, §6.10, с. 16](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf#page=16); [LED Wokwi](https://docs.wokwi.com/parts/board-st-nucleo-c031c6#onboard-led) |
| I²C1 | PB8 SCL, PB9 SDA, AF6, open-drain, без внутрішніх підтягувань. PCLK1 48 МГц; `Timing=0x20303E5D`, 7-bit addressing, analog filter ON, digital filter 0 | [DS13867, табл. 15, с. 36](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=36) |
| Кнопка шаблону | PC13, EXTI rising, `GPIO_NOPULL`. Приклад не реалізує команду калібрування | [UM2953, §6.11, с. 16](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf#page=16) |
| ADC | Не ініціалізовано; ADC HAL-файлів і каналу в поточній збірці немає | `firmware/Makefile`, `.ioc`, `main.c` |
| Wokwi | Лише Nucleo-C031C6 і serial monitor: TX монітора → PA3, RX монітора ← PA2. HEX та ELF у `firmware/build/` | `diagram.json`, `wokwi.toml`; [підтримувані функції MCU](https://docs.wokwi.com/parts/board-st-nucleo-c031c6#simulation-features) |

`0x20303E5D` є значенням регістра TIMINGR, а не підтвердженою частотою шини. Для ЛР2 ціль становить 100 кГц; параметр слід перевірити в CubeMX з фактичним clock tree та виміряти SCL під час транзакції. Зараз транзакцій до датчика немає.

Наявний цикл друкує `STM32C031C6 ready`, потім `tick <секунди>` і викликає `HAL_Delay(1000)`. Перший tick може дорівнювати 0. Це код шаблону; його поведінку в симуляторі ще не підтверджено.

## Компоненти майбутнього стенда

| Компонент | Призначення | Запропоноване підключення та обмеження |
|---|---|---|
| `wokwi-mpu6050`, аналог MPU-6050 | Вектор прискорення для кута; гіроскоп для перевірки спокою під час калібрування | VCC 3V3, GND спільний, SCL PB8, SDA PB9, AD0 GND. INT, XDA, XCL не використовуються. I²C 0x68; з AD0 HIGH адреса 0x69. Для STM32 HAL DevAddress подається як `0x68 << 1`, що підтверджує коментар до `HAL_I2C_Mem_Read()` у наявному драйвері |
| `wokwi-potentiometer` | Ручне задання дозволеного кута | VCC 3V3, GND, SIG PA0 / ADC_IN0. Заплановано 12-бітний ADC, шкала 0…4095. Атрибут `value` моделі 0…1023 не означає розрядність ADC STM32 |
| `wokwi-pushbutton` | Один запит на зняття нового еталона | Зовнішня кнопка PB0 → GND, pull-up до 3V3, active-low. PC13 шаблону не вважається вже готовою командою. У моделі контакти 1.l/1.r спільні, 2.l/2.r спільні; з'єднання має бути між різними групами |

Модель MPU6050 документує прискорення в g і `rotationX/Y/Z` у градусах за секунду. Це кутова швидкість, не кут. Для статичного тесту кута θ навколо Y задаємо `accelX=sin(θ)`, `accelY=0`, `accelZ=cos(θ)` та нульові швидкості. Джерело: [Wokwi MPU6050, Pins, Attributes, Units](https://docs.wokwi.com/parts/wokwi-mpu6050).

Для ADC PA0 відповідає ADC_IN0. Вхід обмежуємо 0…VREF+, у запропонованій схемі 0…3,3 В. Підтримка цифровим GPIO 5 В не дозволяє подавати 5 В на аналоговий вхід. Джерело: [DS13867, с. 20, 32 та характеристики ADC, с. 77](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=32). [Wokwi potentiometer](https://docs.wokwi.com/parts/wokwi-potentiometer) не моделює всю аналогову схему; VCC/GND все одно треба під'єднати. [Wokwi button](https://docs.wokwi.com/parts/wokwi-pushbutton#bouncing) за замовчуванням моделює брязкіт.

## MPU-6050: живлення та регістри

Номери сторінок нижче відповідають друкованій нумерації PDF виробника.

| Параметр | Значення та заплановане використання | Офіційне джерело |
|---|---|---|
| Живлення чипа | VDD 2,375…3,46 В; VLOGIC не вище VDD. Обираємо 3,3 В. Це не специфікація будь-якої плати GY-521 | [TDK PS-MPU-6000A-00 Rev 3.4, с. 14](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf#page=14) |
| Струм чипа | Типово 3,8 мА accel+gyro без DMP; не включає LED, стабілізатор та підтягування модуля | [PS, с. 14](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf#page=14) |
| I²C | 100/400 кГц; 7-bit адреси 0x68/0x69 за AD0. Початкова ціль стенда 100 кГц | [PS, с. 15](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf#page=15) |
| Accel | Заплановано ±2 g, 16384 LSB/g, signed 16-bit. Інші діапазони ±4/8/16 g | [PS, с. 13](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf#page=13) |
| WHO_AM_I | 0x75, очікуємо 0x68 навіть за AD0 HIGH | [RM-MPU-6000A-00 Rev 4.2, §4.32, с. 45](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf#page=45) |
| PWR_MGMT_1 | 0x6B; скинути SLEEP під час старту, обрати clock source | [RM, §4.28, с. 40](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf#page=40) |
| ACCEL_CONFIG | 0x1C; AFS_SEL=0 задає ±2 g | [RM, §4.5, с. 15](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf#page=15) |
| Дані accel | 0x3B…0x40, три пари high/low; читати одним блоком | [RM, §4.17, с. 29](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf#page=29) |

Карта регістрів також доступна через [сторінку MPU-6050 TDK, Documents](https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050). Старий прямий URL InvenSense під час перевірки перенаправляв на HTML; наведені розділи звірено з індексованим документом виробника. Драйвер ще не написано.

У фізичному прототипі I²C потребує підтягувань SDA/SCL до 3,3 В. Початковий вибір 4,7 кОм дає близько 0,70 мА на LOW-лінію; остаточний опір залежить від ємності шини. Наявний `GPIO_NOPULL` цього не забезпечує. Струм MPU та пасивних входів невеликий, але бюджет 3V3 перевірятиметься для конкретного модуля за схемою плати. Симуляція не вимірює струм споживання.

Для потенціометра приклад фізичного аналога: Bourns 3386P-1-103LF, 10 кОм, допуск ±10 %, 0,5 Вт за 85 °C; вивід 2 є повзунком. [Datasheet Bourns, с. 1–2](https://www.bourns.com/docs/Product-Datasheets/3386.pdf#page=1). За 3,3 В розрахунок дає 0,33 мА та 1,09 мВт. Це приклад для дослідження, фізичний компонент не придбано. Кнопка є пасивним контактом без регістрів; зовнішній pull-up 10 кОм дає 0,33 мА під час натискання. Вибір конкретного контакту для реальної плати потребуватиме перевірки його мінімального струму.

## Чому саме ці компоненти

MPU6050 дає вектор гравітації, GPIO-кнопка задає момент калібрування, ADC-потенціометр задає допуск. Три різні інтерфейси мають окремі перевірювані задачі. LED і UART вже є виходами платформи.

Механічний датчик нахилу відкинуто для цієї версії: дискретний контакт не дає числового кута. Потенціометр кута на шарнірі вимагає механічної осі; стенд має порівнювати довільне статичне положення з еталоном. Для регулятора допуску енкодер потребував би іншого вводу й не перевіряв би ADC. Для калібрування команда UART зручна в автоматичних тестах, але фізична кнопка дає окремий GPIO-вхід. Детальні вимоги та критерії міститимуться у PRD.

Обмеження оцінки нахилу: прискорення під час руху спотворює напрям гравітації. Навіть перевірка норми вектора й гіроскопа не доводить повної відсутності руху. Стенд розраховано на повільне встановлення положення та подальшу стабілізацію; абсолютний yaw не визначаємо.
