import itk
from ctk_cli import *
import sys
import logging

def SmoothingRecursiveGaussianImageFilter(inputVolume, outputVolume, sigma):
    reader = itk.ImageFileReader.New(FileName=inputVolume)
    filter = itk.SmoothingRecursiveGaussianImageFilter.New(reader)
    filter.SetSigma(sigma)
    writer = itk.ImageFileWriter.New(filter,FileName=outputVolume)
    writer.SetUseCompression(True)
    writer.Update()
    return 1


def main():
    """Parsing command line arguments and reading input files."""
    logging.basicConfig(level=logging.INFO)
    args=CLIArgumentParser().parse_args()
    # Run processing
    SmoothingRecursiveGaussianImageFilter(args.inputVolume,args.outputVolume,args.sigma)
    # Compare output with baseline
    reader1 = itk.ImageFileReader.New(FileName=args.outputVolume)
    reader2 = itk.ImageFileReader.New(FileName=args.baselineVolume)
    compareFilter=itk.ComparisonImageFilter.New(reader1)
    compareFilter.SetTestInput(reader1)
    compareFilter.SetValidInput(reader2)
    diff=compareFilter.GetTotalDifference()
    if diff < args.tolerance:
      return 0
    return 1

if __name__ == "__main__":
    ret=main()
    if ret:
      sys.exit(ret)
