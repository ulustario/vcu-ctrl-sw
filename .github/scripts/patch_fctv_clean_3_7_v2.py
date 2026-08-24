from pathlib import Path

src_path = Path('.github/scripts/patch_fctv_clean_3_7.py')
src = src_path.read_text(encoding='utf-8')
old = """# Percorso online (nascosto) resta FULL, solo per coerenza compilativa.\nreq('Sanitizer.sanitize(original, unsigned);',\n    'Sanitizer.sanitize(original, unsigned, true, true);', 1)\n"""
new = """# Percorso online (nascosto) resta FULL, solo per coerenza compilativa.\n_old_sanitize_call = 'Sanitizer.sanitize(original, unsigned);'\nif s.count(_old_sanitize_call) != 2:\n    raise SystemExit(f'expected two sanitizer calls before split, found {s.count(_old_sanitize_call)}')\ns = s.replace(_old_sanitize_call, 'Sanitizer.sanitize(original, unsigned, true, true);', 1)\n"""
if src.count(old) != 1:
    raise SystemExit('3.7 sanitizer split patch point not unique')
src = src.replace(old, new)
exec(compile(src, str(src_path), 'exec'), {'__name__': '__main__'})
