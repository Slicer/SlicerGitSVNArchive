#ifndef __vtkSRBHandler_h
#define __vtkSRBHandler_h

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include <vtkRemoteIOConfigure.h>
#include "vtkRemoteIO.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

//--- derived from libMRML class
#include "vtkURIHandler.h"

class VTK_RemoteIO_EXPORT vtkSRBHandler : public vtkURIHandler 
{
  public:
  
  /// The Usual vtk class functions
  static vtkSRBHandler *New();
  vtkTypeRevisionMacro(vtkSRBHandler, vtkURIHandler);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// 
  /// This methods returns 1 if the handler matches the uri's required 
  /// protocol and returns 0 if it's not appropriate for the uri. uri must
  /// start with srb://
  virtual int CanHandleURI ( const char *uri );

  /// 
  /// This function wraps SCommand functionality to download a specified URI to
  /// a specified destination file
  void StageFileRead(const char * source, const char * destination);
  using vtkURIHandler::StageFileRead;
  void StageFileWrite(const char * source, const char * destination);
  using vtkURIHandler::StageFileWrite;

 private:
  virtual void InitTransfer ( );
  virtual int CloseTransfer ( );

 protected:
  vtkSRBHandler();
  virtual ~vtkSRBHandler();
  vtkSRBHandler(const vtkSRBHandler&);
  void operator=(const vtkSRBHandler&);

};

#endif

