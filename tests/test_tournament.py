"""Scoring checks, independent of expensive AI games."""
import importlib.util
from pathlib import Path
import unittest
import tempfile
import json

spec = importlib.util.spec_from_file_location('tournament', Path(__file__).resolve().parents[1] / 'tools/tournament.py')
tournament = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tournament)
compare_spec = importlib.util.spec_from_file_location('compare', Path(__file__).resolve().parents[1] / 'tools/compare_tournaments.py')
comparison = importlib.util.module_from_spec(compare_spec)
compare_spec.loader.exec_module(comparison)


class ScoringTests(unittest.TestCase):
    def test_expanded_schedule_is_paired_and_unique(self):
        tasks = tournament.schedule([1, 2, 3], [204, 205], ['auto', 'random'], 1)
        self.assertEqual(len(tasks), 16)
        self.assertEqual(len(set(tasks)), 16)
        for a, b, seed, side, formation in tasks:
            self.assertIn(1, (a, b))
            self.assertIn((a, b, seed, 1-side, formation), tasks)

    def test_comparison_separates_opponents_and_formations(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            old, new = root / 'old', root / 'new'
            old.mkdir(); new.mkdir()
            matches = []
            for index, (opponent, formation) in enumerate([(2, 'auto'), (3, 'auto'), (3, 'random')]):
                replay = root / f'{index}.jsonl'
                replay.write_text(json.dumps({'board': [index]}) + '\n')
                matches.append(dict(a=1, b=opponent, formation=formation, seed=204, side=0,
                                    winning_model=1, ply=100, replay=str(replay)))
            for path in (old, new):
                (path / 'results.json').write_text(json.dumps({'matches': matches}))
            rows = comparison.compare(old, new)
            self.assertEqual(len(rows), 3)
            self.assertTrue(all(row['first_change'] is None for row in rows))

    def test_duel_only_lists_participants(self):
        rows = tournament.standings([], [1, 3])
        self.assertEqual({r['model'] for r in rows}, {1, 3})

    def test_win_draw_and_unfinished_are_distinct(self):
        matches = [
            dict(a=0, b=1, winner=1, winning_model=0),
            dict(a=0, b=1, winner=2, winning_model=None),
            dict(a=0, b=1, winner=-1, winning_model=None),
        ]
        rows = {r['model']: r for r in tournament.standings(matches)}
        self.assertEqual((rows[0]['points'], rows[1]['points']), (1.5, 0.5))
        self.assertEqual((rows[0]['wins'], rows[1]['losses']), (1, 1))
        for model in (0, 1):
            self.assertEqual(rows[model]['played'], 2)
            self.assertEqual(rows[model]['draws'], 1)
            self.assertEqual(rows[model]['unfinished'], 1)

    def test_complete_round_robin_conserves_points(self):
        matches = [dict(a=a, b=b, winner=2, winning_model=None)
                   for a, b in tournament.itertools.combinations(tournament.NAMES, 2)
                   for side in (0, 1)]
        rows = tournament.standings(matches)
        self.assertEqual(sum(r['points'] for r in rows), 12)
        self.assertTrue(all(r['played'] == 6 and r['points'] == 3 for r in rows))


if __name__ == '__main__':
    unittest.main()
