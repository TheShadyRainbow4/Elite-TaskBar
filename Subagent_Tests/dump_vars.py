import re
vars = set()
with open('SourceFiles/EliteSettings.ps1', 'r', encoding='utf-8') as f:
    for line in f:
        for m in re.finditer(r'\$(chk|cmb|txt|rad|grp|lbl|tab|frm|btn)_[a-zA-Z0-9_]+', line):
            vars.add(m.group(0))
for v in sorted(vars):
    print(v)
