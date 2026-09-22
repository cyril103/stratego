"""Reproducible round robin using the models selected by the game menu."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import shutil
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

NAMES = {0: 'Decouverte', 1: 'Expert+ Improved', 2: 'IA entrainee', 3: 'Expert+ Classique'}
REASONS = {1: 'drapeau', 2: 'immobilisation', 3: 'double immobilisation', 4: 'capitulation', 5: 'accord', 6: 'limite de temps'}
ROOT = Path(__file__).resolve().parents[1]


def standings(matches, models=None):
    rows = {i: dict(model=i, name=NAMES[i], played=0, wins=0, losses=0, draws=0, unfinished=0, points=0.0) for i in (NAMES if models is None else models)}
    for m in matches:
        for model in (m['a'], m['b']):
            row = rows[model]
            if m['winner'] < 0:
                row['unfinished'] += 1
                continue
            row['played'] += 1
            if m['winner'] == 2:
                row['draws'] += 1
                row['points'] += 0.5
            elif m['winning_model'] == model:
                row['wins'] += 1
                row['points'] += 1
            else:
                row['losses'] += 1
    return sorted(rows.values(), key=lambda r: (-r['points'], r['model']))


def save_report(out, matches, manifest):
    models = [int(i) for i in manifest['models']]
    rows = standings(matches, models)
    duration = f"Duree maximale : {manifest['seconds']} secondes par partie." if manifest['seconds'] else 'Sans limite de temps, y compris dans le lanceur.'
    (out / 'results.json').write_text(json.dumps(dict(protocol=manifest, matches=matches, standings=rows), indent=2), encoding='utf-8')
    lines = ['# Tournoi Stratego', '', f"{len(matches)} / {manifest['games']} parties terminees ou arretees.", '',
             f"Aller-retour, {manifest['pairs']} placement(s) par confrontation. {duration} Plafond technique : {manifest['ply_limit']} demi-coups.",
             'Victoire : 1 point ; nulle : 0,5 ; defaite : 0. Les parties inachevees au plafond de demi-coups ne rapportent aucun point et restent signalees.',
             'Les ex aequo restent ex aequo. Comparatif des reglages du jeu, sans egalisation du temps de calcul par coup. Petit echantillon : ce classement ne mesure pas un Elo.', '',
             '| Rang | Modele | J | V | N | D | Inachevees | Points |', '|---|---|---|---|---|---|---|---|']
    last, rank = None, 0
    for index, row in enumerate(rows, 1):
        if row['points'] != last:
            rank, last = index, row['points']
        lines.append(f"| {rank} | {row['name']} | {row['played']} | {row['wins']} | {row['draws']} | {row['losses']} | {row['unfinished']} | {row['points']:g} |")
    lines += ['', '| A | B | Camp A | Demi-coups | Resultat | Motif |', '|---|---|---|---|---|---|']
    for m in matches:
        result = 'inachevee' if m['winner'] < 0 else 'nulle' if m['winner'] == 2 else NAMES[m['winning_model']]
        lines.append(f"| {NAMES[m['a']]} | {NAMES[m['b']]} | {m['side']} | {m['ply']} | {result} | {REASONS.get(m['reason'], 'plafond de demi-coups')} |")
    lines += ['', '| Modele | Calcul moyen par decision | Decisions |', '|---|---|---|']
    for model in models:
        name = NAMES[model]
        seconds, count = 0, 0
        for m in matches:
            for role in ('a', 'b'):
                if m[role] == model:
                    seconds += m.get('seconds_' + role, 0)
                    count += m.get('decisions_' + role, 0)
        lines.append(f'| {name} | {1000*seconds/max(1, count):.3f} ms | {count} |')
    (out / 'classement.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-tournament' / 'ai_benchmark.exe')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--pairs', type=int, default=1)
    parser.add_argument('--seed', type=int, default=912)
    parser.add_argument('--seconds', type=float, default=180)
    parser.add_argument('--plies', type=int, default=100000)
    parser.add_argument('--models', type=int, nargs='+', default=list(NAMES), choices=list(NAMES))
    parser.add_argument('--jobs', type=int, default=1, help='Concurrent games; only available without a clock')
    args = parser.parse_args()
    if args.pairs < 1 or args.seconds < 0 or args.plies < 1:
        parser.error('positive pairs and plies, nonnegative seconds required (0 disables the clock)')
    if len(args.models)<2 or len(set(args.models))!=len(args.models):
        parser.error('at least two distinct models required')
    if args.jobs < 1 or (args.jobs > 1 and args.seconds):
        parser.error('positive jobs required; concurrent games require --seconds 0')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    binary = out / 'ai_benchmark.exe'
    policy = out / 'selfplay.policy'
    shutil.copy2(args.binary, binary)
    shutil.copy2(ROOT / 'assets/models/selfplay.policy', policy)
    # Preserve the implementation associated with each experiment. The caller
    # must build before running; snapshots are evidence, not a substitute for it.
    source = out / 'source'
    source.mkdir()
    hashes = {}
    for path in sorted((ROOT / 'src').glob('*')):
        if path.is_file():
            shutil.copy2(path, source / path.name)
            hashes['src/' + path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
    for name in ('tests/ai_previous.c', 'tests/benchmark_ai.c', 'CMakeLists.txt', 'tools/tournament.py'):
        path = ROOT / name
        shutil.copy2(path, out / path.name)
        hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / 'source_hashes.json').write_text(json.dumps(hashes, indent=2), encoding='utf-8')
    manifest = dict(games=len(args.models)*(len(args.models)-1)*args.pairs, pairs=args.pairs, seed=args.seed, seconds=args.seconds, ply_limit=args.plies, jobs=args.jobs,
                    models={i:NAMES[i] for i in args.models}, policy_header=policy.read_text().splitlines()[0],
                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                    policy_sha256=hashlib.sha256(policy.read_bytes()).hexdigest(),
                    protocol='Same seeded board for every pairing; sides swapped; native model budgets; no draw offers. '+('Wall-time playing period.' if args.seconds else 'No time limit.'))
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    matches, starts = [], {}
    save_report(out, matches, manifest)
    tasks = [(a, b, seed, side) for a, b in itertools.combinations(args.models, 2)
             for seed in range(args.seed, args.seed + args.pairs) for side in (0, 1)]

    def play(index, task):
        a, b, seed, side = task
        folder = out / f'{a}_vs_{b}_{seed}_{side}'
        folder.mkdir()
        command = [str(binary), '1', str(args.plies), str(seed), str(side), '1', str(folder), str(args.seconds), str(a), str(b), str(policy)]
        print(f"GAME {index}/{manifest['games']}: {NAMES[a]} vs {NAMES[b]} seed={seed} side={side}", flush=True)
        started = time.monotonic()
        with (folder / 'stdout.txt').open('w', encoding='utf-8') as log:
            subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=args.seconds+120 if args.seconds else None)
        events = [json.loads(line) for line in (folder / f'match_{seed}_{side}.jsonl').read_text().splitlines()]
        initial, end = events[0], events[-1]
        if not end.get('end'):
            raise RuntimeError('Missing match result')
        winner = end['winner']
        match = dict(a=a, b=b, seed=seed, side=side, winner=winner, winning_model=(a if winner == side else b) if winner in (0, 1) else None,
                     ply=end['ply'], reason=end['reason'], seconds=round(time.monotonic()-started, 3),
                     seconds_a=end['seconds_a'], decisions_a=end['decisions_a'], seconds_b=end['seconds_b'], decisions_b=end['decisions_b'],
                     replay=str(folder / f'match_{seed}_{side}.jsonl'))
        return match, initial['board']

    # Only the collector writes reports. Clocked games remain strictly serial.
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(play, index, task) for index, task in enumerate(tasks, 1)]
        for future in as_completed(futures):
            match, board = future.result()
            if starts.setdefault(match['seed'], board) != board:
                raise RuntimeError('Initial boards differ across paired games')
            matches.append(match)
            matches.sort(key=lambda item: (item['a'], item['b'], item['seed'], item['side']))
            save_report(out, matches, manifest)
            print('RESULT ' + json.dumps(match), flush=True)
    print('COMPLETE ' + str(out / 'classement.md'), flush=True)


if __name__ == '__main__':
    main()
