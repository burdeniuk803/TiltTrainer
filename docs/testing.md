# Автоматичні перевірки

Вісім груп host tests охоплюють Sensors і Trainer: регістри MPU та units; debounce і wrap; свіже калібрування; синтетичні кути/ADC; точні межі гістерезису; validity/errors/recovery; 60-секундний планувальник із одночасними впливами. Fake bus повертає registers/frames, а перевірки write sequence є mock expectations. Окрема група декодує фактичні frames з Wokwi та відтворює кут 30.5°, похилий еталон і невалідність motion. assert.h зупиняє runner на першому failure, не надає test isolation або rich mock diagnostics.

13.10.2026 усі групи PASS на macOS arm64 clang і Linux aarch64 GCC. AddressSanitizer/UndefinedBehaviorSanitizer не видали діагностик. Максимальна похибка синтетичного розрахунку 0.000012°, це не оцінка фізичного MPU. Host scheduling 60000 мс: 3000 MPU, 600 ADC, 300 UART, 6 натисків; max 20/200 мс. Цей тест не виконує реальні HAL calls або UART hardware.

Обрана ціль GCC coverage для кожного переносимого модуля: ≥90% виконуваних рядків і ≥80% branch directions taken. Ціль охоплює нормальні переходи й основні error paths; вона не означає повного покриття станів або всіх числових комбінацій. Core/Board/ST/CMSIS не входять до host coverage. Їх перевіряють ARM build і Wokwi. GCC: MPU 32/32 lines, 18/18 branch directions; Trainer 125/131 lines, 139/161 branches; Timing 17/17 lines, 11/12 branches. CI перевіряє цей критерій для кожного модуля.

Команди: `make test`, `make sanitize`, `make coverage GCOV='xcrun llvm-cov gcov'` на macOS; `make test-linux`, `make coverage-linux` у Docker. GCC і clang по-різному рахують гілки isfinite та рядки, тому критерій застосовуємо до зафіксованого GCC. Build directories розділено за OS/architecture; coverage очищає старі counters. Include paths беруться в лапки через пробіли в імені студента.

[Власні цифрові записи](captures/README.md) перевіряють інтеграцію фінальної прошивки. Нормальний запис169.023с дав maxMPU20.034825мс, ADC99.95715–105.47875мс, highADC70.175мкс. Окремі записи перевірили upright/похилий еталон986/983мс, рух, OUTSIDE, крайні ADC, NACK/recovery та reset. Інтервали в режимі помилки/reset не включено до оцінки нормального PRD period. 
