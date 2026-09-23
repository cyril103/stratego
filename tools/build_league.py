"""Build an isolated arena against a Git-frozen reference, at identical search budgets."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def run(args):
    subprocess.run(args, cwd=ROOT, check=True)


def build(out, reference, scale):
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    frozen = out / 'reference'
    frozen.mkdir(exist_ok=True)
    commit = subprocess.check_output(['git', 'rev-parse', reference], cwd=ROOT, text=True).strip()
    names = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', commit, 'src'], cwd=ROOT, text=True).splitlines()
    # Game's public memory may grow. Both engines use the current Game ABI;
    # the reference's decision code and all its private headers stay frozen.
    for name in names:
        path = Path(name)
        if path.name.startswith('ai') and path.suffix in ('.c', '.h'):
            data = subprocess.check_output(['git', 'show', f'{commit}:{name}'], cwd=ROOT).decode('utf-8')
            if path.name == 'ai.c':
                for old, new in [('.budget=600', '.budget=LEAGUE_BUDGET(600)'),
                                 ('deepen?6400:2400', 'deepen?LEAGUE_BUDGET(6400):LEAGUE_BUDGET(2400)')]:
                    if data.count(old) != 1:
                        raise ValueError(f'Reference budget anchor changed: {old}')
                    data = data.replace(old, new)
                data = f'#define LEAGUE_BUDGET(n) ((n)*{scale}/100)\n' + data
            if path.name == 'ai_deploy.c':
                data = data.replace('AI_FORMATIONS', '6')
            (frozen / path.name).write_text(data, encoding='utf-8')
    reference_obj = out / 'reference.o'
    deploy_obj = out / 'reference_deploy.o'
    base = ['gcc', '-O2', '-std=c99', '-Isrc', '-Wall', '-Wextra', '-Wpedantic']
    run(base + ['-Dai_choose=ai_reference', '-c', str(frozen / 'ai.c'), '-o', str(reference_obj)])
    run(base + ['-Dai_deploy=ai_deploy_reference', '-Dai_deploy_template=ai_deploy_template_reference',
                '-c', str(frozen / 'ai_deploy.c'), '-o', str(deploy_obj)])
    # Instrument a private copy, never the installed engine. Search scale 100
    # is native; smaller values are screening experiments, not playing strength.
    candidate = (ROOT / 'src/ai.c').read_text()
    for old, new in [('.budget=600', f'.budget={600*scale//100}'),
                     ('deepen?6400:2400', f'deepen?{6400*scale//100}:{2400*scale//100}')]:
        if candidate.count(old) != 1:
            raise ValueError(f'Candidate budget anchor changed: {old}')
        candidate = candidate.replace(old, new)
    source = out / 'candidate.c'
    source.write_text(candidate, encoding='utf-8')
    binary = out / 'league.exe'
    run(base + ['tools/league.c', str(source), str(reference_obj), str(deploy_obj), 'tests/ai_previous.c',
                'src/game.c', 'src/ai_deploy.c', 'src/ai_basic.c', 'src/ml.c', 'src/ai_parallel.c',
                'src/ai_strategy.c', '-o', str(binary), '-lm'])
    hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted((ROOT / 'src').glob('*')) if p.is_file()}
    metadata = dict(reference=commit, scale=scale, compiler=subprocess.check_output(['gcc', '--version'], text=True).splitlines()[0],
                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(), source_hashes=hashes)
    (out / 'build.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    return binary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build-campaign')
    parser.add_argument('--reference', default='09936c8')
    parser.add_argument('--scale', type=int, default=100, choices=[25, 50, 100])
    args = parser.parse_args()
    print(build(args.output, args.reference, args.scale))
