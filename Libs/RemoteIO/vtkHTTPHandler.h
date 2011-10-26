#ifndef __vtkHTTPHandler_h
#define __vtkHTTPHandler_h

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include <slicerlibcurl/slicerlibcurl.h>

#include <vtkRemoteIOConfigure.h>
#include "vtkRemoteIO.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

//--- derived from libMRML class
#include "vtkURIHandler.h"

class VTK_RemoteIO_EXPORT vtkHTTPHandler : public vtkURIHandler 
{
  public:
  
  /// The Usual vtk class functions
  static vtkHTTPHandler *New();
  vtkTypeRevisionMacro(vtkHTTPHandler, vtkURIHandler);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// 
  /// This methods returns 1 if the handler matches the uri's required 
  /// protocol and returns 0 if it's not appropriate for the uri.
  virtual int CanHandleURI ( const char *uri );

  /// 
  /// Some web servers don't handle 'keep alive' socket transactions
  /// in a way that's compatible with curl on windows.  When this flag is set
  /// curl will do one transaction per connection with the side-effect
  /// that more network resources are used (so avoid this if you can).
  vtkSetMacro(ForbidReuse, int);
  vtkGetMacro(ForbidReuse, int);

  /// 
  /// This function wraps curl functionality to download a specified URL to a specified dir
  void StageFileRead(const char * source, const char * destination);
  using vtkURIHandler::StageFileRead;
  void StageFileWrite(const char * source, const char * destination);
  using vtkURIHandler::StageFileWrite;
  virtual void InitTransfer ( );
  virtual int CloseTransfer ( );

  CURL* CurlHandle;  

 private:

 protected:
  vtkHTTPHandler();
  virtual ~vtkHTTPHandler();
  vtkHTTPHandler(const vtkHTTPHandler&);
  void operator=(const vtkHTTPHandler&);

  int ForbidReuse;

};

#endif

