# TiltTrainer

Тренажер утримання макета в межах допустимого нахилу відносно заданого положення. Бурденюк Дмитро, ІО-33.

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Платформа Nucleo-C031C6, фізичної плати немає.

## Що готово

Збережено початкову прошивку, конфігурацію CubeMX та схему Wokwi без змін. Поточний HAL-шаблон налаштовує USART2 115200 8N1, LED PA5, I²C1 PB8/PB9 та PC13 EXTI. Код друкує `STM32C031C6 ready`, потім `tick <секунди>` і перемикає LED приблизно раз на секунду. I²C-пристроїв, ADC та калібрування ще немає; схема містить лише Nucleo і serial monitor.

Підготовлено [PRD з 12 вимогами](docs/PRD.md), [опис компонентів і джерела](docs/hardware-components.md), [приймальні тести](docs/test-plan.md) та [план ЛР1/ЛР2](docs/lab-plan.md).

Дві чисті Docker-збірки на Mac та одна на Ubuntu 24.04 успішні. На Mac SHA256 ELF/HEX/BIN збіглися між двома запусками. Між Mac та Ubuntu HEX/BIN однакові; повний ELF відрізняється лише рядком шляху libgcc у `.debug_str`. Завантажувані сегменти `PT_LOAD` однакові. [Логи, хеші та пояснення](docs/verification/README.md) містять фактичний час перевірки.

Wokwi ще не запущено в межах цієї перевірки. UART/LED у симуляції потребують реального запуску й власного запису. PDF-звіт та відео ще не підготовлено.

## Збірка та запуск

З кореня цієї копії проєкту, за запущеного Docker:

```sh
docker --version
make clean
make build
ls -lh firmware/build/firmware.elf firmware/build/firmware.hex firmware/build/firmware.bin
```

`make build` збирає образ із `docker/Dockerfile` і компілює firmware в контейнері з UID/GID поточного користувача. `make clean` очищає `firmware/build/`; ця папка не входить у Git. У Makefile є `image`, `build`, `clean`; запуск симуляції виконується командою розширення Wokwi.

Відкрити цю папку у VS Code, після збірки виконати `Wokwi: Start Simulator` з уже доступною ліцензією. `wokwi.toml` використовує `firmware/build/firmware.hex` та `firmware/build/firmware.elf`. Очікуємо стартове повідомлення, послідовні tick та блимання вбудованого LED. Перший tick може бути 0. Повний цикл світіння й погашення становить близько 2 с.

## Наступний етап

У ЛР2 MPU6050 через I²C дасть оцінку нахилу за напрямом гравітації, кнопка GPIO збере еталон у спокої, а потенціометр ADC задасть допустимий кут. UART і LED показуватимуть вихід за межу. Частоти опитування, гістерезис та обробку помилок визначено в PRD як проектні критерії. Секундний цикл шаблону треба буде замінити для одночасної роботи компонентів.

Основа прошивки: [Wokwi STM32 HAL example](https://github.com/wokwi/stm32-hello-wokwi), адаптований у SP-Boilerplate. Ліцензію прикладу збережено в `LICENSE`, ліцензії ST/CMSIS у `firmware/Drivers`.
