"""Paired, resumable league with held-out seeds and explicitly censored games."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
OPPONENTS = ['reference', 'classic', 'raider', 'miner', 'cautious']


def schedule(opponents, seeds, formations):
    return [(enemy, seed, formation, side) for enemy in opponents for seed in seeds for formation in formations for side in (0, 1)]


def wilson(wins, n):
    if not n:
        return [0., 1.]
    z = 1.95996398454
    center = (wins/n + z*z/(2*n))/(1+z*z/n)
    radius = z*math.sqrt(wins/n*(1-wins/n)/n + z*z/(4*n*n))/(1+z*z/n)
    return [center-radius, center+radius]


def summary(matches):
    wins = sum(m['winner'] == m['side'] for m in matches)
    losses = sum(m['winner'] == 1-m['side'] for m in matches)
    draws = sum(m['winner'] == 2 for m in matches)
    unfinished = sum(m['winner'] < 0 for m in matches)
    n = len(matches)
    decisions=sum(m.get('decisions',[0,0])[0] for m in matches)
    seconds=sum(m.get('decision_seconds',[0,0])[0] for m in matches)
    return dict(games=n, wins=wins, losses=losses, draws=draws, unfinished=unfinished,
                candidate_mean_ms=1000*seconds/decisions if decisions else None,
                decisive_win_interval_95=wilson(wins, wins+losses),
                censored_score_bounds=[(wins+.5*draws)/n, (wins+.5*draws+unfinished)/n] if n else [0., 1.])


def save(out, manifest, matches):
    by_opponent = {name: summary([m for m in matches if m['opponent']==name]) for name in manifest['opponents']}
    data = dict(protocol=manifest, matches=matches, overall=summary(matches), by_opponent=by_opponent)
    temp = out / 'results.tmp'
    temp.write_text(json.dumps(data, indent=2), encoding='utf-8')
    temp.replace(out / 'results.json')
    lines = ['# Ligue Stratego', '', f"{len(matches)}/{manifest['games']} parties ; budget {manifest['build']['scale']} % ; graines {manifest['seed']}..{manifest['seed']+manifest['pairs']-1}.",
             'Camps inverses, placements identiques. Un plafond de demi-coups produit une partie inachevee, jamais une victoire estimee.',
             'Les intervalles sur les parties decisives excluent les inachevees. Les bornes de score les comptent respectivement comme defaites ou victoires.', '',
             '| Adversaire | V | D | N | Inachevees | Bornes de score |', '|---|---|---|---|---|---|']
    for name, row in by_opponent.items():
        lo, hi = row['censored_score_bounds']
        lines.append(f"| {name} | {row['wins']} | {row['losses']} | {row['draws']} | {row['unfinished']} | {lo:.1%}–{hi:.1%} |")
    lines += ['', 'Ces adversaires automatiques ne constituent pas une population humaine. Aucun Elo humain ni taux de victoire contre des humains ne peut etre deduit de ce tableau.']
    (out / 'report.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT/'build-campaign')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--pairs', type=int, default=10)
    parser.add_argument('--seed', type=int, default=17000)
    parser.add_argument('--plies', type=int, default=1200)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--opponents', nargs='+', choices=OPPONENTS, default=OPPONENTS)
    parser.add_argument('--formations', nargs='+', choices=['stable','random','evolved'], default=['stable','random'])
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--resume', action='store_true')
    args = parser.parse_args()
    if min(args.pairs,args.seed,args.plies,args.jobs)<1 or len(set(args.opponents))!=len(args.opponents) or len(set(args.formations))!=len(args.formations):
        parser.error('positive counts and unique opponents/formations required')
    out=args.output.resolve()
    tasks=schedule(args.opponents, range(args.seed,args.seed+args.pairs),args.formations)
    settings=dict(pairs=args.pairs,seed=args.seed,plies=args.plies,opponents=args.opponents,formations=args.formations,baseline=args.baseline)
    if args.resume:
        previous=json.loads((out/'results.json').read_text())
        manifest=previous['protocol'];matches=previous['matches']
        if any(manifest[k]!=v for k,v in settings.items()):
            parser.error('resume settings differ from the saved protocol')
    else:
        out.mkdir(parents=True,exist_ok=False)
        shutil.copy2(args.build/'league.exe',out/'league.exe')
        build=json.loads((args.build/'build.json').read_text())
        manifest=dict(settings,games=len(tasks),build=build,jobs=args.jobs)
        matches=[]
    binary=out/'league.exe'
    if hashlib.sha256(binary.read_bytes()).hexdigest()!=manifest['build']['binary_sha256']:
        raise ValueError('Arena binary does not match its build manifest')
    done={(m['opponent'],m['seed'],m['formation'],m['side']) for m in matches}
    save(out,manifest,matches)
    def play(task):
        enemy,seed,formation,side=task
        replay=out/f'{enemy}_{seed}_{formation}_{side}.jsonl'
        started=time.monotonic()
        result=subprocess.run([str(binary),str(seed),str(side),str(args.plies),enemy,formation,str(int(args.baseline)),str(replay)],
                              cwd=ROOT,capture_output=True,text=True,check=True)
        row=json.loads(result.stdout.strip().splitlines()[-1])
        with replay.open() as f:
            board=json.loads(f.readline())['board']
        return dict(row,opponent=enemy,seed=seed,formation=formation,side=side,seconds=round(time.monotonic()-started,3),
                    initial_sha256=hashlib.sha256(json.dumps(board).encode()).hexdigest(),replay=replay.name)
    starts={(m['seed'],m['formation']):m['initial_sha256'] for m in matches}
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures=[pool.submit(play,task) for task in tasks if task not in done]
        for future in as_completed(futures):
            row=future.result()
            if starts.setdefault((row['seed'],row['formation']),row['initial_sha256'])!=row['initial_sha256']:
                raise ValueError('Paired starting boards differ')
            matches.append(row);matches.sort(key=lambda m:(m['opponent'],m['seed'],m['formation'],m['side']))
            save(out,manifest,matches)
            print(f"{len(matches)}/{len(tasks)} {row['opponent']} seed{row['seed']} side{row['side']} winner{row['winner']} ply{row['ply']}",flush=True)
    print(json.dumps(summary(matches)))


if __name__=='__main__':
    main()
