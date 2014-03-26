#include "vtkHTTPHandler.h"
#include <vtkPermissionPrompter.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro ( vtkHTTPHandler );
vtkCxxRevisionMacro ( vtkHTTPHandler, "$Revision: 1.0 $" );

//----------------------------------------------------------------------------
size_t read_callback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t retcode;

  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */
  retcode = fread(ptr, size, nmemb, stream);

  std::cout << "*** We read " << retcode << " bytes from file\n";

  return retcode;
}

//----------------------------------------------------------------------------
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  if (stream == NULL)
    {
    std::cerr << "write_callback: can't write, stream is null. size = " << size << std::endl;
    return 0;
    }
  int written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

//----------------------------------------------------------------------------
size_t ProgressCallback(FILE* vtkNotUsed( outputFile ), double dltotal, double dlnow, double ultotal, double ulnow)
{
  if(ultotal == 0)
    {
    if(dltotal > 0)
      {
      std::cout << "<filter-progress>" << dlnow/dltotal
                << "</filter-progress>" << std::endl;
      }
    }
  else
    {
    std::cout << ulnow*100/ultotal << "%" << std::endl;
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkHTTPHandler::vtkHTTPHandler()
{
  this->CurlHandle = NULL;
  this->ForbidReuse = 0;
}


//----------------------------------------------------------------------------
vtkHTTPHandler::~vtkHTTPHandler()
{
  this->CurlHandle = NULL;
}


//----------------------------------------------------------------------------
void vtkHTTPHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf ( os, indent );
}



//----------------------------------------------------------------------------
int vtkHTTPHandler::CanHandleURI ( const char *uri )
{
  //--- What's the best way to determine whether this handler
  //--- speaks the correct protocol?
  //--- first guess is to look at the prefix up till the colon.

  std::string::size_type index;
  std::string uriString (uri);
  std::string prefix;

  //--- get all characters up to (and not including) the '://'
  if ( ( index = uriString.find ( "://", 0 ) ) != std::string::npos )
    {
    prefix = uriString.substr ( 0, index );
    //--- check to see if any bracketed characters are in
    //--- this part of the string.
    if ( (index = prefix.find ( "]:", 0 ) ) != std::string::npos )
      {
      //--- if so, strip off the leading bracketed characters in case
      //--- we adopt the gwe "[filename.ext]:" prefix.
      prefix = prefix.substr ( index+2 );
      }
    if ( !strcmp(prefix.c_str(), "http"))
      {
      vtkDebugMacro("vtkHTTPHandler: CanHandleURI: can handle this file: " << uriString.c_str());
      return (1);
      }
    }
  else
    {
    vtkDebugMacro ( "vtkHTTPHandler::CanHandleURI: unrecognized uri format: " << uriString.c_str() );
    }
  return ( 0 );
}



//----------------------------------------------------------------------------
void vtkHTTPHandler::InitTransfer( )
{
  curl_global_init(CURL_GLOBAL_ALL);
  vtkDebugMacro("vtkHTTPHandler: InitTransfer: initialising CurlHandle");
  this->CurlHandle = curl_easy_init();
  if (this->CurlHandle == NULL)
    {
    vtkErrorMacro("InitTransfer: unable to initialise");
    }
}

//----------------------------------------------------------------------------
int vtkHTTPHandler::CloseTransfer( )
{
  curl_easy_cleanup(this->CurlHandle);
  return EXIT_SUCCESS;
}


//----------------------------------------------------------------------------
void vtkHTTPHandler::StageFileRead(const char * source, const char * destination)
{
  if (source == NULL || destination == NULL)
    {
    vtkErrorMacro("StageFileRead: source or dest is null!");
    return;
    }
  /*
  if (this->LocalFile)
    {
    this->LocalFile->close();
    delete this->LocalFile;
    this->LocalFile = NULL;
    }
  this->LocalFile = new std::ofstream(destination, std::ios::binary);
  */
  this->InitTransfer( );


  if ( this->ForbidReuse )
    {
    curl_easy_setopt(this->CurlHandle, CURLOPT_FORBID_REUSE, 1);
    }
  curl_easy_setopt(this->CurlHandle, CURLOPT_HTTPGET, 1);
  curl_easy_setopt(this->CurlHandle, CURLOPT_URL, source);
//  curl_easy_setopt(this->CurlHandle, CURLOPT_NOPROGRESS, false);
  curl_easy_setopt(this->CurlHandle, CURLOPT_FOLLOWLOCATION, true);
  // use the default curl write call back
  curl_easy_setopt(this->CurlHandle, CURLOPT_WRITEFUNCTION, NULL); // write_callback);
  this->LocalFile = fopen(destination, "wb");
  // output goes into LocalFile, must be  FILE*
  curl_easy_setopt(this->CurlHandle, CURLOPT_WRITEDATA, this->LocalFile);
//  curl_easy_setopt(this->CurlHandle, CURLOPT_PROGRESSDATA, NULL);
//  curl_easy_setopt(this->CurlHandle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);

  // quick timeout during connection phase if URL is not accessible (e.g. blocked by a firewall)
  curl_easy_setopt(this->CurlHandle, CURLOPT_CONNECTTIMEOUT, 3); // in seconds (type long)

  vtkDebugMacro("StageFileRead: about to do the curl download... source = " << source << ", dest = " << destination);
  CURLcode retval = curl_easy_perform(this->CurlHandle);

  if (retval == CURLE_OK)
    {
    vtkDebugMacro("StageFileRead: successful return from curl");
    }
  else if (retval == CURLE_BAD_FUNCTION_ARGUMENT)
    {
    vtkErrorMacro("StageFileRead: bad function argument to curl, did you init CurlHandle?");
    }
  else if (retval == CURLE_OUT_OF_MEMORY)
    {
    vtkErrorMacro("StageFileRead: curl ran out of memory!");
    }
  else
    {
    const char *stringError = curl_easy_strerror(retval);
    vtkErrorMacro("StageFileRead: error running curl: " << stringError);
    //--- in case the permissions were not correct and that's
    //--- the reason the read command failed,
    //--- reset the 'remember check' in the permissions
    //--- prompter so that new login info  will be prompted.
    if ( this->GetPermissionPrompter() != NULL )
      {
      this->GetPermissionPrompter()->SetRemember ( 0 );
      }
    }
  this->CloseTransfer();

  /*
  this->LocalFile->close();
  delete this->LocalFile;
  this->LocalFile = NULL;
  */
  if (this->LocalFile)
    {
    fclose(this->LocalFile);
    }
}


//----------------------------------------------------------------------------
void vtkHTTPHandler::StageFileWrite(const char * source, const char * destination)
{
  //--- check these arguments...
  /*
  if (this->LocalFile)
    {
    this->LocalFile->close();
    delete this->LocalFile;
    this->LocalFile = NULL;
    }
  this->LocalFile = new std::ofstream(destination, std::ios::binary);
  */
  this->LocalFile = fopen(source, "r");

  this->InitTransfer( );

  curl_easy_setopt(this->CurlHandle, CURLOPT_PUT, 1);
  curl_easy_setopt(this->CurlHandle, CURLOPT_URL, destination);
//  curl_easy_setopt(this->CurlHandle, CURLOPT_NOPROGRESS, false);
  curl_easy_setopt(this->CurlHandle, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(this->CurlHandle, CURLOPT_READFUNCTION, read_callback);
  curl_easy_setopt(this->CurlHandle, CURLOPT_READDATA, this->LocalFile);
//  curl_easy_setopt(this->CurlHandle, CURLOPT_PROGRESSDATA, NULL);
  //curl_easy_setopt(this->CurlHandle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
  CURLcode retval = curl_easy_perform(this->CurlHandle);

   if (retval == CURLE_OK)
    {
    vtkDebugMacro("StageFileWrite: successful return from curl");
    }
   else
    {
    const char *stringError = curl_easy_strerror(retval);
    vtkErrorMacro("StageFileWrite: error running curl: " << stringError);
    //--- in case the permissions were not correct and that's
    //--- the reason the read command failed,
    //--- reset the 'remember check' in the permissions
    //--- prompter so that new login info  will be prompted.
    if ( this->GetPermissionPrompter() != NULL )
      {
      this->GetPermissionPrompter()->SetRemember ( 0 );
      }
    }

  this->CloseTransfer();

  fclose(this->LocalFile);
  /*
  this->LocalFile->close();
  delete this->LocalFile;
  this->LocalFile = NULL;
  */
}
