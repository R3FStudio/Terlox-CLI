"""Check DCO sign-offs for a pull request's commits. No approval gate or tokens."""
import argparse
import re
import subprocess


def check(base,head):
    for revision in [base,head]:
        if not re.fullmatch(r'[0-9a-fA-F]{40,64}',revision): raise ValueError('Expected a full Git commit ID')
    result=subprocess.run(['git','log','--no-merges','--format=%H%x00%an%x00%ae%x00%B%x00%x1e',f'{base}..{head}'],check=True,capture_output=True,text=True)
    errors=[]
    for record in result.stdout.split('\x1e'):
        if not record.strip(): continue
        commit,name,email,body,*_=record.strip('\n').split('\x00')
        signed={(n.strip().casefold(),e.strip().casefold()) for n,e in re.findall(r'^Signed-off-by:\s*(.+?)\s*<([^>]+)>\s*$',body,re.M|re.I)}
        authors=[(name,email),*re.findall(r'^Co-authored-by:\s*(.+?)\s*<([^>]+)>\s*$',body,re.M|re.I)]
        for author,address in authors:
            if (author.strip().casefold(),address.strip().casefold()) not in signed:
                errors.append(f'{commit[:12]}: missing Signed-off-by for {author} <{address}>')
    if errors: raise SystemExit('\n'.join(errors)+'\nSee CONTRIBUTING.md for git commit -s.')
    print('Contribution sign-offs checked.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--base',required=True); parser.add_argument('--head',required=True)
    args=parser.parse_args(); check(args.base,args.head)
