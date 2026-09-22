"""Compare paired tournament results and locate the first changed move."""
import argparse
import json
from pathlib import Path


def compare(old, new):
    before = json.loads((old / 'results.json').read_text())
    after = json.loads((new / 'results.json').read_text())
    def key(m):
        return m['a'], m['b'], m['seed'], m.get('formation', 'auto'), m['side']
    indexed = {key(m): m for m in before['matches']}
    rows = []
    for match in after['matches']:
        previous = indexed.get(key(match))
        if previous is None:
            continue
        a = [json.loads(line) for line in Path(previous['replay']).read_text().splitlines()]
        b = [json.loads(line) for line in Path(match['replay']).read_text().splitlines()]
        if a[0]['board'] != b[0]['board']:
            raise ValueError('Different starting boards')
        first = next(((x, y) for x, y in zip(a[1:], b[1:])
                      if (x.get('from'), x.get('to')) != (y.get('from'), y.get('to'))), None)
        rows.append(dict(seed=match['seed'], side=match['side'],
                         before=previous['winning_model'], after=match['winning_model'],
                         before_plies=previous['ply'], after_plies=match['ply'],
                         first_change=first))
    return rows


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(args.before, args.after), indent=2))
