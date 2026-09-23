"""Replay audit of concrete mistakes, keeping hidden identities out of public labels."""
import argparse
from collections import Counter
import json
from pathlib import Path


def audit_replay(path, candidate):
    with Path(path).open() as stream:
        rows = [json.loads(line) for line in stream if line.strip()]
    board = [dict(side=s, rank=r, id=i, revealed=False, moved=False) for s, r, i in rows[0]['board']]
    events = []
    disclosures = {}
    quiet = longest_quiet = 0
    returns = Counter()
    def event(kind, row, **extra):
        events.append(dict(kind=kind, ply=row['ply'], move=[row['from'], row['to']], **extra))
    for row in rows[1:]:
        if 'from' not in row:
            continue
        start, end, result = row['from'], row['to'], row['combat']
        quiet = quiet+1 if result == 2 else 0
        longest_quiet = max(longest_quiet, quiet)
        a, d = board[start].copy(), board[end].copy()
        if result != 2:
            known = d['revealed']
            if a['side'] == candidate and d['rank'] == 11 and known and a['rank'] != 3:
                event('non_miner_attacks_known_bomb', row, rank=a['rank'])
            for piece, dies, attacking, other in [(a, result <= 0, True, d), (d, result >= 0, False, a)]:
                if piece['side'] != candidate:
                    continue
                if piece['rank'] in (9, 10) and not piece['revealed']:
                    disclosures.setdefault(str(piece['rank']), row['ply'])
                if not dies:
                    continue
                if piece['rank'] == 1 and any(p['side'] == 1-candidate and p['rank'] == 10 for p in board):
                    event('spy_lost_with_enemy_marshal_alive', row, attacking=attacking, enemy_known=other['revealed'])
                if piece['rank'] == 10:
                    event('marshal_lost', row, attacking=attacking, enemy_rank=other['rank'], enemy_known=other['revealed'])
                if piece['rank'] == 0 and other['rank'] == 2:
                    event('flag_taken_by_scout', row)
            a['revealed'] = d['revealed'] = True
        a['moved'] = True
        if abs(start % 10-end % 10)+abs(start // 10-end // 10) > 1:
            a['revealed'] = True
        board[start] = dict(side=-1, rank=-1, id=-1, revealed=False, moved=False)
        board[end] = a if result > 0 else d if result < 0 else board[start].copy()
        # Public board returns are a stalling diagnostic, not a repetition
        # rule: two-square restrictions and turn history can still differ.
        key = (1-a['side'], tuple((p['id'], p['revealed'], p['moved']) for p in board))
        returns[key] += 1
    return dict(events=events, counts=dict(Counter(e['kind'] for e in events)), first_officer_disclosures=disclosures,
                longest_quiet_plies=longest_quiet, trailing_quiet_plies=quiet, max_public_board_visits=max(returns.values(), default=0))


def audit_league(directory):
    directory = Path(directory)
    results = json.loads((directory/'results.json').read_text())
    counts = Counter()
    matches = []
    for row in results['matches']:
        audit = audit_replay(directory/row['replay'], row['side'])
        counts.update(audit['counts'])
        matches.append(dict(replay=row['replay'], winner=row['winner'], side=row['side'], **audit))
    return dict(games=len(matches), counts=dict(counts), matches=matches,
                caution='Events are diagnostic signals, not automatically errors: a sacrifice can win the game. Inspect its public context.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    report = audit_league(args.directory)
    (args.directory/'audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k != 'matches'}, indent=2))
