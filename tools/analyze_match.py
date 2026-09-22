"""Replay a diagnostic JSONL match using only stdlib; inspect public knowledge."""
import json
import sys
from pathlib import Path
names = ['Drapeau','Espion','Eclaireur','Demineur','Sergent','Lieutenant','Capitaine','Commandant','Colonel','General','Marechal','Bombe']
rows = [json.loads(s) for s in Path(sys.argv[1]).read_text().splitlines()]
board = [dict(side=s, rank=r, id=i, known=False, moved=False) for s,r,i in rows[0]['board']]
checkpoints = {int(s) for s in sys.argv[2:]} or {44,50,82,120,150,160,162,164,168,170,172,174}
for entry in rows[1:]:
    if 'end' in entry:
        print('END',entry);continue
    ply=entry['ply']
    if ply in checkpoints:
        print('\nBEFORE',ply)
        for y in range(10):
            print(' '.join('  .. ' if p['side']<0 else f'{"H" if p["side"]==0 else "A"}{p["rank"]:02}{"!" if p["known"] else "?"} ' for p in board[y*10:y*10+10]))
    a,d=board[entry['from']].copy(),board[entry['to']].copy()
    assert a['side']==entry['side'] and a['rank']==entry['attacker'] and d['rank']==entry['defender']
    if entry['combat']!=2:
        print(f'{ply:3} {"H" if a["side"]==0 else "AI"} {names[a["rank"]]} {entry["from"]}->{entry["to"]} vs {names[d["rank"]]} ({"KNOWN" if d["known"] else "hidden"}) result {entry["combat"]}')
        a['known']=d['known']=True
    a['moved']=True
    if abs(entry['from']//10-entry['to']//10)+abs(entry['from']%10-entry['to']%10)>1:a['known']=True
    empty=dict(side=-1,rank=-1,id=-1,known=False,moved=False)
    board[entry['from']]=empty.copy()
    board[entry['to']]=a if entry['combat'] in (1,2) else d if entry['combat']==-1 else empty.copy()
