/*=========================================================================

  Copyright (c) Brigham and Women's Hospital (BWH) All Rights Reserved.

  See License.txt or http://www.slicer.org/copyright/copyright.txt for details.

==========================================================================*/

// Slicer includes
#include "vtkSlicerApplicationLogic.h"
#include "vtkMRMLColorLogic.h"
#include "vtkSlicerConfigure.h" // For Slicer_BUILD_CLI_SUPPORT
#include "vtkSlicerTask.h"

// MRML includes
#include <vtkCacheManager.h>
#include <vtkDataIOManagerLogic.h>
#ifdef Slicer_BUILD_CLI_SUPPORT
# include <vtkMRMLCommandLineModuleNode.h>
#endif
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLFreeSurferModelOverlayStorageNode.h>
#include <vtkMRMLFreeSurferModelStorageNode.h>
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLTableNode.h>
#include <vtkMRMLRemoteIOLogic.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

// ITKSYS includes
#include <itksys/SystemTools.hxx>

// STD includes
#include <algorithm>

#ifdef ITK_USE_PTHREADS
# include <unistd.h>
# include <sys/time.h>
# include <sys/resource.h>
#endif

#include <queue>

#include "vtkSlicerApplicationLogicRequests.h"

//----------------------------------------------------------------------------
class ProcessingTaskQueue : public std::queue<vtkSmartPointer<vtkSlicerTask> > {};
class ModifiedQueue : public std::queue<vtkSmartPointer<vtkObject> > {};
class ReadDataQueue : public std::queue<DataRequest*> {};
class WriteDataQueue : public std::queue<DataRequest*> {};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerApplicationLogic);

//----------------------------------------------------------------------------
vtkSlicerApplicationLogic::vtkSlicerApplicationLogic()
{
  this->ProcessingThreader = itk::MultiThreader::New();
  this->ProcessingThreadId = -1;
  this->ProcessingThreadActive = false;
  this->ProcessingThreadActiveLock = itk::MutexLock::New();
  this->ProcessingTaskQueueLock = itk::MutexLock::New();

  this->ModifiedQueueActive = false;
  this->ModifiedQueueActiveLock = itk::MutexLock::New();
  this->ModifiedQueueLock = itk::MutexLock::New();

  this->ReadDataQueueActive = false;
  this->ReadDataQueueActiveLock = itk::MutexLock::New();
  this->ReadDataQueueLock = itk::MutexLock::New();

  this->WriteDataQueueActive = false;
  this->WriteDataQueueActiveLock = itk::MutexLock::New();
  this->WriteDataQueueLock = itk::MutexLock::New();

  this->InternalTaskQueue = new ProcessingTaskQueue;
  this->InternalModifiedQueue = new ModifiedQueue;

  this->InternalReadDataQueue = new ReadDataQueue;
  this->InternalWriteDataQueue = new WriteDataQueue;
}

//----------------------------------------------------------------------------
vtkSlicerApplicationLogic::~vtkSlicerApplicationLogic()
{
  // Note that TerminateThread does not kill a thread, it only waits
  // for the thread to finish.  We need to signal the thread that we
  // want to terminate
  if (this->ProcessingThreadId != -1 && this->ProcessingThreader)
    {
    // Signal the processingThread that we are terminating.
    this->ProcessingThreadActiveLock->Lock();
    this->ProcessingThreadActive = false;
    this->ProcessingThreadActiveLock->Unlock();

    // Wait for the thread to finish and clean up the state of the threader
    this->ProcessingThreader->TerminateThread( this->ProcessingThreadId );

    this->ProcessingThreadId = -1;
    }

  delete this->InternalTaskQueue;

  this->ModifiedQueueLock->Lock();
  while (!(*this->InternalModifiedQueue).empty())
    {
    vtkObject *obj = (*this->InternalModifiedQueue).front();
    (*this->InternalModifiedQueue).pop();
    obj->Delete(); // decrement ref count
    }
  this->ModifiedQueueLock->Unlock();
  delete this->InternalModifiedQueue;
  delete this->InternalReadDataQueue;
  delete this->InternalWriteDataQueue;
}

//----------------------------------------------------------------------------
unsigned int vtkSlicerApplicationLogic::GetReadDataQueueSize()
{
  return static_cast<unsigned int>( (*this->InternalReadDataQueue).size() );
}

//-----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::SetMRMLSceneDataIO(vtkMRMLScene* newMRMLScene,
                                                   vtkMRMLRemoteIOLogic *remoteIOLogic,
                                                   vtkDataIOManagerLogic *dataIOManagerLogic)
{
  if (remoteIOLogic)
    {
    if (remoteIOLogic->GetMRMLScene() != newMRMLScene)
      {
      if (remoteIOLogic->GetMRMLScene())
        {
        remoteIOLogic->RemoveDataIOFromScene();
        }
      remoteIOLogic->SetMRMLScene(newMRMLScene);
      }
    }

  if (dataIOManagerLogic)
    {
    if (dataIOManagerLogic->GetMRMLScene() != newMRMLScene)
      {
      dataIOManagerLogic->SetMRMLScene(newMRMLScene);
      }
    }

  if (newMRMLScene)
    {
    if (remoteIOLogic)
      {
      remoteIOLogic->AddDataIOToScene();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::PropagateFiducialListSelection()
{
  if(!this->GetMRMLScene() || !this->GetSelectionNode())
    {
    return;
    }
  //char *ID = this->GetSelectionNode()->GetActiveFiducialListID();

  // set the Fiducials GUI to show the active list? it's watching the node for
  // now
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  os << indent << "SlicerApplicationLogic:             " << this->GetClassName() << "\n";
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::CreateProcessingThread()
{
  if (this->ProcessingThreadId == -1)
    {
    this->ProcessingThreadActiveLock->Lock();
    this->ProcessingThreadActive = true;
    this->ProcessingThreadActiveLock->Unlock();

    this->ProcessingThreadId
      = this->ProcessingThreader
      ->SpawnThread(vtkSlicerApplicationLogic::ProcessingThreaderCallback,
                    this);

    // Start four network threads (TODO: make the number of threads a setting)
    this->NetworkingThreadIDs.push_back ( this->ProcessingThreader
          ->SpawnThread(vtkSlicerApplicationLogic::NetworkingThreaderCallback,
                    this) );
    /*
     * TODO: it looks like curl is not thread safe by default
     * - maybe there's a setting that cmcurl can have
     *   similar to the --enable-threading of the standard curl build
     *
    this->NetworkingThreadIDs.push_back ( this->ProcessingThreader
          ->SpawnThread(vtkSlicerApplicationLogic::NetworkingThreaderCallback,
                    this) );
    this->NetworkingThreadIDs.push_back ( this->ProcessingThreader
          ->SpawnThread(vtkSlicerApplicationLogic::NetworkingThreaderCallback,
                    this) );
    this->NetworkingThreadIDs.push_back ( this->ProcessingThreader
          ->SpawnThread(vtkSlicerApplicationLogic::NetworkingThreaderCallback,
                    this) );
    */

    // Setup the communication channel back to the main thread
    this->ModifiedQueueActiveLock->Lock();
    this->ModifiedQueueActive = true;
    this->ModifiedQueueActiveLock->Unlock();
    this->ReadDataQueueActiveLock->Lock();
    this->ReadDataQueueActive = true;
    this->ReadDataQueueActiveLock->Unlock();
    this->WriteDataQueueActiveLock->Lock();
    this->WriteDataQueueActive = true;
    this->WriteDataQueueActiveLock->Unlock();

    int delay = 1000;
    this->InvokeEvent(vtkSlicerApplicationLogic::RequestModifiedEvent, &delay);
    this->InvokeEvent(vtkSlicerApplicationLogic::RequestReadDataEvent, &delay);
    this->InvokeEvent(vtkSlicerApplicationLogic::RequestWriteDataEvent, &delay);
    }
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::TerminateProcessingThread()
{
  if (this->ProcessingThreadId != -1)
    {
    this->ModifiedQueueActiveLock->Lock();
    this->ModifiedQueueActive = false;
    this->ModifiedQueueActiveLock->Unlock();

    this->ReadDataQueueActiveLock->Lock();
    this->ReadDataQueueActive = false;
    this->ReadDataQueueActiveLock->Unlock();

    this->WriteDataQueueActiveLock->Lock();
    this->WriteDataQueueActive = false;
    this->WriteDataQueueActiveLock->Unlock();

    this->ProcessingThreadActiveLock->Lock();
    this->ProcessingThreadActive = false;
    this->ProcessingThreadActiveLock->Unlock();

    this->ProcessingThreader->TerminateThread( this->ProcessingThreadId );
    this->ProcessingThreadId = -1;

    std::vector<int>::const_iterator idIterator;
    idIterator = this->NetworkingThreadIDs.begin();
    while (idIterator != this->NetworkingThreadIDs.end())
      {
      this->ProcessingThreader->TerminateThread( *idIterator );
      ++idIterator;
      }
    this->NetworkingThreadIDs.clear();

    }
}

//----------------------------------------------------------------------------
ITK_THREAD_RETURN_TYPE
vtkSlicerApplicationLogic
::ProcessingThreaderCallback( void *arg )
{

#ifdef ITK_USE_WIN32_THREADS
  // Adjust the priority of this thread
  SetThreadPriority(GetCurrentThread(),
                    THREAD_PRIORITY_BELOW_NORMAL);
#endif

#ifdef ITK_USE_PTHREADS
  // Adjust the priority of all PROCESS level threads.  Not a perfect solution.
  int which = PRIO_PROCESS;
  id_t pid;
  int priority = 20;
  int ret;

  pid = getpid();
  ret = setpriority(which, pid, priority);
  (void)ret; // unused variable
#endif

  // pull out the reference to the appLogic
  vtkSlicerApplicationLogic *appLogic
    = (vtkSlicerApplicationLogic*)
    (((itk::MultiThreader::ThreadInfoStruct *)(arg))->UserData);

  // Tell the app to start processing any tasks slated for the
  // processing thread
  appLogic->ProcessProcessingTasks();

  return ITK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::ProcessProcessingTasks()
{
  int active = true;
  vtkSmartPointer<vtkSlicerTask> task = 0;

  while (active)
    {
    // Check to see if we should be shutting down
    this->ProcessingThreadActiveLock->Lock();
    active = this->ProcessingThreadActive;
    this->ProcessingThreadActiveLock->Unlock();

    if (active)
      {
      // pull a task off the queue
      this->ProcessingTaskQueueLock->Lock();
      if ((*this->InternalTaskQueue).size() > 0)
        {
        // std::cout << "Number of queued tasks: " << (*this->InternalTaskQueue).size() << std::endl;

        // only handle processing tasks in this thread
        task = (*this->InternalTaskQueue).front();
        if ( task->GetType() == vtkSlicerTask::Processing )
          {
          (*this->InternalTaskQueue).pop();
          }
        else
          {
          task = NULL;
          }
        }
      this->ProcessingTaskQueueLock->Unlock();

      // process the task (should this be in a separate thread?)
      if (task)
        {
        task->Execute();
        task = 0;
        }
      }

    // busy wait
    itksys::SystemTools::Delay(100);
    }
}

ITK_THREAD_RETURN_TYPE
vtkSlicerApplicationLogic
::NetworkingThreaderCallback( void *arg )
{

#ifdef ITK_USE_WIN32_THREADS
  // Adjust the priority of this thread
  SetThreadPriority(GetCurrentThread(),
                    THREAD_PRIORITY_BELOW_NORMAL);
#endif

#ifdef ITK_USE_PTHREADS
  // Adjust the priority of all PROCESS level threads.  Not a perfect solution.
  int which = PRIO_PROCESS;
  id_t pid;
  int priority = 20;
  int ret;

  pid = getpid();
  ret = setpriority(which, pid, priority);
  (void)ret; // unused variable
#endif

  // pull out the reference to the appLogic
  vtkSlicerApplicationLogic *appLogic
    = (vtkSlicerApplicationLogic*)
    (((itk::MultiThreader::ThreadInfoStruct *)(arg))->UserData);

  // Tell the app to start processing any tasks slated for the
  // processing thread
  appLogic->ProcessNetworkingTasks();

  return ITK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::ProcessNetworkingTasks()
{
  int active = true;
  vtkSmartPointer<vtkSlicerTask> task = 0;

  while (active)
    {
    // Check to see if we should be shutting down
    this->ProcessingThreadActiveLock->Lock();
    active = this->ProcessingThreadActive;
    this->ProcessingThreadActiveLock->Unlock();

    if (active)
      {
      // pull a task off the queue
      this->ProcessingTaskQueueLock->Lock();
      if ((*this->InternalTaskQueue).size() > 0)
        {
        // std::cout << "Number of queued tasks: " << (*this->InternalTaskQueue).size() << std::endl;
        task = (*this->InternalTaskQueue).front();
        if ( task->GetType() == vtkSlicerTask::Networking )
          {
          (*this->InternalTaskQueue).pop();
          }
        else
          {
          task = NULL;
          }
        }
      this->ProcessingTaskQueueLock->Unlock();

      // process the task (should this be in a separate thread?)
      if (task)
        {
        task->Execute();
        task = 0;
        }
      }

    // busy wait
    itksys::SystemTools::Delay(100);
    }
}

//----------------------------------------------------------------------------
int vtkSlicerApplicationLogic::ScheduleTask( vtkSlicerTask *task )
{
  // only schedule a task if the processing task is up
  this->ProcessingThreadActiveLock->Lock();
  int active = this->ProcessingThreadActive;
  this->ProcessingThreadActiveLock->Unlock();
  if (!active)
    {
    return false;
    }

  this->ProcessingTaskQueueLock->Lock();
  (*this->InternalTaskQueue).push( task );
  this->ProcessingTaskQueueLock->Unlock();
  return true;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSlicerApplicationLogic::RequestModified(vtkObject *obj)
{
  // only request a Modified if the Modified queue is up
  this->ModifiedQueueActiveLock->Lock();
  int active = this->ModifiedQueueActive;
  this->ModifiedQueueActiveLock->Unlock();
  if (!active)
    {
    // could not request the Modified
    return 0;
    }

  obj->Register(this);
  this->ModifiedQueueLock->Lock();
  this->RequestTimeStamp.Modified();
  vtkMTimeType uid = this->RequestTimeStamp.GetMTime();
  (*this->InternalModifiedQueue).push(obj);
  this->ModifiedQueueLock->Unlock();
  return uid;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSlicerApplicationLogic::RequestReadFile(const char *refNode, const char *filename, int displayData, int deleteFile)
{
  // only request to read a file if the ReadData queue is up
  this->ReadDataQueueActiveLock->Lock();
  int active = this->ReadDataQueueActive;
  this->ReadDataQueueActiveLock->Unlock();
  if (!active)
  {
    // could not request the record be added to the queue
    return 0;
  }

  this->ReadDataQueueLock->Lock();
  this->RequestTimeStamp.Modified();
  int uid = this->RequestTimeStamp.GetMTime();
  (*this->InternalReadDataQueue).push(
    new ReadDataRequestFile(refNode, filename, displayData, deleteFile, uid));
  this->ReadDataQueueLock->Unlock();
  return uid;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSlicerApplicationLogic::RequestUpdateParentTransform(const std::string &refNode, const std::string& parentTransformNode)
{
  // only request to read a file if the ReadData queue is up
  this->ReadDataQueueActiveLock->Lock();
  int active = this->ReadDataQueueActive;
  this->ReadDataQueueActiveLock->Unlock();
  if (!active)
    {
    // could not request the record be added to the queue
    return 0;
    }

  this->ReadDataQueueLock->Lock();
  this->RequestTimeStamp.Modified();
  vtkMTimeType uid = this->RequestTimeStamp.GetMTime();
  (*this->InternalReadDataQueue).push(new ReadDataRequestUpdateParentTransform(refNode, parentTransformNode, uid));
  this->ReadDataQueueLock->Unlock();
  return uid;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSlicerApplicationLogic::RequestUpdateSubjectHierarchyLocation(const std::string &updatedNode, const std::string& siblingNode)
{
  // only request to read a file if the ReadData queue is up
  this->ReadDataQueueActiveLock->Lock();
  int active = this->ReadDataQueueActive;
  this->ReadDataQueueActiveLock->Unlock();
  if (!active)
    {
    // could not request the record be added to the queue
    return 0;
    }

  this->ReadDataQueueLock->Lock();
  this->RequestTimeStamp.Modified();
  vtkMTimeType uid = this->RequestTimeStamp.GetMTime();
  (*this->InternalReadDataQueue).push(new ReadDataRequestUpdateSubjectHierarchyLocation(updatedNode, siblingNode, uid));
  this->ReadDataQueueLock->Unlock();
  return uid;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSlicerApplicationLogic::RequestWriteData(const char *refNode, const char *filename)
{
  // only request to write a file if the WriteData queue is up
  this->WriteDataQueueActiveLock->Lock();
  int active = this->WriteDataQueueActive;
  this->WriteDataQueueActiveLock->Unlock();
  if (!active)
    {
    // could not request the record be added to the queue
    return 0;
    }

  this->WriteDataQueueLock->Lock();
  this->RequestTimeStamp.Modified();
  vtkMTimeType uid = this->RequestTimeStamp.GetMTime();
  (*this->InternalWriteDataQueue).push(
    new WriteDataRequestFile(refNode, filename, uid) );
  this->WriteDataQueueLock->Unlock();
  return uid;
}


//----------------------------------------------------------------------------
vtkMTimeType vtkSlicerApplicationLogic::RequestReadScene(
    const std::string& filename,
    std::vector<std::string> &targetIDs,
    std::vector<std::string> &sourceIDs,
    int displayData, int deleteFile)
{
  // only request to read a file if the ReadData queue is up
  this->ReadDataQueueActiveLock->Lock();
  int active = this->ReadDataQueueActive;
  this->ReadDataQueueActiveLock->Unlock();
  if (!active)
    {
    // could not request the record be added to the queue
    return 0;
    }

  this->ReadDataQueueLock->Lock();
  this->RequestTimeStamp.Modified();
  vtkMTimeType uid = this->RequestTimeStamp.GetMTime();
  (*this->InternalReadDataQueue).push(
    new ReadDataRequestScene(targetIDs, sourceIDs, filename, displayData, deleteFile, uid));
  this->ReadDataQueueLock->Unlock();
  return uid;
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::ProcessModified()
{
  // Check to see if we should be shutting down
  this->ModifiedQueueActiveLock->Lock();
  int active = this->ModifiedQueueActive;
  this->ModifiedQueueActiveLock->Unlock();
  if (!active)
    {
    return;
    }

  vtkSmartPointer<vtkObject> obj = 0;
  // pull an object off the queue to modify
  this->ModifiedQueueLock->Lock();
  if ((*this->InternalModifiedQueue).size() > 0)
    {
    obj = (*this->InternalModifiedQueue).front();
    (*this->InternalModifiedQueue).pop();

    // pop off any extra copies of the same object to save some updates
    while (!(*this->InternalModifiedQueue).empty()
           && (obj == (*this->InternalModifiedQueue).front()))
      {
      (*this->InternalModifiedQueue).pop();
      obj->Delete(); // decrement ref count
      }
    }
  this->ModifiedQueueLock->Unlock();

  // Modify the object
  //  - decrement reference count that was increased when it was added to the queue
  if (obj.GetPointer())
    {
    obj->Modified();
    obj->Delete();
    obj = 0;
    }

  // schedule the next timer sooner in case there is stuff in the queue
  // otherwise for a while later
  int delay = (*this->InternalModifiedQueue).size() > 0 ? 0: 200;
  this->InvokeEvent(vtkSlicerApplicationLogic::RequestModifiedEvent, &delay);
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::ProcessReadData()
{
  // Check to see if we should be shutting down
  this->ReadDataQueueActiveLock->Lock();
  int active = this->ReadDataQueueActive;
  this->ReadDataQueueActiveLock->Unlock();
  if (!active)
    {
    return;
    }

  // pull an object off the queue
  DataRequest* req = NULL;
  this->ReadDataQueueLock->Lock();
  if ((*this->InternalReadDataQueue).size() > 0)
    {
    req = (*this->InternalReadDataQueue).front();
    (*this->InternalReadDataQueue).pop();
    }
  this->ReadDataQueueLock->Unlock();

  vtkMTimeType uid = 0;
  if (req)
    {
    uid = req->GetUID();
    req->Execute(this);
    delete req;
    }

  int delay = (*this->InternalReadDataQueue).size() > 0 ? 0: 200;
  // schedule the next timer sooner in case there is stuff in the queue
  // otherwise for a while later
  this->InvokeEvent(vtkSlicerApplicationLogic::RequestReadDataEvent, &delay);
  if (uid)
    {
    this->InvokeEvent(vtkSlicerApplicationLogic::RequestProcessedEvent,
                      reinterpret_cast<void*>(uid));
    }
}

//----------------------------------------------------------------------------
void vtkSlicerApplicationLogic::ProcessWriteData()
{
  // Check to see if we should be shutting down
  this->WriteDataQueueActiveLock->Lock();
  int active = this->WriteDataQueueActive;
  this->WriteDataQueueActiveLock->Unlock();
  if (!active)
    {
    return;
    }

  // pull an object off the queue
  DataRequest *req = NULL;
  this->WriteDataQueueLock->Lock();
  if ((*this->InternalWriteDataQueue).size() > 0)
    {
    req = (*this->InternalWriteDataQueue).front();
    (*this->InternalWriteDataQueue).pop();
    }
  this->WriteDataQueueLock->Unlock();

  if (req)
  {
    vtkMTimeType uid = req->GetUID();
    req->Execute(this);
    delete req;

    // schedule the next timer sooner in case there is stuff in the queue
    // otherwise for a while later
    int delay = (*this->InternalWriteDataQueue).size() > 0 ? 0 : 200;
    this->InvokeEvent(vtkSlicerApplicationLogic::RequestWriteDataEvent, &delay);
    if (uid)
      {
      this->InvokeEvent(vtkSlicerApplicationLogic::RequestProcessedEvent,
        reinterpret_cast<void*>(uid));
      }
  }
}

//----------------------------------------------------------------------------
bool vtkSlicerApplicationLogic::IsEmbeddedModule(const std::string& filePath,
                                                 const std::string& applicationHomeDir,
                                                 const std::string& slicerRevision)
{
  if (filePath.empty())
    {
    vtkGenericWarningMacro( << "filePath is an empty string !");
    return false;
    }
  if (applicationHomeDir.empty())
    {
    vtkGenericWarningMacro( << "applicationHomeDir is an empty string !");
    return false;
    }
  std::string extensionPath = itksys::SystemTools::GetFilenamePath(filePath);
  bool isEmbedded = itksys::SystemTools::StringStartsWith(extensionPath.c_str(), applicationHomeDir.c_str());
#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
  // On MacOSX extensions are installed in the "<Slicer_EXTENSIONS_DIRBASENAME>-<slicerRevision>"
  // folder being a sub directory of the application dir, an extra test is required to make sure the
  // tested filePath doesn't belong to that "<Slicer_EXTENSIONS_DIRBASENAME>-<slicerRevision>" folder.
  // BUG 2848: Since package name can be rename from "Slicer.app" to "Something.app", let's compare
  // using ".app/Contents/" instead of "Slicer_BUNDLE_LOCATION" which is "Slicer.app/Contents/"
  if (isEmbedded && extensionPath.find(".app/Contents/" Slicer_EXTENSIONS_DIRBASENAME "-" + slicerRevision) != std::string::npos)
    {
    isEmbedded = false;
    }
#else
  (void)slicerRevision;
#endif
  return isEmbedded;
}

//----------------------------------------------------------------------------
bool vtkSlicerApplicationLogic::IsPluginInstalled(const std::string& filePath,
                                                  const std::string& applicationHomeDir)
{
  if (filePath.empty())
    {
    vtkGenericWarningMacro( << "filePath is an empty string !");
    return false;
    }
  if (applicationHomeDir.empty())
    {
    vtkGenericWarningMacro( << "applicationHomeDir is an empty string !");
    return false;
    }

  std::string path = itksys::SystemTools::GetFilenamePath(filePath);
  std::string canonicalPath = itksys::SystemTools::GetRealPath(path.c_str());

  if (itksys::SystemTools::StringStartsWith(canonicalPath.c_str(), applicationHomeDir.c_str()))
    {
    return !itksys::SystemTools::FileExists(
          std::string(applicationHomeDir).append("/CMakeCache.txt").c_str(), true);
    }

  std::string root;
  std::string canonicalPathWithoutRoot =
      itksys::SystemTools::SplitPathRootComponent(canonicalPath.c_str(), &root);
  do
    {
    if (itksys::SystemTools::FileExists(
          (root + canonicalPathWithoutRoot + "/CMakeCache.txt").c_str(), true))
      {
      return false;
      }
    canonicalPathWithoutRoot = itksys::SystemTools::GetParentDirectory(canonicalPathWithoutRoot.c_str());
    }
  while(!canonicalPathWithoutRoot.empty());

  return true;
}

//----------------------------------------------------------------------------
bool vtkSlicerApplicationLogic::IsPluginBuiltIn(const std::string& filePath,
                                                  const std::string& applicationHomeDir)
{
  if (filePath.empty())
    {
    vtkGenericWarningMacro( << "filePath is an empty string !");
    return false;
    }
  if (applicationHomeDir.empty())
    {
    vtkGenericWarningMacro( << "applicationHomeDir is an empty string !");
    return false;
    }

  std::string canonicalApplicationHomeDir =
      itksys::SystemTools::GetRealPath(applicationHomeDir.c_str());

  std::string path = itksys::SystemTools::GetFilenamePath(filePath);
  std::string canonicalPath = itksys::SystemTools::GetRealPath(path.c_str());

  bool isBuiltIn = itksys::SystemTools::StringStartsWith(
        canonicalPath.c_str(), canonicalApplicationHomeDir.c_str());

#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
  // On MacOSX extensions are installed in the "<Slicer_EXTENSIONS_DIRBASENAME>-<slicerRevision>"
  // folder being a sub directory of the application dir, an extra test is required to make sure the
  // tested filePath doesn't belong to that "<Slicer_EXTENSIONS_DIRBASENAME>-<slicerRevision>" folder.
  // Since package name can be rename from "Slicer.app" to "Something.app", let's compare
  // using ".app/Contents/" instead of "Slicer_BUNDLE_LOCATION" which is "Slicer.app/Contents/"
  bool macExtension = (canonicalPath.find(".app/Contents/" Slicer_EXTENSIONS_DIRBASENAME "-") != std::string::npos);
#else
  bool macExtension = false;
#endif

  return  isBuiltIn && !macExtension;
}

namespace
{
//----------------------------------------------------------------------------
std::string GetModuleHomeDirectory(const std::string& filePath,
                                   std::string& slicerSubDir,
                                   std::string& moduleTypeSubDir)
{
  if (filePath.empty())
    {
    vtkGenericWarningMacro( << "filePath is an empty string !");
    return std::string();
    }

  // In the current implementation, we assume the path to a module and its resources
  // will respectively have the following structure:
  //   ../lib/Slicer-X.Y/<module-type>/module
  // and
  //   ../share/Slicer-X.Y/<module-type>/<module-name>/

  std::vector<std::string> components;
  itksys::SystemTools::SplitPath(filePath.c_str(), components, true);

  // components[components.size() - 1] -> fileName
  // components[components.size() - 2 - offset] -> {qt-scripted-modules, qt-loadable-modules, ...}
  // components[components.size() - 3 - offset] -> Slicer-X.Y
  // components[components.size() - 4 - offset] -> lib
  // components[0 .. components.size() - 4 - offset] -> Common path to lib and share directory

  if (components.size() < 5)
    {
    // At least 5 components are expected to be able to compute the module home directory
    vtkGenericWarningMacro( << "Failed to compute module home directory given filePath: " << filePath);
    return std::string();
    }

  // offset == 1 if there is an intermediate build directory
  int offset = 0;
  std::string intDir(".");
  std::string possibleIntDir = components.at(components.size() - 2);
  if (!itksys::SystemTools::Strucmp(possibleIntDir.c_str(), "Debug") ||
      !itksys::SystemTools::Strucmp(possibleIntDir.c_str(), "Release") ||
      !itksys::SystemTools::Strucmp(possibleIntDir.c_str(), "RelWithDebInfo") ||
      !itksys::SystemTools::Strucmp(possibleIntDir.c_str(), "MinSizeRel"))
    {
    offset = 1;
    intDir = possibleIntDir;
    }

  moduleTypeSubDir = components.at(components.size() - 2 - offset);
  slicerSubDir = components.at(components.size() - 3 - offset);

  std::string homeDirectory =
      itksys::SystemTools::JoinPath(components.begin(), components.end() - 4 - offset);
  return homeDirectory;
}

} // end of anonymous namespace

//----------------------------------------------------------------------------
std::string vtkSlicerApplicationLogic::GetModuleShareDirectory(const std::string& moduleName,
                                                               const std::string& filePath)
{
  if (moduleName.empty())
    {
    vtkGenericWarningMacro( << "moduleName is an empty string !");
    return std::string();
    }
  if (filePath.empty())
    {
    vtkGenericWarningMacro( << "filePath is an empty string !");
    return std::string();
    }

  std::string slicerSubDir;
  std::string moduleTypeSubDir;
  std::string shareDirectory = GetModuleHomeDirectory(filePath, slicerSubDir, moduleTypeSubDir);

  if (shareDirectory.empty())
    {
    return std::string();
    }

  shareDirectory.append("/share");
  shareDirectory.append("/");
  shareDirectory.append(slicerSubDir);
  shareDirectory.append("/");
  shareDirectory.append(moduleTypeSubDir);
  shareDirectory.append("/");
  shareDirectory.append(moduleName);

  return shareDirectory;
}

//----------------------------------------------------------------------------
std::string vtkSlicerApplicationLogic::GetModuleSlicerXYShareDirectory(const std::string& filePath)
{
  if (filePath.empty())
    {
    vtkGenericWarningMacro( << "filePath is an empty string !");
    return std::string();
    }
  std::string slicerSubDir;
  std::string moduleTypeSubDir;
  std::string shareDirectory = GetModuleHomeDirectory(filePath, slicerSubDir, moduleTypeSubDir);

  if (shareDirectory.empty())
    {
    return std::string();
    }

  shareDirectory.append("/share");
  shareDirectory.append("/");
  shareDirectory.append(slicerSubDir);
  return shareDirectory;
}

//----------------------------------------------------------------------------
std::string vtkSlicerApplicationLogic::GetModuleSlicerXYLibDirectory(const std::string& filePath)
{
  if (filePath.empty())
    {
    vtkGenericWarningMacro( << "filePath is an empty string !");
    return std::string();
    }
  std::string slicerSubDir;
  std::string moduleTypeSubDir;
  std::string libDirectory = GetModuleHomeDirectory(filePath, slicerSubDir, moduleTypeSubDir);
  libDirectory.append("/lib");
  libDirectory.append("/");
  libDirectory.append(slicerSubDir);
  return libDirectory;
}
