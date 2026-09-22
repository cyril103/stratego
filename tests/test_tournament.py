"""Scoring checks, independent of expensive AI games."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('tournament', Path(__file__).resolve().parents[1] / 'tools/tournament.py')
tournament = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tournament)


class ScoringTests(unittest.TestCase):
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
