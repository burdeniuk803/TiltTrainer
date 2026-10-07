# Залежності й відтворюваність

Основа HAL 1.0.1 та CMSIS з незміненого дерева ЛР1, commit 4f2ee8e. Ліцензії ST/CMSIS збережені. ADC, ADCEx та LL_ADC отримано з STMicroelectronics/stm32c0xx-hal-driver commit 24999b605d0b9da3e4675c4c884036ae724bb59f, tag v1.0.1. Вендорський код не змінюється.

HAL ADC для калібрування/PA0, I2C для MPU, UART для журналу/команд, GPIO для кнопки/LED/markers. RCC/FLASH/PWR/CORTEX потрібні startup і SysTick. EXTI обслуговує успадковане IRQ PC13; DMA збережено для залежностей HAL типів, transfers не застосовуються. SPI/TIM/RTC вимкнено. Власний код компілюється з -Wall -Wextra -Werror; ST з -Wall.

Docker фіксує digest Ubuntu і версії ARM GCC, binutils, newlib. apt snapshot для допоміжних пакетів не фіксується. Host tests виконуються C11 GCC/clang; Python 3 потрібен перевіркам залежностей і схем. Фактичні версії та результати перевірок зберігаються у docs/validation.
