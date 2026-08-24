from pathlib import Path
p = Path('.github/scripts/patch_fctv_clean_3_4.py')
s = p.read_text(encoding='utf-8')
old = """req('Sanitizer.sanitize(original, unsigned, localSignerSha1);',\n    'Sanitizer.sanitize(original, unsigned, localSignerSha1, true, true);', 1)"""
new = """s = s.replace('Sanitizer.sanitize(original, unsigned, localSignerSha1);',\n              'Sanitizer.sanitize(original, unsigned, localSignerSha1, true, true);', 1)"""
if old not in s:
    raise SystemExit('3.4 online sanitizer replacement not found')
s = s.replace(old, new, 1)
exec(compile(s, str(p), 'exec'), {})
