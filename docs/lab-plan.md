# План і стан TiltTrainer

ЛР 1 слита в main, base c0b542f. Її код і матеріали збережено. ЛР 2 продовжено у feature/lab-2.

| Етап ЛР 2 | Результат |
|---|---|
| Піни, ADC, MPU, BASE, markers | Реалізовано; wiring/.ioc перевіряє tools/check_project.py |
| Переносимий MPU driver | ID, конфігурація, signed14byteburst, статуси й units перевірені fakebus та Wokwi |
| Планування і алгоритм |20/100/200 мс, debounce30 мс, calibration50 samples, normalized angle, hysteresis, validity/recovery |
| Верифікація |8 host groups, GCC coverage, ASan/UBSan, ARMbuild,3 повні фінальні VCD, PulseView screenshots |
| Документація |Архітектура, pinout, test/evidence mapping, problem journal, provenance manifest |
| Подання |Звіт у Lab02/Reports; зміни передаються через PR |

У гілці ЛР 2 рівно 10 змістовних комітів. Дати автора й комітера за погодженим навчальним графіком розміщено хронологічно 7–15 жовтня 2026. Реальні дати запусків 13 жовтня, перевірок 15 жовтня та CI збережено в доказах. Backup старої історії й незакоммічених файлів залишено в Lab02/Work/backups; main не переписано.

Фізичний прототип є окремою роботою. WOKWI=0, реальна точність, струм і RC-фронти на платі не перевірялися.
