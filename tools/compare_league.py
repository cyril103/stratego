"""Paired comparison: keep censored outcomes and bootstrap whole seed clusters."""
import argparse
import json
from pathlib import Path
import random


def score(row):
    if row['winner']<0:
        return None
    return .5 if row['winner']==2 else float(row['winner']==row['side'])


def compare(before, after, repeats=2000):
    for field in ('pairs','seed','plies','opponents','formations'):
        if before['protocol'][field]!=after['protocol'][field]:
            raise ValueError(f'Protocols differ: {field}')
    for field in ('reference','scale'):
        if before['protocol']['build'][field]!=after['protocol']['build'][field]:
            raise ValueError(f'Build protocols differ: {field}')
    def index(data):
        result={}
        for row in data['matches']:
            key=(row['opponent'],row['seed'],row['formation'],row['side'])
            if key in result:
                raise ValueError('Duplicate game')
            result[key]=row
        return result
    old,new=index(before),index(after)
    if old.keys()!=new.keys():
        raise ValueError('Compared games must match exactly')
    low,high,groups=0.,0.,{}
    for key,a in old.items():
        b=new[key]
        if a['initial_sha256']!=b['initial_sha256']:
            raise ValueError('Different initial boards')
        sa,sb=score(a),score(b)
        low+=(0 if sb is None else sb)-(1 if sa is None else sa)
        high+=(1 if sb is None else sb)-(0 if sa is None else sa)
        if sa is not None and sb is not None:
            groups.setdefault(a['seed'],[]).append(sb-sa)
    clusters=list(groups.values());rng=random.Random(1729);samples=[]
    if len(clusters)>=2:
        for _ in range(repeats):
            drawn=[rng.choice(clusters) for _ in clusters]
            samples.append(sum(sum(c) for c in drawn)/sum(len(c) for c in drawn))
        samples.sort()
    deltas=[d for c in clusters for d in c]
    return dict(paired_games=len(old),both_finished=len(deltas),finished_seed_clusters=len(clusters),
                finished_score_delta=sum(deltas)/len(deltas) if deltas else None,
                finished_cluster_interval_95=[samples[int(.025*repeats)],samples[min(repeats-1,int(.975*repeats))]] if samples else None,
                all_game_delta_bounds=[low/len(old),high/len(old)] if old else [-1.,1.],
                caution='Finished-only results may be biased by censoring. Seed clusters, not individual moves, are resampled.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before',type=Path)
    parser.add_argument('after',type=Path)
    args=parser.parse_args()
    print(json.dumps(compare(json.loads(args.before.read_text()),json.loads(args.after.read_text())),indent=2))
