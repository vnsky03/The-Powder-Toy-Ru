#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os, re

T = {
"PGRV": ("Гравитация", "Создаёт кратковременную гравитационную воронку."),
"NGRV": ("Антиграв.", "Создаёт кратковременную обратную гравитационную воронку."),
"HEAT": ("Нагрев", "Нагревает выбранный элемент."),
"COOL": ("Охлаждение", "Охлаждает выбранный элемент."),
"AMBM": ("Фон. темп. −", "Снижает температуру окружающего воздуха."),
"WIND": ("Ветер", "Создаёт движение воздуха."),
"VAC": ("Вакуум", "Вакуум, снижает давление воздуха."),
"AMBP": ("Фон. темп. +", "Повышает температуру окружающего воздуха."),
"CYCL": ("Циклон", "Циклон, создаёт закрученные потоки воздуха."),
"MIX": ("Смесь", "Перемешивает частицы."),
"AIR": ("Воздух", "Воздух, создаёт поток и давление."),
}

d = "src/simulation/simtools"
for fn in sorted(os.listdir(d)):
    if not fn.endswith(".cpp"):
        continue
    p = os.path.join(d, fn)
    txt = open(p, encoding="utf-8").read()
    m = re.search(r'Identifier = "DEFAULT_TOOL_([A-Z0-9_]+)";', txt)
    code = m.group(1) if m else None
    if not code or code not in T:
        continue
    orig = txt
    txt = re.sub(r'^(\s*)Name = "[^"]*";\s*$', r'\1Name = "%s";' % T[code][0], txt, count=1, flags=re.M)
    txt = re.sub(r'^(\s*)Description = "[^"]*";\s*$', r'\1Description = "%s";' % T[code][1], txt, count=1, flags=re.M)
    if txt != orig:
        open(p, "w", encoding="utf-8").write(txt)
        print("updated", fn)
print("OK")