import slicer.testing
import time

def TestFiducialAdd(renameFlag=1, visibilityFlag=1, numToAdd=20):
  print "numToAdd = ", numToAdd
  if renameFlag > 0:
    print "Index\tTime to add fid\tDelta between adds\tTime to rename fid\tDelta between renames"
    print "i\tt\tdt\tt\tdt"
  else:
    print "Index\tTime to add fid\tDelta between adds"
    print "i\tt\tdt"
  r = 0
  a = 0
  s = 0
  t1 = 0
  t2 = 0
  t3 = 0
  t4 = 0
  timeToAddThisFid = 0
  timeToAddLastFid = 0
  timeToRenameThisFid = 0
  timeToRenameLastFid = 0
  # iterate over the number of fiducials to add
  for i in range(numToAdd):
#    print "i = ", i, "/", numToAdd, ", r = ", r, ", a = ", a, ", s = ", s
    fidNode = slicer.modulemrml.vtkMRMLAnnotationFiducialNode()
    fidNode.SetFiducialCoordinates(r, a, s)
    t1 = time.clock()
    fidNode.Initialize(slicer.mrmlScene)
    t2 = time.clock()
    timeToAddThisFid = t2 - t1
    dt = timeToAddThisFid - timeToAddLastFid
    if renameFlag > 0:
      t3 = time.clock()
      fidNode.SetName(str(i))
      t4 = time.clock()
      timeToRenameThisFid = t4 - t3
      dt2 = timeToRenameThisFid - timeToRenameLastFid
      print i, "\t", timeToAddThisFid, "\t", dt, "\t", timeToRenameThisFid, "\t", dt2
      timeToRenameLastFid = timeToRenameThisFid
    else:
      print i, "\t", timeToAddThisFid, "\t", dt
    r = r + 1.0
    a = a + 1.0
    s = s + 1.0
    timeToAddLastFid = timeToAddThisFid

testStartTime = time.clock()
TestFiducialAdd()
testEndTime = time.clock()
testTime = testEndTime - testStartTime
print "Test total time = ", testTime
slicer.testing.setEnabled()

slicer.testing.exitSuccess()
