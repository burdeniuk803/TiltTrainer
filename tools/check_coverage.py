#!/usr/bin/env python3
"""Enforce the documented GCC goal on project modules only."""
import pathlib
import re
import sys
for module in ('mpu6050','trainer','timing'):
    path = pathlib.Path(sys.argv[1])/(module+'.c.gcov')
    lines = path.read_text().splitlines()
    executable = hit = branches = taken = 0
    for line in lines:
        match = re.match(r'\s*([^:]+):\s*([0-9]+):',line)
        if match and int(match[2]) and match[1].strip() != '-':
            executable += 1
            if match[1].strip().replace('*','').isdigit() and int(match[1].strip().replace('*','')):
                hit += 1
        if re.match(r'branch\s+\d+',line):
            branches += 1
            match = re.search(r'taken (\d+)',line)
            if match and int(match[1]): taken += 1
    line_percent = 100*hit/executable
    branch_percent = 100*taken/branches
    print(f'{module}: lines={hit}/{executable} {line_percent:.2f}% branch_taken={taken}/{branches} {branch_percent:.2f}%')
    assert line_percent >= 90 and branch_percent >= 80, f'coverage below goal: {module}'
