#ifndef DCMTKFileReader_h
#define DCMTKFileReader_h
#undef HAVE_SSTREAM   // 'twould be nice if people coded without using
                                // incredibly generic macro names
#include "osconfig.h" // make sure OS specific configuration is included first

#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "ofstdinc.h"
#include "dcvrds.h"
#include "dcdict.h"             // For DcmDataDictionary
#include "dctk.h"          /* for various dcmdata headers */
#include "cmdlnarg.h"      /* for prepareCmdLineArgs */
#include "dcuid.h"         /* for dcmtk version name */
#include "dcrledrg.h"      /* for DcmRLEDecoderRegistration */

#include "dcmimage.h"     /* for DicomImage */
#include "digsdfn.h"      /* for DiGSDFunction */
#include "diciefn.h"      /* for DiCIELABFunction */

#include "ofconapp.h"        /* for OFConsoleApplication */
#include "ofcmdln.h"         /* for OFCommandLine */

#include "diregist.h"     /* include to support color images */
#include "ofstd.h"           /* for OFStandard */
#include "itkByteSwapper.h"
#include "itkIntTypes.h"

#define DCMTKException(body)                    \
  {                                             \
  if(throwException)                            \
    {                                           \
    itkGenericExceptionMacro(body);             \
    }                                           \
  else                                          \
    {                                           \
    return EXIT_FAILURE;                        \
    }                                           \
  }
// Don't print error messages if you're not throwing
// an exception
//     std::cerr body;

class DCMTKSequence
{
public:
  DCMTKSequence() : m_DcmSequenceOfItems(0) {}
  void SetDcmSequenceOfItems(DcmSequenceOfItems *seq)
    {
      this->m_DcmSequenceOfItems = seq;
    }
  int card() { return this->m_DcmSequenceOfItems->card(); }
  int GetSequence(unsigned long index,
                  DCMTKSequence &target,bool throwException = true)
    {
      DcmItem *item = this->m_DcmSequenceOfItems->getItem(index);
      DcmSequenceOfItems *sequence =
        dynamic_cast<DcmSequenceOfItems *>(item);
      if(sequence == 0)
        {
        DCMTKException(<< "Can't find DCMTKSequence at index " << index);
        }
      target.SetDcmSequenceOfItems(sequence);
      return EXIT_SUCCESS;
    }
  int GetStack(unsigned short group,
                unsigned short element,
                DcmStack &resultStack, bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      if(this->m_DcmSequenceOfItems->search(tagkey,resultStack) != EC_Normal)
        {
        DCMTKException(<< "Can't find tag " << std::hex << group << " "
                       << element << std::dec);
        }
      return EXIT_SUCCESS;
    }

  int GetElementCS(unsigned short group,
                    unsigned short element,
                    std::string &target,
                    bool throwException = true)
    {
      DcmStack resultStack;
      this->GetStack(group,element,resultStack);
      DcmCodeString *codeStringElement = dynamic_cast<DcmCodeString *>(resultStack.top());
      if(codeStringElement == 0)
        {
          DCMTKException(<< "Can't get CodeString Element at tag "
                                   << std::hex << group << " "
                                   << element << std::dec);
        }
      OFString ofString;
      if(codeStringElement->getOFStringArray(ofString) != EC_Normal)
          {
          DCMTKException(<< "Can't get OFString Value at tag "
                         << std::hex << group << " "
                         << element << std::dec);
          }
      target = "";
      for(unsigned j = 0; j < ofString.length(); ++j)
        {
        target += ofString[j];
        }
      return EXIT_SUCCESS;
    }
  int GetElementFD(unsigned short group,
                    unsigned short element,
                    double * &target,
                    bool throwException = true)
    {
      DcmStack resultStack;
      this->GetStack(group,element,resultStack);
      DcmFloatingPointDouble *fdItem = dynamic_cast<DcmFloatingPointDouble *>(resultStack.top());
      if(fdItem == 0)
        {
          DCMTKException(<< "Can't get CodeString Element at tag "
                                   << std::hex << group << " "
                                   << element << std::dec);
        }
      if(fdItem->getFloat64Array(target) != EC_Normal)
        {
        DCMTKException(<< "Can't get floatarray Value at tag "
                       << std::hex << group << " "
                       << element << std::dec);
        }
      return EXIT_SUCCESS;
    }
  int GetElementFD(unsigned short group,
                    unsigned short element,
                    double &target,
                    bool throwException = true)
    {
      double *array;
      this->GetElementFD(group,element,array,throwException);
      target = array[0];
      return EXIT_SUCCESS;
    }
  int GetElementDS(unsigned short group,
                  unsigned short element,
                  std::string &target,
                  bool throwException = true)
    {
      DcmStack resultStack;
      this->GetStack(group,element,resultStack);
      DcmDecimalString *decimalStringElement = dynamic_cast<DcmDecimalString *>(resultStack.top());
      if(decimalStringElement == 0)
        {
        DCMTKException(<< "Can't get DecimalString Element at tag "
                       << std::hex << group << " "
                       << element << std::dec);
        }
      // check for # of expected numbers in DS
      std::stringstream ss;
      ss << target;
      OFString arity(ss.str().c_str());
      if(decimalStringElement->checkValue(arity) != EC_Normal)
        {
        DCMTKException(<< "Value doesn't have proper number of elements");
        }
      OFString ofString;
      if(decimalStringElement->getOFStringArray(ofString) != EC_Normal)
        {
        DCMTKException(<< "Can't get DecimalString Value at tag "
                       << std::hex << group << " "
                       << element << std::dec);
        }
      target = "";
      for(unsigned j = 0; j < ofString.length(); ++j)
        {
        target += ofString[j];
        }
      return EXIT_SUCCESS;
    }
  int GetElementSQ(unsigned short group,
                  unsigned short element,
                  DCMTKSequence &target,
                  bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmStack resultStack;
      this->GetStack(group,element,resultStack);

      DcmSequenceOfItems *seqElement = dynamic_cast<DcmSequenceOfItems *>(resultStack.top());
      if(seqElement == 0)
        {
          DCMTKException(<< "Can't get  at tag "
                                   << std::hex << group << " "
                                   << element << std::dec);
        }
      target.SetDcmSequenceOfItems(seqElement);
      return EXIT_SUCCESS;
    }
private:
  DcmSequenceOfItems *m_DcmSequenceOfItems;
};

class DCMTKFileReader
{
public:
  typedef DCMTKFileReader Self;

  DCMTKFileReader() : m_DFile(0),
                      m_Dataset(0),
                      m_Xfer(EXS_Unknown),
                      m_FrameCount(0),
                      m_FileNumber(-1L)
    {
    }
  ~DCMTKFileReader()
    {

      delete m_DFile;
    }
  void SetFileName(const std::string &fileName)
    {
      this->m_FileName = fileName;
    }
  const std::string &GetFileName() const
    {
      return this->m_FileName;
    }
  void LoadFile()
    {
      if(this->m_FileName == "")
        {
        itkGenericExceptionMacro(<< "No filename given" );
        }
      if(this->m_DFile != 0)
        {
        delete this->m_DFile;
        }
      this->m_DFile = new DcmFileFormat();
      OFCondition cond = this->m_DFile->loadFile(this->m_FileName.c_str());
      if(cond.bad())
        {
        itkGenericExceptionMacro(<< cond.text() << ": reading file " << this->m_FileName);
        }
      this->m_Dataset = this->m_DFile->getDataset();
      this->m_Xfer = this->m_Dataset->getOriginalXfer();
      if(this->m_Dataset->findAndGetSint32(DCM_NumberOfFrames,this->m_FrameCount).bad())
        {
        this->m_FrameCount = 1;
        }
      int fnum;
      this->GetElementIS(0x0020,0x0013,fnum);
      this->m_FileNumber = fnum;
    }
  int GetElementLO(unsigned short group,
                  unsigned short element,
                  std::string &target,
                  bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmLongString *loItem = dynamic_cast<DcmLongString *>(el);
      if(loItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      OFString ofString;
      if(loItem->getOFStringArray(ofString) != EC_Normal)
        {
        DCMTKException(<< "Cant get string from element " << std::hex
                       << group << " " << std::hex
                       << element << std::dec);
        }
      target = "";
      for(unsigned i = 0; i < ofString.size(); i++)
        {
        target += ofString[i];
        }
      return EXIT_SUCCESS;
    }

  int GetElementLO(unsigned short group,
                    unsigned short element,
                    std::vector<std::string> &target,
                    bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmLongString *loItem = dynamic_cast<DcmLongString *>(el);
      if(loItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      target.clear();
      OFString ofString;
      for(unsigned long i = 0; loItem->getOFString(ofString,i) == EC_Normal; ++i)
        {
        std::string targetStr = "";
        for(unsigned i = 0; i < ofString.size(); i++)
          {
          targetStr += ofString[i];
          }
        target.push_back(targetStr);
        }
      return EXIT_SUCCESS;
    }

   /** Get an array of data values, as contained in a DICOM
    * DecimalString Item
    */
  template <typename TType>
  int  GetElementDS(unsigned short group,
                     unsigned short element,
                     unsigned short count,
                     TType  *target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmDecimalString *dsItem = dynamic_cast<DcmDecimalString *>(el);
      if(dsItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      OFVector<Float64> doubleVals;
      if(dsItem->getFloat64Vector(doubleVals) != EC_Normal)
        {
          DCMTKException(<< "Cant extract Array from DecimalString " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      if(doubleVals.size() != count)
        {
          DCMTKException(<< "DecimalString " << std::hex
                                   << group << " " << std::hex
                                   << element << " expected "
                                   << count << "items, but found "
                                   << doubleVals.size() << std::dec);

        }
      for(unsigned i = 0; i < count; i++)
        {
        target[i] = static_cast<TType>(doubleVals[i]);
        }
      return EXIT_SUCCESS;
    }

  template <typename TType>
  int  GetElementDSorOB(unsigned short group,
                        unsigned short element,
                        TType  &target,
                        bool throwException = true)
    {
      if(this->GetElementDS<TType>(group,element,1,&target,false) == EXIT_SUCCESS)
        {
        return EXIT_SUCCESS;
        }
      std::string val;
      if(this->GetElementOB(group,element,val) != EXIT_SUCCESS)
        {
        DCMTKException(<< "Cant find DecimalString element " << std::hex
                       << group << " " << std::hex
                       << element << std::dec);
        }
      const char *data = val.c_str();
      const TType *fptr = reinterpret_cast<const TType *>(data);
      target = *fptr;
      switch(this->GetTransferSyntax())
        {
        case EXS_LittleEndianImplicit:
        case EXS_LittleEndianExplicit:
          itk::ByteSwapper<TType>::SwapFromSystemToLittleEndian(&target);
          break;
        case EXS_BigEndianImplicit:
        case EXS_BigEndianExplicit:
          itk::ByteSwapper<TType>::SwapFromSystemToBigEndian(&target);
          break;
        default:
          break;
        }
      return EXIT_SUCCESS;

    }
  /** Get a DecimalString Item as a single string
   */
  int  GetElementDS(unsigned short group,
                     unsigned short element,
                     std::string &target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmDecimalString *dsItem = dynamic_cast<DcmDecimalString *>(el);
      if(dsItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      OFString ofString;
      if(dsItem->getOFStringArray(ofString) != EC_Normal)
        {
        DCMTKException(<< "Can't get DecimalString Value at tag "
                       << std::hex << group << " "
                       << element << std::dec);
        }
      target = "";
      for(unsigned j = 0; j < ofString.length(); ++j)
        {
        target += ofString[j];
        }
      return EXIT_SUCCESS;
    }
  int  GetElementFD(unsigned short group,
                     unsigned short element,
                     double &target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmFloatingPointDouble *fdItem = dynamic_cast<DcmFloatingPointDouble *>(el);
      if(fdItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      if(fdItem->getFloat64(target) != EC_Normal)
        {
          DCMTKException(<< "Cant extract Array from DecimalString " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      return EXIT_SUCCESS;
    }
  int  GetElementFD(unsigned short group,
                     unsigned short element,
                     double * &target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmFloatingPointDouble *fdItem = dynamic_cast<DcmFloatingPointDouble *>(el);
      if(fdItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      if(fdItem->getFloat64Array(target) != EC_Normal)
        {
          DCMTKException(<< "Cant extract Array from DecimalString " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      return EXIT_SUCCESS;
    }
  int  GetElementFL(unsigned short group,
                     unsigned short element,
                     float &target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmFloatingPointSingle *flItem = dynamic_cast<DcmFloatingPointSingle *>(el);
      if(flItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      if(flItem->getFloat32(target) != EC_Normal)
        {
          DCMTKException(<< "Cant extract Array from DecimalString " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      return EXIT_SUCCESS;
    }
  int  GetElementFLorOB(unsigned short group,
                     unsigned short element,
                     float &target,
                     bool throwException = true)
    {
      if(this->GetElementFL(group,element,target,false) == EXIT_SUCCESS)
        {
        return EXIT_SUCCESS;
        }
      std::string val;
      if(this->GetElementOB(group,element,val) != EXIT_SUCCESS)
        {
        DCMTKException(<< "Cant find DecimalString element " << std::hex
                       << group << " " << std::hex
                       << element << std::dec);
        }
      const char *data = val.c_str();
      const float *fptr = reinterpret_cast<const float *>(data);
      target = *fptr;
      switch(this->GetTransferSyntax())
        {
        case EXS_LittleEndianImplicit:
        case EXS_LittleEndianExplicit:
          itk::ByteSwapper<float>::SwapFromSystemToLittleEndian(&target);
          break;
        case EXS_BigEndianImplicit:
        case EXS_BigEndianExplicit:
          itk::ByteSwapper<float>::SwapFromSystemToBigEndian(&target);
          break;
        default:
          break;
        }
      return EXIT_SUCCESS;
    }

  int  GetElementUS(unsigned short group,
                     unsigned short element,
                     unsigned short &target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmUnsignedShort *usItem = dynamic_cast<DcmUnsignedShort *>(el);
      if(usItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      if(usItem->getUint16(target) != EC_Normal)
        {
          DCMTKException(<< "Cant extract Array from DecimalString " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      return EXIT_SUCCESS;
    }
  int  GetElementUS(unsigned short group,
                     unsigned short element,
                     unsigned short *&target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmUnsignedShort *usItem = dynamic_cast<DcmUnsignedShort *>(el);
      if(usItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      if(usItem->getUint16Array(target) != EC_Normal)
        {
          DCMTKException(<< "Cant extract Array from DecimalString " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      return EXIT_SUCCESS;
    }
  /** Get a DecimalString Item as a single string
   */
  int  GetElementCS(unsigned short group,
                     unsigned short element,
                     std::string &target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmCodeString *csItem = dynamic_cast<DcmCodeString *>(el);
      if(csItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      OFString ofString;
      if(csItem->getOFStringArray(ofString) != EC_Normal)
        {
        DCMTKException(<< "Can't get DecimalString Value at tag "
                       << std::hex << group << " "
                       << element << std::dec);
        }
      target = "";
      for(unsigned j = 0; j < ofString.length(); ++j)
        {
        target += ofString[j];
        }
      return EXIT_SUCCESS;
    }

  /** get an IS (Integer String Item
   */
  int  GetElementIS(unsigned short group,
                    unsigned short element,
                    ::itk::int32_t  &target,
                    bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmIntegerString *isItem = dynamic_cast<DcmIntegerString *>(el);
      if(isItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      if(isItem->getSint32(target) != EC_Normal)
        {
        DCMTKException(<< "Can't get DecimalString Value at tag "
                       << std::hex << group << " "
                       << element << std::dec);
        }
      return EXIT_SUCCESS;
    }

  int  GetElementISorOB(unsigned short group,
                        unsigned short element,
                        ::itk::int32_t  &target,
                        bool throwException = true)
    {
      if(this->GetElementIS(group,element,target,false) == EXIT_SUCCESS)
        {
        return EXIT_SUCCESS;
        }
      std::string val;
      if(this->GetElementOB(group,element,val,throwException) != EXIT_SUCCESS)
        {
        return EXIT_FAILURE;
        }
      const char *data = val.c_str();
      const int *iptr = reinterpret_cast<const int *>(data);
      target = *iptr;
      switch(this->GetTransferSyntax())
        {
        case EXS_LittleEndianImplicit:
        case EXS_LittleEndianExplicit:
          itk::ByteSwapper<int>::SwapFromSystemToLittleEndian(&target);
          break;
        case EXS_BigEndianImplicit:
        case EXS_BigEndianExplicit:
          itk::ByteSwapper<int>::SwapFromSystemToBigEndian(&target);
          break;
        default:                // no idea what to do
          break;
        }

      return EXIT_SUCCESS;
    }

  /** get an OB OtherByte Item
   */
  int  GetElementOB(unsigned short group,
                     unsigned short element,
                     std::string &target,
                     bool throwException = true)
    {
      DcmTagKey tagkey(group,element);
      DcmElement *el;
      if(this->m_Dataset->findAndGetElement(tagkey,el) != EC_Normal)
        {
          DCMTKException(<< "Cant find tag " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      DcmOtherByteOtherWord *obItem = dynamic_cast<DcmOtherByteOtherWord *>(el);
      if(obItem == 0)
        {
          DCMTKException(<< "Cant find DecimalString element " << std::hex
                                   << group << " " << std::hex
                                   << element << std::dec);
        }
      OFString ofString;
      if(obItem->getOFStringArray(ofString) != EC_Normal)
        {
        DCMTKException(<< "Can't get OFString Value at tag "
                       << std::hex << group << " "
                       << element << std::dec);
        }
      target = Self::ConvertFromOB(ofString);
      return EXIT_SUCCESS;
    }

  int GetElementSQ(unsigned short group,
                  unsigned short entry,
                  DCMTKSequence &sequence,
                  bool throwException = true)
    {
      DcmSequenceOfItems *seq;
      DcmTagKey tagKey(group,entry);

      if(this->m_Dataset->findAndGetSequence(tagKey,seq) != EC_Normal)
        {
        DCMTKException(<< "Can't find sequence "
                       << std::hex << group << " "
                       << std::hex << entry)
        }
      sequence.SetDcmSequenceOfItems(seq);
      return EXIT_SUCCESS;
    }
  int GetFrameCount() { return this->m_FrameCount; }

  E_TransferSyntax GetTransferSyntax() { return m_Xfer; }

  long GetFileNumber() const
    {
      return m_FileNumber;
    }
  static void
  AddDictEntry(DcmDictEntry *entry)
    {
      DcmDataDictionary &dict = dcmDataDict.wrlock();
      dict.addEntry(entry);
      dcmDataDict.unlock();
    }

private:
  static unsigned ascii2hex(char c)
    {
      switch(c)
        {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a':
        case 'A': return 10;
        case 'b':
        case 'B': return 11;
        case 'c':
        case 'C': return 12;
        case 'd':
        case 'D': return 13;
        case 'e':
        case 'E': return 14;
        case 'f':
        case 'F': return 15;
        }
      return 255; // should never happen
    }
  static std::string ConvertFromOB(OFString &toConvert)
    {
      // string format is nn\nn\nn...
      std::string rval;
      for(size_t pos = 0; pos < toConvert.size(); pos += 3)
        {
        unsigned char convert[2];
        convert[0] = Self::ascii2hex(toConvert[pos]);
        convert[1] = Self::ascii2hex(toConvert[pos+1]);
        unsigned char conv = convert[0] << 4;
        conv += convert[1];
        rval.push_back(static_cast<unsigned char>(conv));
        }
      return rval;
    }

  std::string          m_FileName;
  DcmFileFormat*       m_DFile;
  DcmDataset *         m_Dataset;
  E_TransferSyntax     m_Xfer;
  Sint32               m_FrameCount;
  long                 m_FileNumber;
};

bool CompareDCMTKFileReaders(DCMTKFileReader *a, DCMTKFileReader *b)
{
  return a->GetFileNumber() < b->GetFileNumber();
}

#endif // DCMTKFileReader_h
