#!/usr/bin/env python3
"""Check portable include rules and the project's fixed simulator wiring."""
import json
import pathlib
import re
root = pathlib.Path(__file__).resolve().parents[1]
allowed = {'stddef.h','stdint.h','stdbool.h','string.h','math.h','timing.h','trainer.h','mpu6050.h'}
for directory in ('Sensors','Trainer'):
    for path in (root/'firmware'/directory).glob('*.[ch]'):
        for include in re.findall(r'#\s*include\s*[<"]([^>"]+)',path.read_text()):
            assert include in allowed, f'{path}: forbidden dependency {include}'
        assert not re.search(r'\b(?:HAL_\w+|GPIO[A-F]|__HAL_\w+)\b',path.read_text()), f'{path}: hardware symbol'
diagram = json.loads((root/'diagram.json').read_text())
edges = {frozenset(edge[:2]) for edge in diagram['connections']}
required = [('imu:SCL','nucleo:PB8'),('imu:SDA','nucleo:PB9'),('limit:SIG','nucleo:PA0'),('base:1.l','nucleo:PB0'),('base:2.l','nucleo:GND.1')]
required += [(device+':VCC','nucleo:3V3.1') for device in ('imu','limit')]
required += [(f'logic:D{i}','nucleo:'+pin) for i,pin in enumerate(['PB8','PB9','PA2','PB0','PA4','PA5','PA6'])]
for edge in required:
    assert frozenset(edge) in edges, f'missing wire {edge}'
ioc = (root/'firmware/firmware.ioc').read_text()
for item in ['PA0.Signal=ADC_IN0','PB0.Signal=GPIO_Input','PB8.Signal=I2C1_SCL','PB9.Signal=I2C1_SDA','PA4.Signal=GPIO_Output','PA6.Signal=GPIO_Output','PA5.PinState=GPIO_PIN_RESET']:
    assert item in ioc, f'missing IOC setting {item}'
print('PASS portable dependencies and TiltTrainer wiring/IOC')
