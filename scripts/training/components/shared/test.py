import subprocess

def cat(filename, content):
  fh = open(filename, "w")
  for line in content:
    #print(line, file=fh)
    print >> fh, line
  fh.close()

def diff(filename1, filename2):
  diff_output = subprocess.check_output(["diff", filename1, filename2], stderr=subprocess.STDOUT)
  if diff_output:
    raise Exception()
