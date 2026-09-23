import importlib.util
from pathlib import Path
import unittest

spec=importlib.util.spec_from_file_location('league',Path(__file__).resolve().parents[1]/'tools/league.py')
league=importlib.util.module_from_spec(spec)
spec.loader.exec_module(league)
spec=importlib.util.spec_from_file_location('compare_league',Path(__file__).resolve().parents[1]/'tools/compare_league.py')
comparison=importlib.util.module_from_spec(spec)
spec.loader.exec_module(comparison)


class LeagueTests(unittest.TestCase):
    def test_balanced_unique_schedule(self):
        tasks=league.schedule(league.OPPONENTS,range(10,20),['stable','random'])
        self.assertEqual(len(tasks),200)
        self.assertEqual(len(set(tasks)),200)
        for enemy,seed,formation,side in tasks:
            self.assertIn((enemy,seed,formation,1-side),tasks)

    def test_censoring_never_becomes_a_win_or_draw(self):
        rows=[dict(side=0,winner=0),dict(side=1,winner=0),dict(side=0,winner=2),dict(side=1,winner=-1)]
        result=league.summary(rows)
        self.assertEqual([result[k] for k in ('wins','losses','draws','unfinished')],[1,1,1,1])
        self.assertEqual(result['censored_score_bounds'],[.375,.625])
        self.assertEqual(league.summary([])['censored_score_bounds'],[0,1])

    def test_interval_remains_uncertain_for_small_samples(self):
        self.assertLess(league.wilson(10,10)[0],.75)
        self.assertEqual(league.wilson(0,0),[0,1])
        lo,hi=league.wilson(50,100)
        self.assertAlmostEqual(lo,1-hi)

    def test_paired_comparison_preserves_censoring_and_checks_boards(self):
        protocol=dict(pairs=1,seed=1,plies=100,opponents=['reference'],formations=['stable'],build=dict(reference='abc',scale=100))
        row=dict(opponent='reference',seed=1,formation='stable',side=0,initial_sha256='same')
        old=dict(protocol=protocol,matches=[dict(row,winner=-1)])
        new=dict(protocol=protocol,matches=[dict(row,winner=0)])
        result=comparison.compare(old,new)
        self.assertEqual(result['all_game_delta_bounds'],[0,1])
        self.assertEqual(result['both_finished'],0)
        new['matches'][0]['initial_sha256']='changed'
        with self.assertRaises(ValueError):
            comparison.compare(old,new)


if __name__=='__main__':
    unittest.main()
