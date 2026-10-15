# Цифрові записи TiltTrainer

Власні запуски Wokwi від 13.10.2026. Візуальне декодування PulseView і підготовка фінальних фрагментів виконані 15.10.2026. Дати записів та CI не змінюються разом із датами Git-комітів. Фізична плата не перевірялася.

## Походження і відтворення

Три повні фінальні VCD збережено тут без втрат як `*-full.vcd.gz`; оригінали також у `SP/Lab02/Work/captures` студента. `full-recordings.json` містить SHA256 нестиснених оригіналів. HEX/ELF, що виконувались у цих запусках, мають хеші з `recorded-firmware.sha256`. Вони збіглися з поточною локальною збіркою 15.10.2026.

`clip-origins.json` вказує джерело, часовий offset та крок SR для кожного короткого фрагмента. Час у VCD/SR фрагментах починається з нуля. Початкові рівні та останній тихий інтервал збережено. SR пересемплює цифрові переходи з кроком 50 нс для I²C/UART/ADC та 1 мкс для тривалих GPIO-сценаріїв. Похибка позиції фронту не більша одного sample. PVS містить налаштування видимих каналів та декодерів. Інтервали нижче обчислено з оригінальних timestamp VCD, а не з пікселів.

```sh
gzip -dc docs/captures/final-adapter-long-full.vcd.gz > /tmp/tilttrainer.vcd
python3 tools/vcd_trace.py /tmp/tilttrainer.vcd --output /tmp/tilt-analysis.json
python3 tools/vcd_trace.py /tmp/tilttrainer.vcd --output /tmp/tilt-id.vcd --clip-ms 103.65 104.30
sigrok-cli -i /tmp/tilt-id.vcd -I vcd:downsample=50 -o /tmp/tilt-id.sr -O srzip
sigrok-cli -i /tmp/tilt-id.sr -P i2c:scl=SCL:sda=SDA -A i2c=address-read:address-write:data-read:data-write:ack:nack:start:stop
```

Відкрити `bringup.sr`, `first-burst.sr`, `first-uart-byte.sr`, `first-adc.sr`, `baseline.sr` або `nack.sr` у PulseView. Для UART використати TX=UART_TX, 115200, 8 data bits, parity none, 1 stop, LSB-first. Суміжний PVS відновлює конфігурацію. Декодовані `.txt` незалежно отримано sigrok; власний `vcd_trace.py` додатково формує JSON та UART-текст.

## I²C: ID, дані та NACK

WHO_AM_I: START, D0 ACK, 75 ACK, STOP; START, D1 ACK, 68, master NACK, STOP. Wire D0/D1 відповідають 7-bit 0x68. Останній NACK читання завершує master і не означає відмови MPU. Pointer і receive є окремими HAL операціями зі STOP між ними. Перший ID у фінальному записі від 103.710175 до 104.246450 мс.

Burst після pointer3B дає 14 bytes. У спокої `000000004000EF5C000000000000`: accel=(0,0,1)g, gyro=0, temp≈24°C. Нахил `200000003666EF5C000000000000`: ax=8192/16384=0.5g, az=13926/16384=0.8499756g. Кут від upright≈30.466°, UART округлює 30.5°. `test_recorded.c` відтворює саме ці кадри, зокрема калібрування похилого еталона. GUI MPU має крок 0.05g, тому ця поза не є ідеальними 30°.

Виміряний SCL period 10.000 мкс, 100kHz, high/low по5мкс у моделі. Програмована частина TIMINGR при PCLK48MHz становить9.8125мкс без фільтра/синхронізації. Цифрові фронти моделі не доводять фізичний rise time. Receive14bytes має135clocks=1.350мс плюс START/STOP; фактичне receive≈1.365мс. Повний read включає pointer, gap та HAL service.

`nack.sr` показує справжню відсутність ACK за адресою0x69, wire D2. Це контрольована fault injection, а не від'єднання моделі MPU. У `final-calibration-fault.uart.txt`: FAULT t_ms146364, SENSOR_ERROR146405, RETRY147405 OK, WAIT_BASE147411. Три заплановані читання69 дали NACK, після чого повернуто адресу68. Інтервал до SENSOR_ERROR41мс, retryчерез1000мс, старий еталон скинуто. Timestamp UART є tick застосунку; він не тотожний часу початку фізичного передавання рядка.

## GPIO та ADC

У `final-inclined-adc-reset` BASE утримувався близько1500мс. CAL_START805, CAL_OK1788, тобто983мс/50samples; нормований еталон=(507,0,861)mg. На `baseline.sr` видно активний LOW і неперервні MPU_MARK/ADC_MARK, LEDLOW. Повторного CAL_START під час hold немає. Окреме upright калібрування у `final-calibration-fault`:4922→5908,986мс.

Потенціометр дав ADC0/limit5.0 та ADC4095/limit45.0. Reset під час активного еталона повернув BOOT та WAIT_BASE/valid0/NA, зберігши фізичне положення потенціометра4095. У UART записі біля reset є два не-друковані байти, в тексті вони чесно екрановані як `\x00`; reset обірвав UART frame. Далі повний новий self-test читається. Це не чисте завершення попереднього рядка.

`final-adapter-long`:169023.475475мс запису,8446MPU pulses, min19.950775/max20.034825мс;1690ADC pulses, min99.95715/max105.47875мс. ADC_MARK high70.175мкс; курсори на20MHz SR показують≈70.2мкс. Теоретична ADC conversion173/12MHz=14.4167мкс, marker містить HAL Start/Poll/GetValue/Stop і software/interrupt overhead. DATA t_ms видається з period200мс. Цей довгий запис переважноWAIT_BASE; не підміняє функціональні сценарії.

`final-calibration-fault` містить motion112727, TRAINING113870, OUTSIDE113970, потім NACK/recovery. Глобальний maxMPU1004.0516мс включає дозволене очікування retry в SENSOR_ERROR. `final-inclined-adc-reset` max133.81235мс включає reset, ADC_MARK1571.525мкс включає перезапуск у середині pulse. Ці значення не треба порівнювати з критерієм нормального безпомилкового опитування.

Аналізатор Wokwi обмежений5млн transitions; довгі записи завершуються приблизно169–170с, хоча UI працював довше. Максимум на скінченному інтервалі не є доказом нескінченної стабільності.

## Реальна аномалія HAL у моделі

Історичні `hal-mem-read-failure-full.vcd` і `hal-master-failure-full.vcd` збережено в Lab02/Work/captures. HAL Mem_Read підтверджував address/pointer, але не генерував потрібного RESTART/STOP. Blocking Master після STOP генерував зайвий START із00NACK та зависав на write. Фінальний Board/i2c_bus використовує незмінений HAL `_IT` state machine, опитує HAL event/error handlers з вимкненим I2C NVIC та deadline10мс на фазу. UART/SysTick interrupts працюють. Лише WOKWI=1 очищає stickySTART та дає10мкс на відкладене оновлення TXIS/RXNE моделі. У RM0490 Rev5, с.731, START очищає hardware, запис0 не має ефекту на фізичному MCU; адаптація явно відокремлена препроцесором. Vendor ST не змінено. WOKWI=0 на фізичній платі не перевірено.

Перший функціональний запис128.897с і його JSON/UART лишені як історичне свідчення. Він не містить початкових рівнів усіх каналів, тому фінальні короткі clips згенеровані з трьох пізніших повних записів. Експортер тепер відхиляє фрагмент, якщо перед його початком немає підтвердженого рівня кожного каналу. Жодних вигаданих переходів або ретушування осцилограм немає.
