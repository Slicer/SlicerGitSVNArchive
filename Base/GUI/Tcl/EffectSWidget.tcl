
package require Itcl

#########################################################
#
if {0} { ;# comment

  EffectSWidget a superclass for editor effects

  - subclasses of this need to define their specialness

# TODO : 

}
#
#########################################################

#
#########################################################
# ------------------------------------------------------------------
#                             EffectSWidget
# ------------------------------------------------------------------
#
# The class definition - define if needed (not when re-sourcing)
#
if { [itcl::find class EffectSWidget] == "" } {

  itcl::class EffectSWidget {

    inherit SWidget

    constructor {args} {}
    destructor {}

    public variable scope "all"
    public variable animationSteps "10"
    public variable animationDelay "200"
    public variable exitCommand ""

    variable _renderer ""
    variable _startPosition "0 0 0"
    variable _currentPosition "0 0 0"
    variable _cursorActors ""
    variable _outputLabel ""
    variable _extractedBackground ""
    variable _extractedLabel ""
    variable _Label ""
    variable _observerRecords "" ;# list of the observers so we can clean up
    variable _cursorAnimationTag ""
    variable _cursorAnimationState 0
    variable _scopeOptions "all" ;# popup will appear if this is a list of options

    # methods
    method processEvent {{caller ""} {event ""}} {}
    method preProcessEvent {{caller ""} {event ""}} {}
    method positionCursor {} {}
    method createCursor {} {}
    method setCursor {imageData} { $o(cursorMapper) SetInput $imageData }
    method preview {} {}
    method apply {} {}
    method postApply {} {}
    method getVisibleCorners { layer {slicePaint ""} } {}
    method getLayerIJK { layer x y } {}
    method getInputLayer { layer } {}
    method getInputBackground {} {}
    method getInputForeground {} {}
    method setInputForeground { im } {}
    method setInputImageData { im ID } {}
    method getInputLabel {} {}
    method swapInputForegroundLabel {foregroundID labelID} {}
    method getOutputLabel {} {}
    method getOptionsFrame {} {}
    method buildOptions {} {}
    method tearDownOptions {} {}
    method previewOptions {} {}
    method applyOptions {} {}
    method setMRMLDefaults {} {}
    method updateGUIFromMRML {} {}
    method flashCursor { {repeat 1} {delay 50} } {}
    method animateCursor { {onOff "on"} } {}
    method setAnimationState { p } {}
    method setProgressFilter { filter {description ""} } {}
    method progressCallback { event caller description } {}
    method errorDialog { message } {}
  }
}

# ------------------------------------------------------------------
#                        CONSTRUCTOR/DESTRUCTOR
# ------------------------------------------------------------------
itcl::body EffectSWidget::constructor {sliceGUI} {

  $this configure -sliceGUI $sliceGUI
  $sliceGUI SetActiveLeftButtonTool $this

  # don't update annotations while effect is active
  $this configure -forceAnnotationsOff 1
 
  $this createCursor

  set _startPosition "0 0 0"
  set _currentPosition "0 0 0"

  $this processEvent

  set events {
    LeftButtonPressEvent LeftButtonReleaseEvent 
    RightButtonPressEvent RightButtonReleaseEvent 
    MouseMoveEvent KeyPressEvent EnterEvent LeaveEvent
  }
  foreach event $events {
    $::slicer3::Broker AddObservation $sliceGUI $event "::SWidget::ProtectedCallback $this processEvent $sliceGUI $event"
  }

  set node [[$sliceGUI GetLogic] GetSliceNode]
  $::slicer3::Broker AddObservation $node DeleteEvent "::SWidget::ProtectedDelete $this"
  $::slicer3::Broker AddObservation $node AnyEvent "::SWidget::ProtectedCallback $this processEvent $node"

  set node [EditorGetParameterNode]
  $::slicer3::Broker AddObservation $node DeleteEvent "::SWidget::ProtectedDelete $this"
  $::slicer3::Broker AddObservation $node AnyEvent "::SWidget::ProtectedCallback $this processEvent $node"
}

itcl::body EffectSWidget::destructor {} {

  $sliceGUI SetActiveLeftButtonTool ""
  $this animateCursor off
  $this tearDownOptions

  if { [info command $_renderer] != "" } {
    foreach a $_cursorActors {
      $_renderer RemoveActor2D $a
    }
    foreach a $_actors {
      $_renderer RemoveActor2D $a
    }
    [$sliceGUI GetSliceViewer] RequestRender
  }

  if { $_outputLabel != "" } {
    $_outputLabel Delete
  }

  if { $exitCommand != "" } {
    eval $exitCommand
  }

}

itcl::configbody EffectSWidget::scope {
  if { [lsearch $_scopeOptions $scope] == -1 } {
    set scope "all"
    error "invalid scope for EffectSWidget $this"
  }
}

# ------------------------------------------------------------------
#                             METHODS
# ------------------------------------------------------------------

itcl::body EffectSWidget::createCursor {} {

  set o(cursorDummyImage) [vtkNew vtkImageData]
  $o(cursorDummyImage) AllocateScalars
  set o(cursorMapper) [vtkNew vtkImageMapper]
  $o(cursorMapper) SetInput $o(cursorDummyImage)
  set o(cursorActor) [vtkNew vtkActor2D]
  $o(cursorActor) SetMapper $o(cursorMapper)
  $o(cursorMapper) SetColorWindow 255
  $o(cursorMapper) SetColorLevel 128

  lappend _cursorActors $o(cursorActor)

  set _renderer [$_renderWidget GetRenderer]
  foreach actor $_cursorActors {
    $_renderer AddActor2D $o(cursorActor)
  }

  $o(cursorActor) VisibilityOff
}

itcl::body EffectSWidget::positionCursor {} {
  set xyzw [$this rasToXY $_currentPosition]
  foreach {x y z w} $xyzw {}

  if { $x == "nan" } {
    puts "Bad cursor position in $this"
    return
  }

  set x [expr $x + 16]
  set y [expr $y - 32]
  foreach actor $_cursorActors {
    eval $actor SetPosition $x $y
  }
}

itcl::body EffectSWidget::getOptionsFrame { } {
  return [EditorGetOptionsFrame $::Editor(singleton)]
}

itcl::body EffectSWidget::buildOptions { } {
  if { [llength $_scopeOptions] > 1 } {
    set o(scopeOption) [vtkNew vtkKWMenuButtonWithLabel]
    set menuButton [$o(scopeOption) GetWidget]
    $o(scopeOption) SetParent [$this getOptionsFrame]
    $o(scopeOption) Create
    $o(scopeOption) SetLabelText "Scope: "
    $o(scopeOption) SetBalloonHelpString "Choose the scope for applying this tool.  Scope of 'visible' refers to contents of Red slice by default (or slice clicked in)"
    foreach s $_scopeOptions {
      [$menuButton GetMenu] AddRadioButton $s
    }
    $menuButton SetValue $scope
    pack [$o(scopeOption) GetWidgetName] \
      -side top -anchor e -fill x -padx 2 -pady 2 

    set tag [$menuButton AddObserver AnyEvent "::SWidget::ProtectedCallback $this processEvent $menuButton"]
    lappend _observerRecords "$menuButton $tag"
  }
}

itcl::body EffectSWidget::tearDownOptions { } {
  foreach w "scopeOption" {
    if { [info exists o($w)] } {
      $o($w) SetParent ""
      pack forget [$o($w) GetWidgetName] 
    }
  }
}



itcl::body EffectSWidget::flashCursor { {repeat 1} {delay 50} } {

  for {set i 0} {$i < $repeat} {incr i} {
    set oldVisibility [$o(cursorActor) GetVisibility]
    $o(cursorActor) SetVisibility [expr !$oldVisibility]
    [$sliceGUI GetSliceViewer] RequestRender
    update
    after $delay
    $o(cursorActor) SetVisibility $oldVisibility
    [$sliceGUI GetSliceViewer] RequestRender
    update
  }
}

#
# background animation for the cursor
# - useful for showing events as 'pending'
#
itcl::body EffectSWidget::animateCursor { {onOff "on"} } {
  
  if { $onOff == "off" } {
    if { $_cursorAnimationTag != "" } {
      after cancel $_cursorAnimationTag
      set $_cursorAnimationTag ""
    }
    $this setAnimationState 1
    set _cursorAnimationState 0
    [$sliceGUI GetSliceViewer] RequestRender
    return
  }

  set p [expr $_cursorAnimationState / (1.0 * $animationSteps)]

  $this setAnimationState $p

  # force a render
  [$sliceGUI GetSliceViewer] RequestRender

  incr _cursorAnimationState
  set _cursorAnimationTag [after $animationDelay "$this animateCursor on"]
}

#
# for virtual override 
# - p will be 0 to 1 loop
#
itcl::body EffectSWidget::setAnimationState { p } {
  # example - set opacity of actor in cosine
  # (won't work for image actors)
  # see ThresholdEffect for example on image
  set amt [expr 0.5 * (1 + cos(6.2831852 * ($p - floor($p)))) ]
  foreach a $_cursorActors {
    [$a GetProperty] SetOpacity $amt
  }
}

#
# returns 1 if the event is 'swallowed' by
# the superclass, otherwise returns 0 and the
# subclass should do normal processing
#
itcl::body EffectSWidget::preProcessEvent { {caller ""} {event ""} } {

  if { [info command $sliceGUI] == "" } {
    # the sliceGUI was deleted behind our back, so we need to 
    # self destruct
    itcl::delete object $this
    return 1
  }

  set grabID [$sliceGUI GetGrabID]
  if { ($grabID != "") && ($grabID != $this) } {
    # some other widget wants these events
    # -- we can position wrt the current slice node
    $this positionCursor
    [$sliceGUI GetSliceViewer] RequestRender
    return 1
  }

  #
  # if the caller was the parameter node, invoke the subclass's 
  # updateGUIFromMRML method which will copy the parameters into the 
  # GUI and into the configuration options of the effect
  #
  if { $caller == [EditorGetParameterNode] } {
    $this updateGUIFromMRML
    return 1
  }

  # propagate menu selection to all effects
  if { [info exists o(scopeOption)] } {
    set menuButton [$o(scopeOption) GetWidget]
    if { $caller == $menuButton } {
      set scope [$menuButton GetValue]
      EffectSWidget::ConfigureAll EffectSWidget -scope $scope
      return 1
    }
  }

  switch $event {
    "KeyPressEvent" {
      set key [$_interactor GetKeySym]
      if { [lsearch "Escape z y" $key] != -1 } {
        $sliceGUI SetCurrentGUIEvent "" ;# reset event so we don't respond again
        $sliceGUI SetGUICommandAbortFlag 1
        switch [$_interactor GetKeySym] {
          "Escape" {
            after idle ::EffectSWidget::RemoveAll
            return 1
          }
          "z" {
            EditorPerformPreviousCheckPoint
            return 1
          }
          "y" {
            EditorPerformNextCheckPoint
            return 1
          }
        }
      } else {
        # puts "effect ignoring $key"
      }
    }
  }

  return 0
}

#
# manage the data to work on:
# - input is always the current contents of the 
#   background and label layers
#   -- can be either the whole volume or just visible portion
# - output is a temp buffer that gets re-inserted into
#   the label layer using the postApply method
# 

   
# return the pixel index for the current layer at the given screen space location
# taking into account the current scope
itcl::body EffectSWidget::getLayerIJK { layer x y } {
  set capLayer [string totitle $layer]
  switch $scope {
    "all" {
      $this queryLayers $x $y
      return [list $_layers($layer,i) $_layers($layer,j) $_layers($layer,k)]
    }
    "visible" {
      foreach {windoww windowh} [[$_interactor GetRenderWindow] GetSize] {}
      set vdcX [expr (1.0 * $x) / $windoww]
      set vdcY [expr (1.0 * $y) / $windowh]
      set capLayer [string totitle $layer]
      set imageData [set _extracted${capLayer}]
      foreach dim {w h d} size [$imageData GetDimensions] {
        set $dim [expr $size - 1]
      }
      set i [expr round($vdcX * $w)]
      set j [expr round($vdcY * $h)]
      return [list $i $j 0]
    }
  }
}

itcl::body EffectSWidget::getInputBackground {} {
  return [$this getInputLayer background]
}

itcl::body EffectSWidget::getInputForeground {} {

  set numCnodes [$::slicer3::MRMLScene GetNumberOfNodesByClass "vtkMRMLSliceCompositeNode"]
  set j 0
  set cnode [$::slicer3::MRMLScene GetNthNodeByClass $j "vtkMRMLSliceCompositeNode"]
  set foregroundID [$cnode GetForegroundVolumeID]
  if { $foregroundID != "" } {
   
      set numVolNodes [$::slicer3::MRMLScene GetNumberOfNodesByClass "vtkMRMLScalarVolumeNode"]
      for { set j 0 } { $j < $numVolNodes } { incr j } {
    set vnode [$::slicer3::MRMLScene GetNthNodeByClass $j "vtkMRMLScalarVolumeNode"]
          set vID [$vnode GetID]
          if { $vID == $foregroundID } {
        return [$vnode GetImageData]
          }
      }   
  } else {
   return ""
  }

  return ""
 
}


itcl::body EffectSWidget::swapInputForegroundLabel { foregroundID  labelID} {

  set numCnodes [$::slicer3::MRMLScene GetNumberOfNodesByClass "vtkMRMLSliceCompositeNode"]
  set j 0
  set cnode [$::slicer3::MRMLScene GetNthNodeByClass $j "vtkMRMLSliceCompositeNode"]
  set fID [$cnode GetForegroundVolumeID]
  set lID [$cnode GetLabelVolumeID]
  
  if { $fID != "" && $fID == $foregroundID && $lID == $labelID } {
     puts "swapping foreground $foregroundID and label $labelID"
      for { set j 0 } { $j < $numCnodes } { incr j } {
        set cnode [$::slicer3::MRMLScene GetNthNodeByClass $j "vtkMRMLSliceCompositeNode"]
        $cnode SetReferenceLabelVolumeID $foregroundID
        $cnode SetReferenceForegroundVolumeID $labelID
      }
   }
}


itcl::body EffectSWidget::setInputImageData { im ID } {


   set numVolNodes [$::slicer3::MRMLScene GetNumberOfNodesByClass "vtkMRMLScalarVolumeNode"]
   set foundNode 0

    for { set j 0 } { $foundNode == 0 && $j < $numVolNodes } { incr j } {
       set vnode [$::slicer3::MRMLScene GetNthNodeByClass $j "vtkMRMLScalarVolumeNode"]
       set vID [$vnode GetID]
       if { $vID == $ID } {
            $vnode SetAndObserveImageData $im
            $vnode Modified
           #$vnode SetImageData $im
            set foundNode 1
        }
    }
}




itcl::body EffectSWidget::setInputForeground { im } {

  catch {
  set numCnodes [$::slicer3::MRMLScene GetNumberOfNodesByClass "vtkMRMLSliceCompositeNode"]
  for { set k 0 } { $k < $numCnodes } {incr k} {
     set cnode [$::slicer3::MRMLScene GetNthNodeByClass $k "vtkMRMLSliceCompositeNode"]
     set foregroundID [$cnode GetForegroundVolumeID]
     if { $foregroundID != "" } {
   
        set numVolNodes [$::slicer3::MRMLScene GetNumberOfNodesByClass "vtkMRMLScalarVolumeNode"]
        for { set j 0 } { $j < $numVolNodes } { incr j } {
      set vnode [$::slicer3::MRMLScene GetNthNodeByClass $j "vtkMRMLScalarVolumeNode"]
            set vID [$vnode GetID]
            if { $vID == $foregroundID } {
          $vnode SetAndObserveImageData $im
                $vnode Modified
            }
         }   
     }
  } 
  } errCopy
  if {$errCopy != "" } { puts "Error in copying foreground $errCopy"} 

}



itcl::body EffectSWidget::getInputLabel {} {

  #
  # check for 'short' label and change type if needed
  # - required for compatibility with legacy tools
  #
  set logic [[$sliceGUI GetLogic]  GetLabelLayer]
  set node [$logic GetVolumeNode]
  if { $node == "" } {
    return ""
  }
  set imageLabel [$node GetImageData]
  if { [$imageLabel GetScalarTypeAsString] != "short" } {
    $this errorDialog "Warning: label map is being converted to type 'short' for processing."  
    set cast [vtkImageCast New]
    $cast SetInput $imageLabel
    $cast SetOutputScalarTypeToShort
    $cast Update
    $imageLabel DeepCopy [$cast GetOutput]
    $cast Delete
  }

  #
  # return full or subset of image for this layer
  #
  return [$this getInputLayer label]
}

itcl::body EffectSWidget::getInputLayer { layer } {
  set capLayer [string totitle $layer]
  set logic [[$sliceGUI GetLogic]  Get${capLayer}Layer]
  set node [$logic GetVolumeNode]
  if { $node == "" } {
    return ""
  }
  set image${capLayer} [$node GetImageData]
  switch $scope {
    "all" {
      return [set image${capLayer}]
    }
    "visible" {
      if { [set _extracted${capLayer}] == "" } {
        set _extracted${capLayer} [vtkImageData New]
      }
      set slicePaint [vtkImageSlicePaint New]
      $slicePaint SetWorkingImage [set image${capLayer}]
      $slicePaint SetExtractImage [set _extracted${capLayer}]
      $this getVisibleCorners $layer $slicePaint
      $slicePaint Paint
      $slicePaint Delete
      #EffectSWidget::ViewVisible $layer [set _extracted${capLayer}]
      return [set _extracted${capLayer}]
    }
  }
  return ""
}

itcl::body EffectSWidget::getOutputLabel {} {
  if { $_outputLabel == "" } {
    set _outputLabel [vtkImageData New]
  }
  return $_outputLabel
}


itcl::body EffectSWidget::postApply {} {
  set logic [[$sliceGUI GetLogic]  GetLabelLayer]
  set node [$logic GetVolumeNode]
  EditorStoreCheckPoint $node
  set targetImage [$node GetImageData]
  switch $scope {
    "all" {
      if { $_outputLabel != "" } {
        $targetImage DeepCopy $_outputLabel
      }
    }
    "visible" {
      set slicePaint [vtkImageSlicePaint New]
      $slicePaint SetWorkingImage $targetImage
      $slicePaint SetReplaceImage $_outputLabel
      $this getVisibleCorners label $slicePaint
      $slicePaint Paint
      $slicePaint Delete
    }
    default {
      error "bad scope: should be all or visible, not $scope"
    }
  }
  # TODO: workaround for new pipeline in slicer4
  # - editing image data of the calling modified on the node
  #   does not pull the pipeline chain
  # - so we trick it by changing the image data first
  set workaround 1
  if { $workaround } {
    if { ![info exists o(tempImageData)] } {
      set o(tempImageData) [vtkNew vtkImageData]
    }
    set imageData [$node GetImageData]
    $node SetAndObserveImageData $o(tempImageData)
    $node SetAndObserveImageData $imageData
  } else {
    $node SetModifiedSinceRead 1
    $node Modified
  }
  foreach imageData {_outputLabel _extractedBackground _extractedLabel} {
    if { [set $imageData] != "" } {
      [set $imageData] Delete
      set $imageData ""
    }
  }
}

itcl::body EffectSWidget::getVisibleCorners { layer {slicePaint ""} } {

  #
  # return a nested list of ijk coordinates representing
  # the indices of the corners of the currently visible 
  # slice view
  # - optionally set those as the corners of a vtkImageSlicePaint
  #

  $this queryLayers 0 0 0
  foreach {windoww windowh} [[$_interactor GetRenderWindow] GetSize] {}
  set xyCorners [list [list 0 0] [list $windoww 0] \
                  [list 0 $windowh] [list $windoww $windowh]]
  set ijkCorners ""
  foreach xy $xyCorners {
    foreach {x y} $xy {}
    set ijkl [$_layers($layer,xyToIJK) MultiplyPoint $x $y 0 1]
    set intIJKL ""
    foreach element $ijkl {
      lappend intIJKL [expr round($element)]
    }
    lappend ijkCorners [lrange $intIJKL 0 2]
  }


  if { $slicePaint != "" } {
    set corners {TopLeft TopRight BottomLeft BottomRight}
    foreach corner $corners ijk $ijkCorners {
      eval $slicePaint Set$corner $ijk
    }
  }

  return $ijkCorners
}

#
# default implementations of methods to be overridden by subclass
#

itcl::body EffectSWidget::preview {} {
  # to be overridden by subclass
}


itcl::body EffectSWidget::processEvent { {caller ""} {event ""} } {
  # to be overridden by subclass
  # - should include call to superclass preProcessEvent
  #   to handle 'friendly' interaction with other SWidgets

  $this preProcessEvent

  # your event processing can replace the dummy code below...
  #
  set _currentPosition [$this xyToRAS [$_interactor GetEventPosition]]

  switch $event {
    "LeftButtonPressEvent" {
      $this apply
      $sliceGUI SetGUICommandAbortFlag 1
    }
    "EnterEvent" {
      $o(cursorActor) VisibilityOn
    }
    "LeaveEvent" {
      $o(cursorActor) VisibilityOff
      $this cancelDelayedAnnotation
    }
  }


  $this positionCursor
  [$sliceGUI GetSliceViewer] RequestRender

}

itcl::body EffectSWidget::apply {} {
  # default behavior, just flash...
  $this flashCursor 3
}

itcl::body EffectSWidget::errorDialog { errorText } {
  if { [info command vtkKWMessageDialog] == "" } {
    puts "Error: $errorText"
  } else {
    set dialog [vtkKWMessageDialog New]
    $dialog SetParent [$::slicer3::ApplicationGUI GetMainSlicerWindow]
    $dialog SetMasterWindow [$::slicer3::ApplicationGUI GetMainSlicerWindow]
    $dialog SetStyleToMessage
    $dialog SetText $errorText
    $dialog Create
    $dialog Invoke
    $dialog Delete
  }
}

itcl::body EffectSWidget::setProgressFilter { filter {description ""} } {

  foreach event {StartEvent ProgressEvent EndEvent} {
    $::slicer3::Broker AddObservation $filter $event [list $this progressCallback $event $filter "$description"]
  }

}

itcl::body EffectSWidget::progressCallback { event caller description } {

  set mainWindow [$::slicer3::ApplicationGUI GetMainSlicerWindow]
  set progressGauge [$mainWindow GetProgressGauge]

  switch $event {
    "StartEvent" { 
      $mainWindow SetStatusText "$description"
      $progressGauge SetValue 0
    }
    "ProgressEvent" {
      $progressGauge SetValue [expr 100 * [$caller GetProgress]]
    }
    "DeleteEvent" - 
    "EndEvent" {
      $progressGauge SetValue 0
      $mainWindow SetStatusText ""
    }
  }
}



#
# helper procs to manage effects
#

proc EffectSWidget::Add {effect} {
  foreach sw [itcl::find objects -class SliceSWidget] {
    set sliceGUI [$sw cget -sliceGUI]
    if { [info command $sliceGUI] != "" } {
      $effect #auto $sliceGUI
    }
  }
}

proc EffectSWidget::RotateToVolumePlanes {} {
  foreach sw [itcl::find objects -class SliceSWidget] {
    set sliceGUI [$sw cget -sliceGUI]
    if { [info command $sliceGUI] != "" } {
      set logic [$sliceGUI GetLogic]
      set sliceNode [$logic GetSliceNode]
      set volumeNodeID [[$logic GetSliceCompositeNode] GetBackgroundVolumeID]
      set volumeNode [$::slicer3::MRMLScene GetNodeByID $volumeNodeID]
      $sliceNode RotateToVolumePlane $volumeNode
      $sliceNode UpdateMatrices
    }
  }
}

proc EffectSWidget::RemoveAll {} {
  foreach ew [itcl::find objects -isa EffectSWidget] {
    itcl::delete object $ew
  }
}

proc EffectSWidget::Remove {effect} {
  foreach pw [itcl::find objects -class $effect] {
    itcl::delete object $pw
  }
}

proc EffectSWidget::Toggle {effect} {
  if { [itcl::find objects -class $effect] == "" } {
    EffectSWidget::Add $effect
  } else {
    EffectSWidget::Remove $effect
  }
}

proc EffectSWidget::ConfigureAll { effect args } {
  foreach pw [itcl::find objects -isa $effect] {
    eval $pw configure $args
  }
}

proc EffectSWidget::SetCursorAll { effect imageData } {

  foreach ew [itcl::find objects -class $effect] {
    $ew setCursor $imageData
  }
}

# for debugging layer/scope image extraction
proc EffectSWidget::ViewVisible { layer imageData } {

  if { [info command viewer$layer] == "" } {
    vtkImageViewer viewer$layer
    viewer$layer SetColorWindow 2
    viewer$layer SetColorLevel 1
  }

  viewer$layer SetInput $imageData
  viewer$layer Render
}
