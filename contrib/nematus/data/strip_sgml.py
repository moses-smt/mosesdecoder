import sys
import re


def main():
    fin = sys.stdin
    fout = sys.stdout
    for l in fin:
        line = l.strip()
        text = re.sub('<[^<]+>', "", line).strip()
        if len(text) == 0:
            continue
        print >>fout, text
                

if __name__ == "__main__":
    main()

