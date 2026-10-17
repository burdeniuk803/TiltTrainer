# TiltTrainer

Тренажер відхилення від користувацького еталона. Бурденюк Дмитро Сергійович, ІО-33. Форма B, Nucleo-C031C6; середовище Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Фізичної плати немає.

ЛР 2 реалізує MPU6050/I²C, потенціометр/ADC, кнопку BASE/GPIO, числовий кут у UART і LED виходу за допуск. До калібрування кут невалідний. BASE збирає 50 нових стабільних samples; фільтр 5 векторів, допуск 5–45°, гістерезис 1°/100 мс. Motion та помилки приховують кут; після reinit потрібен новий еталон.

## Відтворення

```sh
make build
make test
make test-linux
make coverage-linux
make sanitize
```

Відкрити цю папку у VS Code та виконати `Wokwi: Start Simulator` з уже доступною ліцензією. `wokwi.toml` завантажує firmware/build/firmware.hex та ELF. На старті очікуються BOOT, три SELFTEST та WAIT_BASE/valid0/NA. Натиснути зовнішню BASE, зачекати CAL_OK. Для повільного нахилу задати MPU accelX0.5, accelY0, accelZ0.85g та gyro0. Після стабілізації й підтвердження очікується 30.5°, OUTSIDE при допуску≈25°, LED HIGH. Потенціометр End дає 45° і WITHIN.

UART приймає по одній ASCII-команді з Enter: `i` читає ID, `d` читає raw14bytes, `n` перевіряє відсутній 0x69, `f` спричиняє три заплановані NACK, `r` робить reset. Дочекатися відповіді перед наступною командою. Калібрування виконує фізичний GPIO BASE, не UART.

## Докази й межі

Вісім host groups PASS; GCC coverage переносимих модулів≥95.42%рядків/≥86.34%гілок. Власний 169 с Wokwi запис без помилок дав max MPU 20.034825 мс. Окремі записи підтверджують похилий еталон, ADC endpoints, motion, LED, NACK/recovery та reset. Пороги й точність host math не є виміряною точністю фізичного MPU. Board transport містить адаптацію моделі Wokwi; WOKWI=0 на платі не перевірено.

- [Вимоги](docs/PRD.md), [сценарії T01–T13](docs/test-plan.md), [підключення і розрахунки](docs/pinout.md).
- [Архітектура](docs/architecture.md), [тести та coverage](docs/testing.md), [залежності](docs/dependencies.md).
- [Повні записи, PulseView і аналіз](docs/captures/README.md), [журнал проблем](docs/problem-journal.md), [план і стан](docs/lab-plan.md).
- [Перевірки основи ЛР 1](docs/verification/README.md) збережені як історичні матеріали.

Звіт PDF зберігається у SP/Lab02/Reports цього студента поза репозиторієм. Зміни ЛР2 передаються через PR feature/lab-2→main. Початковий main ЛР 1 не змінювався.

Основа: Wokwi STM32 HAL example, адаптований у SP-Boilerplate. Ліцензії збережено в LICENSE та firmware/Drivers. Vendor ST sources не змінювалися.
