==============
Segment editor
==============

This is a module for segmentation of volumes. Segmentations (also known as contouring) delineate structures of interest. Some of the tools mimic a painting interface like photoshop or gimp, but work on 3D arrays of voxels rather than on 2D pixels. This module is a new, improved version of the old :ref:`module_editor` module. The Segment Editor contains many of the same functionalities and many more. New features include: overlapping segments, display in both 2D and 3D views, per-segment visualization options, editing in 3D views, create segmentation by interpolating or extrapolating segmentation on a few slices, editing on slices in any orientation.

It is important to remember that Segment Editor does not edit labelmap volumes, as Editor does. Segment editor creates segmentations, which can do many things that labelmap volumes cannot (overlapping contours, show/hide segments individually, show in 3D view, etc). Segmentations can be converted to labelmap volumes and models using the Import/Export section of :ref:`_module_segment_editor` module.

Keyboard shortcuts
------------------

The following keyboard shortcuts are active when you are in the Editor module.  They are intended to allow two-handed editing, where on hand is on the mouse and the other hand uses the keyboard to switch modes.

+-------------------------------------+-----------------------------------------+
| Key                                 | Operation                               |
+=====================================+=========================================+
| ``left arrow``                      | move to previous slice                  |
+-------------------------------------+-----------------------------------------+
| ``right arrow``                     | move to next slice                      |
+-------------------------------------+-----------------------------------------+
| ``Shift`` + ``mouse move``          | scroll slices to mouse location         |
+-------------------------------------+-----------------------------------------+
| ``Ctrl`` + ``mouse wheel``          | zoom image in/out                       |
+-------------------------------------+-----------------------------------------+
| ``q``                               | select previous segment                 |
+-------------------------------------+-----------------------------------------+
| ``w``                               | select next segment                     |
+-------------------------------------+-----------------------------------------+
| ``z``                               | undo                                    |
+-------------------------------------+-----------------------------------------+
| ``y``                               | redo                                    |
+-------------------------------------+-----------------------------------------+
| ``esc``                             | unselect effect                         |
+-------------------------------------+-----------------------------------------+
| ``space``                           | toggle between last two active effects  |
+-------------------------------------+-----------------------------------------+
| ``1``, ``2``, ... ``0``             | select effect (1-10)                    |
+-------------------------------------+-----------------------------------------+
| ``Shift`` + ``1``, ``2``, ... ``0`` | select effect (11-20)                   |
+-------------------------------------+-----------------------------------------+

Panels and their use
--------------------

- Segmentation: Choose the segmentation to edit
- Master volume: Choose the volume to segment. The master volume that is selected the very first time after the segmentation is created is used to determine the segmentation's labelmap representation geometry (resolution, axis directions, origin). The master volume is used by all editor effects that uses intensity of the segmented volume (e.g., thresholding, level tracing). The master volume can be changed at any time during the segmentation process. Note: Currently the only way to change geometry is to create a new segmentation, set its geometry, and then import segments from another segmentation.
- Add segment: Add a new segment to the segmentation and select it.
- Remove segment: Select the segment you would like to delete then click Remove segment to delete from the segmentation.
- Create Surface: Display your segmentation in the 3D Viewer. This is a toggle button. When turned on the surface is created and updated automatically as the user is segmenting. When turned off, the conversion is not ongoing so the segmentation process is faster. To change surface creation parameters: go to Segmentations module, click Update button in Closed surface row in Representations section, click Binary labelmap -> Closed surface line, double-click on value column to edit a conversion parameter value. Setting Smoothing factor to 0 disables smoothing, making updates much faster. Set Smoothing factor to 0.1 for weak smoothing and 0.5 or larger for stronger smoothing.
- Segments table: Displays list of all segments.
  - Eye icon: Toggle segment's visibility. To customize visualization: either open the slice view controls (click on push-pint and double-arrow icons at the top of a slice viewer) or go to Segmentations module.
  - Color swatch: set color and assign segment to standardized terminology.
- Effects: Select the desired effect here. See below for more information about each effect.
- Options: Options for the selected effect will be displayed here.
- Undo/Redo: The module saves state of segmentation before each effect is applied. This is useful for experimentation and error correction. By default the last 10 states are remembered.
- Masking: These options allow you to define the editable areas and whether or not certain segments can be overwritten.
  - Editable area: Changes will be limited to the selected area. This can be used for drawing inside a specific region or split a segment into multiple segments.
  - Editable intensity range: Changes wil be limited to areas where the master volume's voxels are in the selected intensity range. It is useful when locally an intensity threshold separates well between different regions. Intensity range can be previewed by using Threshold effect.
  - Overwrite other segments: Select which segments will be overwritten rather than overlapped.
    - All segments: Segment will not overlap.
    - Visible segments: Visible segments will not overlap with each other. Hidden segments will not be overwritten by changes done to visible segments.
    - None: Segments can overlap. Changing one segment will not change any other.

Effects
-------

Effects operate either by clicking the Apply button in the effect options section or by clicking and/or dragging in slice or 3D views.

|paint| Paint 
~~~~~~~~~~~~~

.. |paint| image:: images/module_segment_editor/paint.png

- Pick the radius (in millimeters) of the brush to apply
- Left click to apply single circle
- Left click and drag to fill a region
- A trace of circles is left which are applied when the mouse button is released
- Sphere mode applies the radius to slices above and below the current slice.

+-----------------------------+-----------------------------------------+
| Key                         | Operation                               |
+=============================+=========================================+
| ``Shift`` + ``mouse wheel`` | increase/decrease brush size            |
+-----------------------------+-----------------------------------------+
| ``-``                       | shrink brush radius by 20%              |
+-----------------------------+-----------------------------------------+
| ``+``                       | grow brush radius by 20%                |
+-----------------------------+-----------------------------------------+

|draw| Draw
~~~~~~~~~~~~~~~~~~

.. |draw| image:: images/module_segment_editor/draw.png

- Left click to lay individual points of an outline
- Left drag to lay down a continuous line of points
- Right click to apply segment

+---------------------+-----------------------------------------+
| Key                 | Operation                               |
+=====================+=========================================+
| ``x``               | delete the last point added             |
+---------------------+-----------------------------------------+
| ``a``               | apply segment                           |
+---------------------+-----------------------------------------+

|erase| Erase
~~~~~~~~~~~~~~~~~~~~

.. |erase| image:: images/module_segment_editor/erase.png

Same as the Paint effect, but the highlighted regions are removed from the selected segment instead of added.

If Masking / Editable area is set to a specific segment then the highlighted region is removed from selected segment *and* added to the masking segment. This is useful when a part of a segment has to be separated into another segment.

+-----------------------------+-----------------------------------------+
| Key                         | Operation                               |
+=============================+=========================================+
| ``Shift`` + ``mouse wheel`` | increase/decrease brush size            |
+-----------------------------+-----------------------------------------+
| ``-``                       | shrink brush radius by 20%              |
+-----------------------------+-----------------------------------------+
| ``+``                       | grow brush radius by 20%                |
+-----------------------------+-----------------------------------------+

|level_tracing| Level Tracing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |level_tracing| image:: images/module_segment_editor/level_tracing.png

- Moving the mouse defines an outline where the pixels all have the same background value as the current background pixel
- Clicking the left mouse button applies that outline to the label map

|grow_from_seeds| Grow from seeds
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |grow_from_seeds| image:: images/module_segment_editor/grow_from_seeds.png

Draw segment inside each anatomical structure. This method will start from these "seeds" and grow them to achieve complete segmentation.

- Initialize: Click this button after initial segmentation is completed (by using other editor effects). Initial computation may take more time than subsequent updates. Master volume, auto-complete method, segmentation extent will be locked after initialization, therefore if any of these have to be changed then click Cancel and initialize again.
- Update: Update completed segmentation based on changed inputs.
- Auto-update: activate this option to automatically updating result preview when segmentation is changed.
- Cancel: Remove result preview. Seeds are kept unchanged, so parameters can be changed and segmentation can be restarted by clicking Initialize.
- Apply: Overwrite seeds segments with previewed results.

Notes:

- Only visible segments are used by this effect.
- At least two segments are required.
- If parts of a segment is removed (and not overwritten by another segment) then it is recommended to cancel and initialize again.
- The method uses grow-cut algorithm: Liangjia Zhu, Ivan Kolesov, Yi Gao, Ron Kikinis, Allen Tannenbaum. An Effective Interactive Medical Image Segmentation Method Using Fast GrowCut, International Conference on Medical Image Computing and Computer Assisted Intervention (MICCAI), Interactive Medical Image Computing Workshop, 2014.

|fill_between_slices| Fill between slices
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |fill_between_slices| image:: images/module_segment_editor/fill_between_slices.png

Create complete segmentation on selected slices using any editor effect. You can skip any number of slices between segmented slices. This method will fill the skipped slices by interpolating between segmented slices.

- Initialize: Click this button after initial segmentation is completed (by using other editor effects). Initial computation may take more time than subsequent updates. Master volume, auto-complete method, segmentation extent will be locked after initialization, therefore if any of these have to be changed then click Cancel and initialize again.
- Update: Update completed segmentation based on changed inputs.
- Auto-update: activate this option to automatically updating result preview when segmentation is changed.
- Cancel: Remove result preview. Seeds are kept unchanged, so parameters can be changed and segmentation can be restarted by clicking Initialize.
- Apply: Overwrite seeds segments with previewed results.

Notes:

- Only visible segments are used by this effect.
- The method does not use the master volume, only the shape of the specified segments.
- The method uses ND morphological contour interpolation algorithm. See details here: http://insight-journal.org/browse/publication/977

|threshold| Threshold
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |threshold| image:: images/module_segment_editor/threshold.png

Use Threshold to determine a threshold range and save results to selected segment or use it as Editable intensity range.

|margin| Margin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |margin| image:: images/module_segment_editor/margin.png

Grows or shrinks the selected segment by the specified margin.

|smoothing| Smoothing
~~~~~~~~~~~~~~~~~~~~~

.. |smoothing| image:: images/module_segment_editor/smoothing.png

Smoothes selected labelmap or all labelmaps (only for Joint smoothing method).
  
|scissors| Scissors
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |scissors| image:: images/module_segment_editor/scissors.png

Clip segments to the specified region or fill regions of a segment (typically used with masking). Regions can be drawn on both slice view or 3D views.

- Left click to start drawing (free-form or rubber band circle or rectangle)
- Release button to apply

|islands| Identify islands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |islands| image:: images/module_segment_editor/islands.png


Use this tool to create a unique segment for each connected region of the selected segment. Connected regions are defined as groups of pixels which touch each other but are surrounded by zero valued voxels.

- Fully connected: If checked then only voxels that share a face are counted as connected; if unchecked then voxels that touch at an edge or a corner are considered connected.
- Minimum size: All regions that have less than this number of voxels will be deleted.

|logical_operators| Logical operators
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. |logical_operators| image:: images/module_segment_editor/logical_operators.png

Apply Boolean operators to selected segment or combine segments.


Hints
-----

[[Image:Selection 139.png|thumb|400px|right|Use of the Label Outline feature (below) vs the default view (above).  Note that the label outlines appear faint in the lower image -- zoom in by clicking on the image to see them as they actually appear in Slicer.]]
- A large radius paint brush with threshold painting is often a very fast way to segment anatomy that is consistently brighter or darker than the surrounding region, but partially connected to similar nearby structures (this happens a lot).
- Use the slice viewer menus to control the label map opacity and display mode (to show outlines only or full volume).

Limitations
-----------

- Threshold will not work with non-scalar volume background volumes.
- Mouse wheel can be used to move slice through volume, but on some platforms (mac) it may move more than one slice at a time.

Similar Modules
---------------

- :ref:`module_editor` is the predecessor of this module. Segment Editor will eventually replace the Editor module.

Information for Developers
--------------------------

TODO

Contributors
------------

- Contributors: Csaba Pinter (PerkLab, Queen's University), Andras Lasso (PerkLab, Queen's University), Steve Pieper (Isomics Inc.), Wendy Plesniak (SPL, BWH), Ron Kikinis (SPL, BWH), Jim Miller (GE)
- Contact: Csaba Pinter, csaba.pinter@queensu.ca; Andras Lasso, lasso@queensu.ca

Acknowledgements
----------------

This module is partly funded by an Applied Cancer Research Unit of Cancer Care Ontario with funds provided by the Ministry of Health and Long-Term Care and the Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO) to provide free, open-source toolset for radiotherapy and related image-guided interventions.
The work is part of the `National Alliance for Medical Image Computing <http://www.na-mic.org/>`_ (NA-MIC), funded by the National Institutes of Health through the NIH Roadmap for Medical Research, Grant U54 EB005149.

+------------------+----------------+--------------+-------------+
|  |isomics_logo|  |  |namic_logo|  |  |nac_logo|  |  |ge_logo|  |
+------------------+----------------+--------------+-------------+
