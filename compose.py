F = [
  # h
  "field.h",
  "status.h",
  "ai.h",
  "ai_random.h",
  "ai_chainer.h",
  # cpp
  "field.cpp",
  "ai_random.cpp",
  "ai_chainer.cpp",
  "codevs2019.cpp",
]

out = open("Main.cpp", "wb")  # for LF
for f in F:
  print>>out, "//  ==== %s ===="%f
  for l in open(f):
    l = l[:-1]
    if (l.startswith("#include \"") or
        l.startswith("#pragma once")):
      l = "//"+l
    print>>out, l
