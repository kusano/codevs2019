for x in range(10):
  for y in range(19):
    T = []
    for dx in range(-1, 2):
      for dy in range(-1, 2):
        if (dx, dy)!=(0, 0):
          tx = x + dx
          ty = y + dy
          if not (0<=tx and tx<10 and 0<=ty and ty<19):
            tx = x
            ty = y
          T += [tx*19+ty]
    print "{"+", ".join("0x%02x"%t for t in T)+"},"

for x in range(10):
  print ", ".join(str(x) for y in range(19))+","

for x in range(10):
  print ", ".join(str(y) for y in range(19))+","
