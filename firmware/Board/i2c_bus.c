#include "i2c_bus.h"
#include "main.h"
static tt_io_result io_status(I2C_HandleTypeDef *bus, HAL_StatusTypeDef result)
{
  if (result == HAL_OK) return TT_IO_OK;
  if (result == HAL_BUSY) return TT_IO_BUS_ERROR;
  uint32_t error=HAL_I2C_GetError(bus);
  if (result == HAL_TIMEOUT || (error & HAL_I2C_ERROR_TIMEOUT)) return TT_IO_TIMEOUT;
  if (error & HAL_I2C_ERROR_AF) return TT_IO_NACK;
  return TT_IO_BUS_ERROR;
}
/* Wokwi keeps START latched and updates TXIS/RXNE after the register access.
 * Service the unmodified HAL event state machine with a 10 us settling interval.
 * I2C NVIC remains disabled; UART and SysTick interrupts still run. */

#if TT_WOKWI
static void bus_settle(void)
{
  uint32_t start=SysTick->VAL;
  uint32_t ticks=SystemCoreClock/100000U;
  while (((start + SysTick->LOAD + 1U - SysTick->VAL) % (SysTick->LOAD + 1U)) < ticks) {}
}
#endif
static tt_io_result bus_complete(I2C_HandleTypeDef *bus, HAL_StatusTypeDef started)
{
  if (started != HAL_OK) return io_status(bus,started);
  #if TT_WOKWI
  CLEAR_BIT(bus->Instance->CR2,I2C_CR2_START);
  #endif
  uint32_t start=HAL_GetTick();
  while (HAL_I2C_GetState(bus)!=HAL_I2C_STATE_READY) {
    HAL_I2C_EV_IRQHandler(bus);
    HAL_I2C_ER_IRQHandler(bus);
    #if TT_WOKWI
    bus_settle();
    #endif
    if (HAL_GetTick()-start >= 10U) {
      HAL_I2C_DeInit(bus);
      HAL_I2C_Init(bus);
      return TT_IO_TIMEOUT;
    }
  }
  return io_status(bus,HAL_I2C_GetError(bus)==HAL_I2C_ERROR_NONE?HAL_OK:HAL_ERROR);
}
tt_io_result tt_bus_read(void *ctx, uint8_t address, uint8_t reg, uint8_t *data, size_t length)
{
  I2C_HandleTypeDef *bus=ctx;
  if (__HAL_I2C_GET_FLAG(bus,I2C_FLAG_BUSY)) return TT_IO_TIMEOUT;
  tt_io_result result=bus_complete(bus,HAL_I2C_Master_Transmit_IT(bus,(uint16_t)(address<<1),&reg,1));
  if (result != TT_IO_OK) return result;
  return bus_complete(bus,HAL_I2C_Master_Receive_IT(bus,(uint16_t)(address<<1),data,(uint16_t)length));
}
tt_io_result tt_bus_write(void *ctx, uint8_t address, uint8_t reg, uint8_t value)
{
  I2C_HandleTypeDef *bus=ctx;
  if (__HAL_I2C_GET_FLAG(bus,I2C_FLAG_BUSY)) return TT_IO_TIMEOUT;
  uint8_t packet[]={reg,value};
  return bus_complete(bus,HAL_I2C_Master_Transmit_IT(bus,(uint16_t)(address<<1),packet,sizeof packet));
}
