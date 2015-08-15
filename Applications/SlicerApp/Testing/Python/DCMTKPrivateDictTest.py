import DICOMLib, sys, slicer

dcmfile = sys.argv[1]

dcmdump=DICOMLib.DICOMCommand('dcmdump',[dcmfile])
dump=str(dcmdump.start()).split('\n')

for line in dump:
  line = line.split(' ')
  if line[0] == '(2001,1003)':
    if line[-1] == "DiffusionBFactor":
      exit(slicer.util.EXIT_SUCCESS)

exit(slicer.util.EXIT_FAILURE)
