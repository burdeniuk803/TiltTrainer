# Журнал фактичних проблем ЛР2

Історичні failure logs збережено без зміни дат. Посилання нижче ведуть на поточні коміти після погодженого впорядкування дат.

## 1. Coverage не запускався зі шляхом студента, що містить пробіли

Доказ: [docs/validation/coverage-space-path-failure.txt](validation/coverage-space-path-failure.txt). Include paths взято в лапки; build/coverage розділено за OS/arch.

Виправлення: [32b3582](https://github.com/burdeniuk803/TiltTrainer/commit/32b3582cf60f917d9b97c9598f88b9fde5a4848a).

## 2. Прерване калібрування могло продовжити несвіже вікно

Доказ: [docs/validation/calibration-gap-failure.txt](validation/calibration-gap-failure.txt). Read error або gap>30мс відхиляє вікно, deadline1.2с; regression tests.

Виправлення: [8e65de2](https://github.com/burdeniuk803/TiltTrainer/commit/8e65de2d7c0f56af427f7eb4dc4b1c8c484ea02c).

## 3. Retry міг починатися від старого tick замість входу в SENSOR_ERROR

Доказ: [firmware/Board/runtime.c](../firmware/Board/runtime.c). Timer retry скидається при вході в error; host state та фактичний NACK record.

Виправлення: [8e65de2](https://github.com/burdeniuk803/TiltTrainer/commit/8e65de2d7c0f56af427f7eb4dc4b1c8c484ea02c).

## 4. CI sanitizer не міг перезаписати root-owned files після Docker tests

Доказ: [docs/validation/ci-initial-failure.txt](validation/ci-initial-failure.txt). Docker tests/build працюють з UID/GID користувача; наступний CI38430341089 успішний.

Виправлення: [8e65de2](https://github.com/burdeniuk803/TiltTrainer/commit/8e65de2d7c0f56af427f7eb4dc4b1c8c484ea02c).

## 5. Третя помилка під час calibration приховувала CAL_REJECT

Доказ: [docs/validation/calibration-third-error-failure.txt](validation/calibration-third-error-failure.txt). CAL_REJECT лишається у event mask разом із SENSOR_ERROR; додано regression.

Виправлення: [f73dab7](https://github.com/burdeniuk803/TiltTrainer/commit/f73dab707a89eee066be3c55859ef8140d490ca5).

## 6. Blocking HAL I²C не завершував транзакцію в моделі

Доказ: [docs/captures/README.md](captures/README.md). Власний bounded HAL event adapter, WOKWI guards, vendor ST untouched; повні успішні VCD.

Виправлення: [f73dab7](https://github.com/burdeniuk803/TiltTrainer/commit/f73dab707a89eee066be3c55859ef8140d490ca5).

## 7. VCD parser плутав одночасні SCL/SDA updates; clips губили тихий кінець

Доказ: [tools/vcd_trace.py](../tools/vcd_trace.py). Зміни одного timestamp групуються; tail та підтверджені initial levels збережено, sigrok independently decodes ID/bytes. Незадані initial levels відхиляються.

Виправлення: [f73dab7](https://github.com/burdeniuk803/TiltTrainer/commit/f73dab707a89eee066be3c55859ef8140d490ca5).
