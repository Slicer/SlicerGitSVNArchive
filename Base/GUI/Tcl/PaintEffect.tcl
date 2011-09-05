
package require Itcl

#########################################################
#
if {0} { ;# comment

  PaintEffect a class for slicer painting


# TODO : 

}
#
#########################################################

#
#########################################################
# ------------------------------------------------------------------
#                             PaintEffect
# ------------------------------------------------------------------
#
# The class definition - define if needed (not when re-sourcing)
#
if { [itcl::find class PaintEffect] == "" } {

  itcl::class PaintEffect {

    inherit Labeler

    constructor {sliceGUI} {Labeler::constructor $sliceGUI} {}
    destructor {}

    public variable radius 5
    public variable smudge 0
    public variable delayedPaint 1

    variable _startPosition "0 0 0"
    variable _currentPosition "0 0 0"
    variable _paintCoordinates ""
    variable _feedbackActors ""
    variable _lastRadius 0

    # methods
    method processEvent {{caller ""} {event ""}} {}
    method buildOptions {} {}
    method tearDownOptions {} {}
    method setMRMLDefaults {} {}
    method updateMRMLFromGUI {} {}
    method updateGUIFromMRML {} {}

    method positionActors {} {}
    method highlight {} {}
    method createGlyph { {polyData ""} } {}
    method paintBrush {x y} {}
    method paintAddPoint {x y} {}
    method paintFeedback {} {}
    method paintApply {} {}
  }
}

# ------------------------------------------------------------------
#                        CONSTRUCTOR/DESTRUCTOR
# ------------------------------------------------------------------

itcl::body PaintEffect::constructor {sliceGUI} {

  $this configure -sliceGUI $sliceGUI
 
  set o(brush) [vtkNew vtkPolyData]
  set o(rasToXY) [vtkNew vtkMatrix4x4]
  $this createGlyph $o(brush)
  set o(mapper) [vtkNew vtkPolyDataMapper2D]
  set o(actor) [vtkNew vtkActor2D]
  $o(mapper) SetInput $o(brush)
  $o(actor) SetMapper $o(mapper)
  $o(actor) VisibilityOff
  [$_renderWidget GetRenderer] AddActor2D $o(actor)
  lappend _actors $o(actor)

  set _startPosition "0 0 0"
  set _currentPosition "0 0 0"
  
  $this processEvent

}

itcl::body PaintEffect::destructor {} {

  if { [info command $_renderer] != "" } {
    foreach a $_actors {
      $_renderer RemoveActor2D $a
    }
  }
}

itcl::configbody PaintEffect::radius {
  if { $radius != "" } {
    if { $radius != $_lastRadius } {
      $this createGlyph $o(brush)
      set _lastRadius $radius
    }
  }
}

# ------------------------------------------------------------------
#                             METHODS
# ------------------------------------------------------------------

itcl::body PaintEffect::createGlyph { {polyData ""} } {
  #
  # get the brush radius in XY space
  # - assume uniform scaling between XY and RAS which
  #   is enforced by the view interactors
  #
  $o(rasToXY) DeepCopy [$_sliceNode GetXYToRAS]
  $o(rasToXY) Invert
  set xyRadius "10 0 0"
  if { $radius != "" } {
    if { [$o(rasToXY) GetElement 0 0] != 0 } {
      set xyRadius [$o(rasToXY) MultiplyPoint $radius 0 0 0]
    } else {
      set xyRadius [$o(rasToXY) MultiplyPoint 0 $radius 0 0]
    }
  }
  foreach {xRadius yRadius zRadius zero} $xyRadius {}
  set xyRadius [expr sqrt( $xRadius * $xRadius + $yRadius * $yRadius + $zRadius * $zRadius )]

  # make a circle paint brush
  if { $polyData == "" } {
    set polyData [vtkNew vtkPolyData]
  }
  set points [vtkPoints New]
  set lines [vtkCellArray New]
  $polyData SetPoints $points
  $polyData SetLines $lines
  set PI 3.1415926
  set TWOPI [expr $PI * 2]
  set PIoverSIXTEEN [expr $PI / 16]
  set prevPoint ""
  set firstPoint ""
  for { set angle 0 } { $angle <= $TWOPI } { set angle [expr $angle + $PIoverSIXTEEN] } {
    set x [expr $xyRadius * cos($angle)]
    set y [expr $xyRadius * sin($angle)]
    set p [$points InsertNextPoint $x $y 0]
    if { $prevPoint != "" } {
      set idList [vtkIdList New]
      $idList InsertNextId $prevPoint
      $idList InsertNextId $p
      $polyData InsertNextCell 3 $idList  ;# 3 is a VTK_LINE
      $idList Delete
    }
    set prevPoint $p
    if { $firstPoint == "" } {
      set firstPoint $p
    }
  }
  # make the last line in the circle
  set idList [vtkIdList New]
  $idList InsertNextId $p
  $idList InsertNextId $firstPoint
  $polyData InsertNextCell 3 $idList  ;# 3 is a VTK_LINE
  $idList Delete

  $points Delete
  $lines Delete
  return $polyData
}

itcl::body PaintEffect::positionActors { } {

  if { ![info exists o(actor)] } {
    # if called during construction...
    return
  }

  set xyzw [$this rasToXY $_currentPosition]
  eval $o(actor) SetPosition [lrange $xyzw 0 1]
}

itcl::body PaintEffect::highlight { } {

  if { ![info exists o(actor)] } {
    # if called during construction...
    return
  }

  set property [$o(actor) GetProperty]
  $property SetColor 1 1 0
  $property SetLineWidth 1
  set _description ""
  switch $_actionState {
    "dragging" {
      $property SetColor 0 1 0
      set _description "Move mouse with left button down to drag"
    }
    default {
      switch $_pickState {
        "over" {
          $property SetColor 1 1 0
          $property SetLineWidth 2
        }
      }
    }
  }
}

itcl::body PaintEffect::processEvent { {caller ""} {event ""} } {

  if { ![info exists o(brush)] } {
    # event triggered before constructor finished
    return
  }

  # chain to superclass
  chain $caller $event

  if { [info command $sliceGUI] == "" } {
    # the sliceGUI was deleted behind our back, so we need to 
    # self destruct
    itcl::delete object $this
    return
  }

  set grabID [$sliceGUI GetGrabID]
  if { ($grabID != "") && ($grabID != $this) } {
    # some other widget wants these events
    # -- we can position wrt the current slice node
    $this positionActors
    [$sliceGUI GetSliceViewer] RequestRender
    return 
  }

  if { $caller == $sliceGUI } {
    switch $event {
      "LeftButtonPressEvent" {
        set _actionState "painting"
        foreach {x y} [$_interactor GetEventPosition] {}
        if { $smudge } {
          # in smudge mode, set the paint color to be the first pixel you touch
          $this queryLayers $x $y
          EditorSetPaintLabel [$this getPixel $_layers(label,image) \
            $_layers(label,i) $_layers(label,j) $_layers(label,k)]
        }
        $this paintAddPoint $x $y
        $sliceGUI SetGUICommandAbortFlag 1
        $sliceGUI SetGrabID $this
        [$_renderWidget GetRenderWindow] HideCursor
      }
      "MouseMoveEvent" {
        $o(actor) VisibilityOn
        set _currentPosition [$this xyToRAS [$_interactor GetEventPosition]]
        switch $_actionState {
          "painting" {
            foreach {x y} [$_interactor GetEventPosition] {}
            $this paintAddPoint $x $y
          }
          default {
          }
        }
      }
      "LeftButtonReleaseEvent" {
        $this paintApply
        [$_renderWidget GetRenderWindow] ShowCursor
        foreach {x y} [$_interactor GetEventPosition] {}
        $this queryLayers $x $y
        $_layers(label,node) Modified
        set _actionState ""
        $sliceGUI SetGrabID ""
        set _description ""
      }
      "EnterEvent" {
        set _description "Ready to Paint!"
        $o(actor) VisibilityOn
      }
      "LeaveEvent" {
        set _description ""
        $o(actor) VisibilityOff
      }
    }
  }

  $this highlight
  $this positionActors
  [$sliceGUI GetSliceViewer] RequestRender
}

  
itcl::body PaintEffect::buildOptions {} {

  # call superclass version of buildOptions
  chain

  #
  # a radius control
  #
  set o(radius) [vtkNew vtkKWThumbWheel]
  $o(radius) SetParent [$this getOptionsFrame]
  $o(radius) PopupModeOn
  $o(radius) Create
  $o(radius) DisplayEntryAndLabelOnTopOn
  $o(radius) DisplayEntryOn
  $o(radius) DisplayLabelOn
  $o(radius) SetValue [[EditorGetParameterNode] GetParameter Paint,radius]
  $o(radius) SetMinimumValue 0.01
  $o(radius) ClampMinimumValueOn
  [$o(radius) GetLabel] SetText "Radius: "
  $o(radius) SetBalloonHelpString "Set the radius of the paint brush in millimeters"
  pack [$o(radius) GetWidgetName] \
    -side top -anchor e -fill x -padx 2 -pady 2 

  set o(smudge) [vtkNew vtkKWCheckButtonWithLabel]
  $o(smudge) SetParent [$this getOptionsFrame]
  $o(smudge) Create
  $o(smudge) SetLabelText "Smudge: "
  $o(smudge) SetBalloonHelpString "Set the label number automatically by sampling the pixel location where the brush stroke starts."
  [$o(smudge) GetWidget] SetSelectedState [[EditorGetParameterNode] GetParameter Paint,smudge] 
  pack [$o(smudge) GetWidgetName] \
    -side top -anchor e -fill x -padx 2 -pady 2 

  #
  # a cancel button
  #
  set o(cancel) [vtkNew vtkKWPushButton]
  $o(cancel) SetParent [$this getOptionsFrame]
  $o(cancel) Create
  $o(cancel) SetText "Cancel"
  $o(cancel) SetBalloonHelpString "Cancel wand without applying to label map."
  pack [$o(cancel) GetWidgetName] \
    -side right -anchor e -padx 2 -pady 2 

  #
  # event observers 
  # 
  set InvokedEvent 10000
  set ThumbWheelValueChangedEvent 10001
  set SelectedStateChangedEvent 10000
  $::slicer3::Broker AddObservation $o(radius) $ThumbWheelValueChangedEvent "after idle $this updateMRMLFromGUI"
  $::slicer3::Broker AddObservation [$o(smudge) GetWidget] $SelectedStateChangedEvent  "after idle $this updateMRMLFromGUI"
  $::slicer3::Broker AddObservation $o(cancel) $InvokedEvent  "after idle ::EffectSWidget::RemoveAll"

  if { [$this getInputBackground] == "" || [$this getInputLabel] == "" } {
    $this errorDialog "Background and Label map needed for painting"
    after idle ::EffectSWidget::RemoveAll
  }

  $this updateGUIFromMRML
}

itcl::body PaintEffect::updateMRMLFromGUI { } {
  #
  # set the node to the current value of the GUI
  # - this will be saved/restored with the scene
  # - all instances of the effect are observing the node,
  #   so changes will propogate automatically
  #
  chain
  set node [EditorGetParameterNode]
  $node SetParameter "Paint,radius" [$o(radius) GetValue]
  $node SetParameter "Paint,smudge" [[$o(smudge) GetWidget] GetSelectedState]
}

itcl::body PaintEffect::setMRMLDefaults { } {
  chain
  set node [EditorGetParameterNode]
  foreach {param default} {
    radius 5
    smudge 0
  } {
    set pvalue [$node GetParameter Paint,$param] 
    if { $pvalue == "" } {
      $node SetParameter Paint,$param $default
    } 
  }
}


itcl::body PaintEffect::updateGUIFromMRML { } {
  #
  # get the parameter from the node
  # - set default value if it doesn't exist
  #
  chain


  set node [EditorGetParameterNode]
  # set the GUI and effect parameters to match node
  # (only if this is the instance that "owns" the GUI
  set radius [$node GetParameter Paint,radius] 
  $this configure -radius $radius
  if { [info exists o(radius)] } {
    $o(radius) SetValue $radius
  }
  set smudge [$node GetParameter Paint,smudge] 
  $this configure -smudge $smudge
  if { [info exists o(smudge)] } {
    [$o(smudge) GetWidget] SetSelectedState $smudge
  }

  $this preview
}

itcl::body PaintEffect::tearDownOptions { } {

  # call superclass version of tearDownOptions
  chain

  foreach w "radius smudge cancel" {
    if { [info exists o($w)] } {
      $o($w) SetParent ""
      pack forget [$o($w) GetWidgetName] 
    }
  }
}

itcl::body PaintEffect::paintFeedback {} {

  set renderer [$_renderWidget GetRenderer]

  if { $_paintCoordinates == "" } {
    foreach a $_feedbackActors {
      $renderer RemoveActor2D $a
      $a Delete
    }
    set _feedbackActors ""
    return
  }

  set numActors [llength $_feedbackActors]
  foreach xy [lrange $_paintCoordinates $numActors end] {
    set a [vtkActor2D New]
    lappend _feedbackActors $a
    $a SetMapper $o(mapper)
    eval $a SetPosition $xy
    set property [$a GetProperty]
    $property SetColor .7 .7 0
    $property SetOpacity .5
    $renderer AddActor2D $a
  }
}

itcl::body PaintEffect::paintAddPoint {x y} {

  lappend _paintCoordinates "$x $y"
  if { $delayedPaint } {
    paintFeedback 
  } else {
    $this paintApply
    $_layers(label,node) Modified
  }
}

itcl::body PaintEffect::paintApply {} {
  if { $_paintCoordinates != "" } {
    $this queryLayers 0 0
    EditorStoreCheckPoint $_layers(label,node)
  }
  foreach xy $_paintCoordinates {
    eval $this paintBrush $xy
  }
  set _paintCoordinates ""
  paintFeedback

  # TODO: workaround for new pipeline in slicer4
  # - editing image data of the calling modified on the node
  #   does not pull the pipeline chain
  # - so we trick it by changing the image data first
  $_layers(label,node) SetModifiedSinceRead 1
  set workaround 1
  if { $workaround } {
    if { ![info exists o(tempImageData)] } {
      set o(tempImageData) [vtkNew vtkImageData]
    }
    set imageData [$_layers(label,node) GetImageData]
    $_layers(label,node) SetAndObserveImageData $o(tempImageData)
    $_layers(label,node) SetAndObserveImageData $imageData
  } else {
    $_layers(label,node) Modified
  }
}


## Note: there is a pure-tcl implementation of the painting
##  in the subversion history - it is slower (of course) but
##  useful for debugging (and was remarkably terse)
itcl::body PaintEffect::paintBrush {x y} {

  #
  # paint with a brush that is circular in XY space 
  # (could be streched or rotate when transformed to IJK)
  # - make sure to hit ever pixel in IJK space 
  # - apply the threshold if selected
  #

  if { $_layers(label,node) == "" } {
    # if there's no label, we can't paint
    return
  }

  #
  # get the brush bounding box in ijk coordinates
  # - get the xy bounds
  # - transform to ijk
  # - clamp the bounds to the dimensions of the label image
  #
  set bounds [[$o(brush) GetPoints] GetBounds]
  set left [expr $x + [lindex $bounds 0]]
  set right [expr $x + [lindex $bounds 1]]
  set bottom [expr $y + [lindex $bounds 2]]
  set top [expr $y + [lindex $bounds 3]]

  set xyToIJK [[$_layers(label,logic) GetXYToIJKTransform] GetMatrix]
  set tlIJK [$xyToIJK MultiplyPoint $left $top 0 1]
  set trIJK [$xyToIJK MultiplyPoint $right $top 0 1]
  set blIJK [$xyToIJK MultiplyPoint $left $bottom 0 1]
  set brIJK [$xyToIJK MultiplyPoint $right $bottom 0 1]

  set dims [$_layers(label,image) GetDimensions]
  foreach v {i j k} c [lrange $tlIJK 0 2] d $dims {
    set tl($v) [expr int(round($c))]
    if { $tl($v) < 0 } { set tl($v) 0 }
    if { $tl($v) >= $d } { set tl($v) [expr $d - 1] }
  }
  foreach v {i j k} c [lrange $trIJK 0 2] d $dims {
    set tr($v) [expr int(round($c))]
    if { $tr($v) < 0 } { set tr($v) 0 }
    if { $tr($v) >= $d } { set tr($v) [expr $d - 1] }
  }
  foreach v {i j k} c [lrange $blIJK 0 2] d $dims {
    set bl($v) [expr int(round($c))]
    if { $bl($v) < 0 } { set bl($v) 0 }
    if { $bl($v) >= $d } { set bl($v) [expr $d - 1] }
  }
  foreach v {i j k} c [lrange $brIJK 0 2] d $dims {
    set br($v) [expr int(round($c))]
    if { $br($v) < 0 } { set br($v) 0 }
    if { $br($v) >= $d } { set br($v) [expr $d - 1] }
  }

  #
  # get the ijk to ras matrices 
  #
  set backgroundIJKToRAS [vtkMatrix4x4 New]
  $_layers(background,node) GetIJKToRASMatrix $backgroundIJKToRAS
  set labelIJKToRAS [vtkMatrix4x4 New]
  $_layers(label,node) GetIJKToRASMatrix $labelIJKToRAS


  set xyToRAS [$_sliceNode GetXYToRAS]
  set brushCenter [lrange [$xyToRAS MultiplyPoint $x $y 0 1] 0 2]
  set brushRadius $radius

  #
  # set up the painter class and let 'r rip!
  #
  set extractImage [vtkImageData New]
  set painter [vtkImageSlicePaint New]
  $painter SetBackgroundImage $_layers(background,image)
  $painter SetBackgroundIJKToWorld $backgroundIJKToRAS
  $painter SetWorkingImage $_layers(label,image)
  $painter SetWorkingIJKToWorld $labelIJKToRAS
  $painter SetTopLeft $tl(i) $tl(j) $tl(k)
  $painter SetTopRight $tr(i) $tr(j) $tr(k)
  $painter SetBottomLeft $bl(i) $bl(j) $bl(k)
  $painter SetBottomRight $br(i) $br(j) $br(k)
  eval $painter SetBrushCenter $brushCenter
  $painter SetBrushRadius $brushRadius
  $painter SetPaintLabel [EditorGetPaintLabel]
  $painter SetPaintOver $paintOver
  $painter SetThresholdPaint $paintThreshold
  $painter SetThresholdPaintRange $paintThresholdMin $paintThresholdMax
  $painter Paint

  $extractImage Delete
  $painter Delete
  $labelIJKToRAS Delete
  $backgroundIJKToRAS Delete


  return
}

proc PaintEffect::AddPaint {} {
  foreach sw [itcl::find objects -class SliceSWidget] {
    set sliceGUI [$sw cget -sliceGUI]
    if { [info command $sliceGUI] != "" } {
      PaintEffect #auto [$sw cget -sliceGUI]
    }
  }
}

proc PaintEffect::RemovePaint {} {
  foreach pw [itcl::find objects -class PaintEffect] {
    itcl::delete object $pw
  }
}

proc PaintEffect::TogglePaint {} {
  if { [itcl::find objects -class PaintEffect] == "" } {
    PaintEffect::AddPaint
  } else {
    PaintEffect::RemovePaint
  }
}

proc PaintEffect::ConfigureAll { args } {
  foreach pw [itcl::find objects -class PaintEffect] {
    eval $pw configure $args
  }
}
