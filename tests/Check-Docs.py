"""Check local documentation links and image references. No network required."""
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

root = Path(__file__).resolve().parents[1]
documents = [root/'README.md',root/'CONTRIBUTING.md', *sorted((root/'docs').rglob('*.md'))]
errors = []
references = 0
for document in documents:
    text = document.read_text(encoding='utf-8')
    # Don't interpret example code as Markdown links.
    prose = re.sub(r'```.*?```', '', text, flags=re.S)
    targets = re.findall(r'\[[^\]]*\]\(([^)]+)\)', prose)
    targets += re.findall(r'(?:src|href)="([^"]+)"', prose)
    for target in targets:
        url = urlsplit(target.strip('<>'))
        if url.scheme or not url.path:
            continue
        references += 1
        destination = (document.parent/unquote(url.path)).resolve()
        if not destination.exists():
            errors.append(f'{document.relative_to(root)}: missing {target}')
if errors:
    print('\n'.join(errors), file=sys.stderr)
    raise SystemExit(1)
print(f'{len(documents)} documents, {references} local links/images checked.')
