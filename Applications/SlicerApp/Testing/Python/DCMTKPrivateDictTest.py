import DICOMLib, sys, slicer, os

dcmfile = sys.argv[1]

print 'DCMDICTPATH is',
try:
  print os.environ['DCMDICTPATH']
except:
  print 'not defined'
  exit(slicer.util.EXIT_FAILURE)

dcmdump=DICOMLib.DICOMCommand('dcmdump',[dcmfile])
dump=str(dcmdump.start()).split('\n')

for line in dump:
  line = line.split(' ')
  if line[0] == '(2001,1003)':
    if line[-1] == "DiffusionBFactor":
      exit(slicer.util.EXIT_SUCCESS)

exit(slicer.util.EXIT_FAILURE)
