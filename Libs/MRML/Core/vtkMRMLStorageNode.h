/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLStorageNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/

#ifndef __vtkMRMLStorageNode_h
#define __vtkMRMLStorageNode_h

// MRML includes
#include "vtkMRMLNode.h"
class vtkURIHandler;

// VTK includes
class vtkStringArray;

// STD includes
#include <vector>

/// \brief A supercalss for other storage nodes.
///
/// A superclass for other storage nodes like volume and model.
class VTK_MRML_EXPORT vtkMRMLStorageNode : public vtkMRMLNode
{
public:
  vtkTypeMacro(vtkMRMLStorageNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance() = 0;

  ///
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  ///
  /// Read data from \a FileName and set it in the referenced node.
  /// If temporaryFile is true, it informs the reader that the file to read is
  /// from a temporary directory. It means that the node should probably be
  /// saved by the user before leaving the application as the file might already
  /// be deleted or be deleted soon after.
  /// Return 1 on success, 0 on failure.
  /// \todo make temporaryFile a property (similar to what FileName)
  /// \sa SetFileName(), ReadDataInternal(), GetStoredTime()
  virtual int ReadData(vtkMRMLNode *refNode, bool temporaryFile = false);

  ///
  /// Write data from a  referenced node
  /// Return 1 on success, 0 on failure.
  /// NOTE: Subclasses should implement this method
  virtual int WriteData(vtkMRMLNode *refNode);

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  ///
  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName() = 0;

  /// A file name or the archetype file name for a series used for read or write
  /// \sa ReadData(), WriteData()
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  ///
  /// return the nth file name, null if doesn't exist
  const char *GetNthFileName(int n) const;

  ///
  /// Use compression on write
  vtkBooleanMacro(UseCompression, int);
  vtkGetMacro(UseCompression, int);
  vtkSetMacro(UseCompression, int);

  ///
  /// Location of the remote copy of this file.
  vtkSetStringMacro(URI);
  vtkGetStringMacro(URI);

  vtkGetObjectMacro (URIHandler, vtkURIHandler);
  virtual void SetURIHandler(vtkURIHandler* uriHandler);

  ///
  /// Propagate Progress Event generated in ReadData
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData );

  ///
  /// Possible Read and Write states
  /// Idle: not currently working on any data, ready for the next transfer
  /// Pending: the data is remote, waiting for a transfer to be scheduled
  /// Scheduled: the data is remote, and is scheduled for download
  /// Transferring: data is remote, and the transfer is working to completion
  /// TransferDone: the data is on disk and ready to be read
  /// Cancelled: the user cancelled the remote data transfer
  enum
  {
    Idle,
    Pending,
    Scheduled,
    Transferring,
    TransferDone,
    Cancelled
  };

  /// Get/Set the state of reading
  vtkGetMacro(ReadState,int);
  vtkSetMacro(ReadState,int);
  void SetReadStatePending() { this->SetReadState(this->Pending); };
  void SetReadStateIdle() { this->SetReadState(this->Idle); };
  void SetReadStateScheduled() { this->SetReadState(this->Scheduled); };
  void SetReadStateTransferring() { this->SetReadState(this->Transferring); };
  void SetReadStateTransferDone() { this->SetReadState(this->TransferDone); };
  void SetReadStateCancelled() { this->SetReadState(this->Cancelled); };
  const char *GetStateAsString(int state);
  const char *GetReadStateAsString() { return this->GetStateAsString(this->ReadState); };

  ///
  /// Get/Set the state of writing
  vtkGetMacro(WriteState,int);
  vtkSetMacro(WriteState,int);
  void SetWriteStatePending() { this->SetWriteState(this->Pending); };
  void SetWriteStateIdle() { this->SetWriteState(this->Idle); };
  void SetWriteStateScheduled() { this->SetWriteState(this->Scheduled); };
  void SetWriteStateTransferring() { this->SetWriteState(this->Transferring); };
  void SetWriteStateTransferDone() { this->SetWriteState(this->TransferDone); };
  void SetWriteStateCancelled() { this->SetWriteState(this->Cancelled); };
  const char *GetWriteStateAsString() { return this->GetStateAsString(this->WriteState); };

  ///
  /// Get the file's absolute path from the file name and the mrml scene root
  /// dir. GetFullnameFromFileName calls GetFullNameFromNthFileName with -1.
  std::string GetFullNameFromFileName();
  std::string GetFullNameFromNthFileName(int n);

  ///
  /// Check to see if this storage node can handle the file type in the input
  /// string. If input string is null, check URI, then check FileName. Returns
  /// 1 if is supported, 0 otherwise.
  /// Subclasses should implement this method.
  virtual int SupportedFileType(const char *fileName);

  ///
  /// Get all the supported read file types
  /// Subclasses should overwrite InitializeSupportedReadFileTypes().
  virtual vtkStringArray* GetSupportedReadFileTypes();

  ///
  /// Get all the supported write file types
  /// Subclasses should overwrite InitializeSupportedWriteFileTypes().
  virtual vtkStringArray* GetSupportedWriteFileTypes();

  ///
  /// Allow to set specific file format that this node will write output.
  vtkSetStringMacro(WriteFileFormat);
  vtkGetStringMacro(WriteFileFormat);

  ///
  /// Add in another file name to the list of file names
  unsigned int AddFileName (const char *fileName);
  ///
  /// Clear the array of file names
  void ResetFileNameList();

  ///
  /// See how many file names were generated during ExecuteInformation
  int GetNumberOfFileNames() const
  {
    return (int)this->FileNameList.size();
  };

  ///
  /// is filename in the filename list already?
  /// returns 1 if yes, 0 if no
  int FileNameIsInList(const char *fileName);

  ///
  /// Add in another URI to the list of URI's
  unsigned int AddURI(const char *uri);

  ///
  /// Get the nth URI from the list of URI's
  const char *GetNthURI(int n);

  ///
  /// Clear the array of URIs
  void ResetURIList();

  ///
  /// Return how many uri names this storage node holds in it's list
  int GetNumberOfURIs()
  {
    return (int)this->URIList.size();
  }

  ///
  /// Set a new data directory for all files
  void SetDataDirectory(const char* dataDirName);
  ///
  /// Set a new URI base for all URI's
  void SetURIPrefix(const char *uriPrefix);

  ///
  /// Return a default file extension for writting
  virtual const char* GetDefaultWriteFileExtension()
    {
    return NULL;
    };

  ///
  /// Set the nth file in FileNameList, checks that it is already defined
  void ResetNthFileName(int n, const char *fileName);
  ///
  /// Set the nth uri in URIList, checks that it is already defined
  void ResetNthURI(int n, const char *uri);

  ///
  /// Checks is file path is a relative path by calling appropriate
  /// method on the scene depending on whether the scene pointer is valid.
  /// returns 0 if it's not relative or the input is null, 1 if it is relative
  int IsFilePathRelative(const char * filepath);

  /// Calcualtes and the absolute path to the input file if the input path is
  /// relative and the scene is defined with a root directory. Sets and then
  /// return TempFileName. Returns null if the input path is null or the path
  /// is relative and the scene is not defined. Returns inputPath if it's absolute.
  const char *GetAbsoluteFilePath(const char *inputPath);

  ///
  /// A temporary file name used to calculate absolute paths
  vtkSetStringMacro(TempFileName);
  vtkGetStringMacro(TempFileName);

  /// Inform that the file that has been last read or write
  /// has been deleted.
  /// Use with care, typically called by the cache manager.
  void InvalidateFile();

  /// Return the last time stamp when a reference node has been
  /// read in or written from.
  vtkTimeStamp GetStoredTime();

  /// Return true if the node can be read in. Used by ReadData to know
  /// if the file can be read into the reference node.
  /// Subclasses must reimplement the method.
  /// Typically it's a check on the node type (e.g. the model storage node
  /// can only read in model nodes)
  /// \sa CanWriteFromReferenceNode, ReadData
  virtual bool CanReadInReferenceNode(vtkMRMLNode* refNode) = 0;
  /// Return true if the node can be written from. Used by WriteData to know
  /// if the file can be written from the reference node.
  /// By default it returns the same than CanReadInReferenceNode.
  /// Subclasses can reimplement the method.
  /// \sa CanReadInReferenceNode, WriteData
  virtual bool CanWriteFromReferenceNode(vtkMRMLNode* refNode);

  ///
  /// Configure the storage node for data exchange. This is an
  /// opportunity to optimize the storage node's settings, for
  /// instance to turn off compression.
  virtual void ConfigureForDataExchange() {};

  /// Helper function for getting extension from a full filename.
  /// It always returns lowercase extension.
  static std::string GetLowercaseExtensionFromFileName(const std::string& filename);

protected:
  vtkMRMLStorageNode();
  ~vtkMRMLStorageNode();
  vtkMRMLStorageNode(const vtkMRMLStorageNode&);
  void operator=(const vtkMRMLStorageNode&);

  /// Does the actual reading. Returns 1 on success, 0 otherwise.
  /// Returns 0 by default (read not supported).
  /// To be reimplemented in subclass.
  virtual int ReadDataInternal(vtkMRMLNode* refNode);

  /// Does the actual writing. Returns 1 on success, 0 otherwise.
  /// Returns 0 by default (write not supported).
  /// To be reimplemented in subclass.
  virtual int WriteDataInternal(vtkMRMLNode* refNode);

  ///
  /// If the URI is not null, fetch it and save it to the node's FileName location or
  /// load directly into the reference node.
  void StageReadData ( vtkMRMLNode *refNode );

  ///
  /// Copy data from the local file location (node->FileName) or node to the remote
  /// location specified by the URI
  void StageWriteData ( vtkMRMLNode *refNode );

  char *FileName;
  char *TempFileName;
  char *URI;
  vtkURIHandler *URIHandler;
  int UseCompression;
  int ReadState;
  int WriteState;

  ///
  /// An array of file names, should contain the FileName but may not
  std::vector<std::string> FileNameList;
  ///
  /// An array of URI's, should contain the URI but may not
  std::vector<std::string> URIList;
  /// List of supported extensions to read in
  vtkStringArray* SupportedReadFileTypes;

  /// List of supported extensions to write in
  vtkStringArray* SupportedWriteFileTypes;
  char* WriteFileFormat;

  /// Initialize all the supported read file types
  /// Subclasses can derive this method to initialize SupportedReadFileTypes
  virtual void InitializeSupportedReadFileTypes();
  ///
  /// Initialize all the supported write file types
  /// Subclasses should use this method to initialize SupportedWriteFileTypes.
  virtual void InitializeSupportedWriteFileTypes();

  /// Time when data was last read or written.
  /// This is used by the storable node to know when it needs to save its data
  /// Can be reset with InvalidateFile.
  /// \sa InvalidateFile
  vtkTimeStamp* StoredTime;
};

#endif
