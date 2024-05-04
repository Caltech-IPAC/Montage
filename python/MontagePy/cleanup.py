import sys

if len(sys.argv) < 2:
    print('Error> Usage: python fix_wrapper.py infile')
    sys.exit(0)

infile = open(sys.argv[1], 'r')

while True:
    line = infile.readline()

    if not line:
        break

    line = line.rstrip()

    print(line)

    if line[0:11] == 'def mViewer':
        print('    # Next four lines added by clean-up script')
        print('')
        print('    import importlib_resources')
        print('')
        print('    if fontFile == "":')
        print('        fontFile = str(importlib_resources.files("MontagePy") / "FreeSans.ttf")')
