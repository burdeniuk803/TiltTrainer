#!/usr/bin/env python3
"""Decode Wokwi's sparse digital VCD; keep captures distinct from host fixtures."""
import argparse
import bisect
import hashlib
import json
from pathlib import Path


def read(path):
    header=[]
    names={}
    edges={}
    changes={}
    time=0
    body=False
    with path.open() as stream:
        for line in stream:
            line=line.strip()
            if not body:
                header.append(line)
                if line.startswith('$var'):
                    fields=line.split()
                    names[fields[3]]=fields[4]
                    edges[fields[3]]=[]
                if line=='$enddefinitions $end':body=True
                continue
            if line.startswith('#'):
                new=int(line[1:])
                if new != time:
                    for signal,value in changes.items():
                        if not edges[signal] or edges[signal][-1][1] != value:edges[signal].append((time,value))
                    changes={}
                    time=new
            elif len(line)==2 and line[0] in '01' and line[1] in names:
                changes[line[1]]=int(line[0])
    for signal,value in changes.items():
        if not edges[signal] or edges[signal][-1][1] != value:edges[signal].append((time,value))
    return header,names,edges,time


def i2c(edges,scl,sda):
    times=sorted(set(t for signal in (scl,sda) for t,_ in edges[signal]))
    indices={scl:0,sda:0}
    state={scl:1,sda:1}
    frame=None
    bits=[]
    result=[]
    for time in times:
        previous=state.copy()
        for signal in (scl,sda):
            index=indices[signal]
            if index<len(edges[signal]) and edges[signal][index][0]==time:
                state[signal]=edges[signal][index][1]
                indices[signal]=index+1
        if previous[scl]==state[scl]==1 and previous[sda]!=state[sda]:
            if state[sda]==0:
                if frame:result.append(frame)
                frame={'start_ns':time,'bytes':[],'acks':[]}
                bits=[]
            elif frame:
                frame['stop_ns']=time
                result.append(frame)
                frame=None
        if frame is not None and previous[scl]==0 and state[scl]==1:
            bits.append(state[sda])
            if len(bits)==9:
                frame['bytes'].append(sum(value<<(7-i) for i,value in enumerate(bits[:8])))
                frame['acks'].append(bits[8]==0)
                bits=[]
    if frame:result.append(frame)
    return result


def uart(edges,baud=115200):
    period=1e9/baud
    times=[time for time,_ in edges]
    def level(time):
        index=bisect.bisect_right(times,time)-1
        return edges[index][1] if index>=0 else 1
    result=[]
    next_start=0
    for time,value in edges:
        if value or time<next_start or time==0:continue
        if level(time+9.5*period)!=1:continue
        byte=sum(level(time+(1.5+bit)*period)<<bit for bit in range(8))
        result.append((time,byte))
        next_start=time+9.8*period
    return result


def intervals(edges,value=1):
    times=[time for time,level in edges if level==value and time>0]
    return [(b-a)/1e6 for a,b in zip(times,times[1:])]


def clip(path,target,start,end):
    header,names,edges,_=read(path)
    state={signal:None for signal in names}
    events=[]
    for signal,values in edges.items():
        for time,value in values:
            if time<start:state[signal]=value
            elif time<=end:events.append((time,signal,value))
            else:break
    if any(value is None for value in state.values()):
        raise ValueError('Source lacks initial levels; choose a later clip or a complete capture')
    lines=header[:-1]+[f'$comment Source {path.name}; offset_ns={start}; SHA256={hashlib.sha256(path.read_bytes()).hexdigest()} $end',header[-1],'#0']
    lines += [f'{value}{signal}' for signal,value in state.items()]
    last=0
    for time,signal,value in sorted(events):
        shifted=time-start
        if shifted!=last:lines.append(f'#{shifted}');last=shifted
        lines.append(f'{value}{signal}')
        state[signal]=value
    # Retain the requested quiet tail; repeated levels do not add transitions.
    lines.append(f'#{end-start}')
    lines += [f'{value}{signal}' for signal,value in state.items()]
    target.write_text('\n'.join(lines)+'\n')


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--clip-ms',nargs=2,type=float)
    args=parser.parse_args()
    if args.clip_ms:
        clip(args.source,args.output,*[int(ms*1e6) for ms in args.clip_ms])
        return
    _,names,edges,duration=read(args.source)
    by_name={name:signal for signal,name in names.items()}
    transactions=i2c(edges,by_name['SCL'],by_name['SDA'])
    decoded=uart(edges[by_name['UART_TX']])
    args.output.with_suffix('.uart.txt').write_text(''.join(chr(byte) if byte in (9,10,13) or 32 <= byte < 127 else f'\\x{byte:02x}' for _,byte in decoded))
    unique={}
    for frame in transactions:
        if len(frame['bytes'])==15 and frame['bytes'][0]==0xD1:
            raw=bytes(frame['bytes'][1:]).hex().upper()
            unique.setdefault(raw,frame['start_ns']/1e6)
    stats={}
    for name in ('ADC_MARK','MPU_MARK'):
        differences=intervals(edges[by_name[name]])
        stats[name]={'count':len(differences)+1,'min_ms':min(differences),'max_ms':max(differences)} if differences else {}
    pulses=[]
    for time,value in edges[by_name['ADC_MARK']]:
        if value:pulses.append([time])
        elif pulses and len(pulses[-1])==1:pulses[-1].append(time)
    widths=[(pulse[1]-pulse[0])/1e3 for pulse in pulses if len(pulse)==2]
    result={'source':str(args.source),'sha256':hashlib.sha256(args.source.read_bytes()).hexdigest(),'duration_ms':duration/1e6,'transactions':len(transactions),'startup':transactions[:12],'unique_bursts':unique,'periods':stats,'adc_mark_width_us':{'min':min(widths),'max':max(widths)}}
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':main()
