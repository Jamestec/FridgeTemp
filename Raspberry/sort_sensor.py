import os
import time
for tup in os.walk("sensor"):
	if (tup[1] == [] and len(tup[2]) > 0):
		for x in tup[2]:
			if "backup" not in x:
				path = os.path.join(tup[0], x)
				data = {}
				keys = []
				with open(path, 'r') as FILE:
					for line in FILE.read().splitlines():
						time = line.split(" ")[0]
						if time not in data:
							data[time] = [line]
							keys.append(time)
						else:
							data[time].append(line)
				to_write = []
				keys_sorted = sorted(keys)
				if keys != keys_sorted:
					os.rename(path, path + ".backup")
					print("Sorted: {}".format(path))
					for key in keys_sorted:
						to_write += data[key]
					content = ""
					for line in to_write:
						content += "{}\n".format(line)
					# Write
					with open(path, "a+") as FILE:
						FILE.write(content)
					try:
						os.chmod(path, 0o666)
					except OSError:
						pass
