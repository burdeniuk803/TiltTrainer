#include "runtime.h"
#include "main.h"
#include "mpu6050.h"
#include "timing.h"
#include "trainer.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;
static tt_io_result io_status(HAL_StatusTypeDef result)
{
  if (result == HAL_OK) return TT_IO_OK;
  return result == HAL_TIMEOUT ? TT_IO_TIMEOUT : TT_IO_NACK;
}
static tt_io_result bus_read(void *ctx, uint8_t address, uint8_t reg, uint8_t *data, size_t length)
{
  return io_status(HAL_I2C_Mem_Read(ctx,(uint16_t)(address<<1),reg,I2C_MEMADD_SIZE_8BIT,data,(uint16_t)length,10));
}
static tt_io_result bus_write(void *ctx, uint8_t address, uint8_t reg, uint8_t value)
{
  return io_status(HAL_I2C_Mem_Write(ctx,(uint16_t)(address<<1),reg,I2C_MEMADD_SIZE_8BIT,&value,1,10));
}
static tt_imu imu = {&hi2c1,bus_read,bus_write,0x68};
static tt_clock mpu_clock, adc_clock, uart_clock, retry_clock;
static tt_button button;
static uint16_t adc;
static bool adc_ok;
static tt_io_result imu_status;
static tt_sample sample;
static unsigned fault_reads;
static tt_trainer trainer;
/* Bounded UART queue; hardware interrupts send without delaying the scheduler. */
static uint8_t tx[2048], transmitting;
static unsigned tx_head, tx_tail;
static void log_line(const char *format, ...)
{
  char line[256]; va_list args;
  va_start(args,format); int count=vsnprintf(line,sizeof line,format,args); va_end(args);
  if (count < 0) return;
  unsigned length = (unsigned)count < sizeof line ? (unsigned)count : sizeof line-1;
  unsigned free_bytes = (tx_tail + sizeof tx - tx_head - 1U) % sizeof tx;
  if (length > free_bytes) return;
  for (unsigned i=0;i<length;++i) { tx[tx_head]=(uint8_t)line[i]; tx_head=(tx_head+1U)%sizeof tx; }
}
static void uart_service(void)
{
  if (tx_tail != tx_head && huart2.gState == HAL_UART_STATE_READY) {
    transmitting=tx[tx_tail];
    if (HAL_UART_Transmit_IT(&huart2,&transmitting,1)==HAL_OK) tx_tail=(tx_tail+1U)%sizeof tx;
  }
}
static void read_adc(void)
{
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
  adc_ok=HAL_ADC_Start(&hadc1)==HAL_OK;
  if (adc_ok) adc_ok=HAL_ADC_PollForConversion(&hadc1,2)==HAL_OK;
  if (adc_ok) adc=(uint16_t)HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
  tt_trainer_adc(&trainer,adc_ok,adc);
}
void tt_board_start(void)
{
  tt_trainer_init(&trainer);
  HAL_Delay(100); /* Only power-on settling; never in the measurement loop. */
  HAL_NVIC_SetPriority(USART2_IRQn,1,0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
  HAL_ADCEx_Calibration_Start(&hadc1);
  read_adc(); uint8_t id=0;
  imu_status=tt_imu_start(&imu,&id);
  tt_sensor_start_result(&trainer,imu_status);
  uint32_t now=HAL_GetTick();
  mpu_clock=(tt_clock){now,20}; adc_clock=(tt_clock){now,100}; uart_clock=(tt_clock){now,200};
  retry_clock=(tt_clock){now,1000};
  tt_button_init(&button,now);
  log_line("BOOT product=TiltTrainer lab=2\r\n");
  log_line("SELFTEST device=MPU6050 status=%s id=0x%02X address=0x68\r\n",tt_io_name(imu_status),id);
  log_line("SELFTEST device=ADC status=%s adc=%u\r\n",adc_ok?"OK":"ERROR",adc);
  log_line("SELFTEST device=BASE status=READABLE level=%u\r\n",(unsigned)HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0));
  log_line("READY mode=%s valid=0 angle_deg=NA commands=i:id,d:data,n:nack,r:reset,f:fault\r\n",tt_mode_name(trainer.mode));
  trainer.event=TT_NO_EVENT;
}
void tt_board_poll(void)
{
  uint32_t now=HAL_GetTick(); uart_service();
  tt_trainer_tick(&trainer,now);
  if (tt_button_update(&button,HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)==GPIO_PIN_RESET,now))
    tt_request_base(&trainer,now);
  if (trainer.mode==TT_SENSOR_ERROR && tt_due(&retry_clock,now)) {
    uint8_t id=0; imu_status=tt_imu_start(&imu,&id);
    log_line("RETRY t_ms=%lu status=%s id=0x%02X\r\n",(unsigned long)now,tt_io_name(imu_status),id);
    if (imu_status==TT_IO_OK) tt_sensor_start_result(&trainer,imu_status);
  }
  if (trainer.mode!=TT_SENSOR_ERROR && tt_due(&mpu_clock,now)) {
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET);
    if (fault_reads) { /* Fault command performs a real read at an absent address. */
      --fault_reads; imu.address=0x69; imu_status=tt_imu_read(&imu,&sample); imu.address=0x68;
    } else imu_status=tt_imu_read(&imu,&sample);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET);
    tt_trainer_sample(&trainer,&sample,imu_status,HAL_GetTick());
  }
  if (tt_due(&adc_clock,now)) read_adc();
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,tt_led(&trainer)?GPIO_PIN_SET:GPIO_PIN_RESET);
  if (trainer.event!=TT_NO_EVENT) {
    log_line("EVENT t_ms=%lu event=%s mode=%s samples=%u reason=%s state=%s\r\n",
      (unsigned long)now,tt_event_name(trainer.event),tt_mode_name(trainer.mode),trainer.cal_count,trainer.reason,trainer.boundary.position==TT_OUTSIDE?"OUTSIDE":"WITHIN");
    trainer.event=TT_NO_EVENT;
  }
  if (tt_due(&uart_clock,now)) {
    char angle[16]="NA", limit[16]="NA";
    if (trainer.valid) { unsigned value=(unsigned)(trainer.angle*10.0f+0.5f); snprintf(angle,sizeof angle,"%u.%u",value/10U,value%10U); }
    if (trainer.adc_valid) { unsigned value=(unsigned)(trainer.limit*10.0f+0.5f); snprintf(limit,sizeof limit,"%u.%u",value/10U,value%10U); }
    log_line("DATA t_ms=%lu mode=%s valid=%u angle_deg=%s limit_deg=%s state=%s adc=%u\r\n",
      (unsigned long)now,tt_mode_name(trainer.mode),(unsigned)trainer.valid,angle,limit,trainer.valid?(trainer.boundary.position==TT_OUTSIDE?"OUTSIDE":"WITHIN"):"NA",adc);
  }
  if (__HAL_UART_GET_FLAG(&huart2,UART_FLAG_RXNE)) {
    uint8_t command;
    if (HAL_UART_Receive(&huart2,&command,1,1)!=HAL_OK) return;
    if (command=='r') { log_line("EVENT t_ms=%lu event=RESET\r\n",(unsigned long)now); NVIC_SystemReset(); }
    if (command=='f') { fault_reads=3; log_line("FAULT t_ms=%lu reads=3 address=0x69\r\n",(unsigned long)now); }
    if (command=='i') { uint8_t id=0; tt_io_result result=bus_read(&hi2c1,0x68,0x75,&id,1); log_line("ID t_ms=%lu status=%s id=0x%02X\r\n",(unsigned long)now,tt_io_name(result),id); }
    if (command=='n') { uint8_t id=0; tt_io_result result=bus_read(&hi2c1,0x69,0x75,&id,1); log_line("PROBE t_ms=%lu address=0x69 status=%s\r\n",(unsigned long)now,tt_io_name(result)); }
    if (command=='d') {
      log_line("RAW t_ms=%lu status=%s bytes=",(unsigned long)now,tt_io_name(imu_status));
      if (imu_status==TT_IO_OK) for (unsigned i=0;i<14;++i) log_line("%02X",sample.raw[i]);
      log_line("\r\n");
    }
  }
}
