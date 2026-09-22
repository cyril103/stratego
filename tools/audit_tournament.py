"""Audit recorded moves; hidden ranks are used for hindsight statistics only."""
import argparse
import collections
import json
from pathlib import Path

VALUES = [0, 7, 4, 12, 7, 9, 12, 17, 24, 35, 50, 0]
NAMES = ['drapeau', 'espion', 'eclaireur', 'demineur', 'sergent', 'lieutenant',
         'capitaine', 'commandant', 'colonel', 'general', 'marechal', 'bombe']


def audit(path):
    events = [json.loads(line) for line in path.read_text().splitlines()]
    header = events[0]
    board = [dict(side=s, rank=r, id=i, known=False, moved=False) for s, r, i in header['board']]
    improved = header['new_side']
    losses, combats, scout_disclosures = [], [], []
    seen = collections.Counter()
    quiet = longest_quiet = last_combat = 0
    peak = advantage = 0
    material = []
    for event in events[1:]:
        if event.get('end'):
            break
        source, target, ply, result = event['from'], event['to'], event['ply'], event['combat']
        attacker, defender = board[source].copy(), board[target].copy()
        assert attacker['side'] == event['side'] and attacker['rank'] == event['attacker']
        assert defender['rank'] == event['defender']
        if result != 2:
            combat = dict(ply=ply, side=attacker['side'], source=source, target=target,
                          attacker=NAMES[attacker['rank']], defender=NAMES[defender['rank']],
                          attacker_known=attacker['known'],
                          defender_known=defender['known'], defender_moved=defender['moved'], result=result)
            combats.append(combat)
            for piece, died, role in ((attacker, result <= 0, 'attacker'), (defender, result >= 0, 'defender')):
                if died and piece['side'] == improved:
                    losses.append(dict(**combat, lost_rank=piece['rank'], lost=NAMES[piece['rank']], role=role))
            attacker['known'] = defender['known'] = True
            quiet = 0
            last_combat = ply
        else:
            quiet += 1
            longest_quiet = max(longest_quiet, quiet)
        length = abs(source // 10 - target // 10) + abs(source % 10 - target % 10)
        if length > 1:
            if not attacker['known'] and result == 2 and attacker['side'] == improved:
                scout_disclosures.append(ply)
            attacker['known'] = True
        attacker['moved'] = True
        empty = dict(side=-1, rank=-1, id=-1, known=False, moved=False)
        board[source] = empty.copy()
        board[target] = attacker if result in (1, 2) else defender if result == -1 else empty.copy()
        scores = [sum(VALUES[p['rank']] for p in board if p['side'] == side) for side in (0, 1)]
        advantage = scores[improved] - scores[1-improved]
        peak = max(peak, advantage)
        if result != 2:
            material.append(dict(ply=ply, advantage=advantage))
        # A repeated diagram is diagnostic, not a claim of an illegal repetition.
        key = (1-event['side'], tuple((p['id'], p['known'], p['moved']) for p in board))
        seen[key] += 1
    end = events[-1] if events[-1].get('end') else None
    return dict(file=str(path), seed=header['seed'], side=improved, end=end,
                plies=events[-2]['ply'] if end else events[-1].get('ply', 0),
                peak_material_advantage=peak, final_material_advantage=advantage,
                longest_quiet_plies=longest_quiet, last_combat=last_combat,
                max_diagram_occurrences=max(seen.values(), default=0),
                quiet_scout_disclosures=scout_disclosures, losses=losses,
                material=material, combats=combats,
                surviving_ranks=[[sum(piece['side'] == side and piece['rank'] == rank for piece in board)
                                  for rank in range(12)] for side in (0, 1)])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    args = parser.parse_args()
    audits = [audit(path) for path in sorted(args.folder.glob('*/match_*.jsonl'))]
    (args.folder / 'audit.json').write_text(json.dumps(audits, indent=2), encoding='utf-8')
    for item in audits:
        end = item['end']
        outcome = 'pending' if not end else 'unfinished' if end['winner'] < 0 else 'draw' if end['winner'] == 2 else 'win' if end['winner'] == item['side'] else 'loss'
        print(f"{item['seed']}/{item['side']}: {outcome}, {item['plies']} plies; material peak/final {item['peak_material_advantage']}/{item['final_material_advantage']}; quiet {item['longest_quiet_plies']}; repeated diagram {item['max_diagram_occurrences']}")
        for loss in item['losses']:
            if loss['lost_rank'] in (3, 8, 9, 10):
                print(' ', json.dumps(loss))
