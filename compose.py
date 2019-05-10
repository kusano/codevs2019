F = [
  # h
  "field.h",
  "game.h",
  "ai.h",
  "ai_random.h",
  "ai_chainer.h",
  "ai_alice.h",
  "table.h",
  # cpp
  "field.cpp",
  "ai_random.cpp",
  "ai_chainer.cpp",
  "ai_alice.cpp",
  "table.cpp",
  "codevs2019.cpp",
]

out = open("Main.cpp", "wb")  # for LF

print >>out, '#pragma GCC optimize("no-aggressive-loop-optimizations")'

for f in F:
  print>>out, "//  ==== %s ===="%f
  for l in open(f):
    l = l[:-1]
    if (l.startswith("#include \"") or
        l.startswith("#pragma once")):
      l = "//"+l
    print>>out, l
