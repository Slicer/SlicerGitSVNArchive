NOTE:  THIS REPOSITORY IS FOR IMPROVEMENTS SLATED TO BE INTEGRATED INTO THE
       MAIN SLICER REPOSITORY AFTER November 2012

THIS IS FOR AGGRESSIVE DEVEMPMENT TO MOVE TOWARDS ITKv4 and UPDATE MANY
PACKAGES THAT ARE WOEFULLY OUT OF DATE.

Goals:

Move to ITKv4
Improve integration of DCMTK
Remove legacy modules (This was supposed to have been done January 2011)
Update python infrastructure to allow for pip installs of external modules        into the Slicer environment
    - Remove build dependancy on numpy (force use of pip, and make this
      an end user issue)
    - Remove build dependancy on scipy (force use of pip, and make this
      an end user issue)
Allow building with QT 4.8.2
Allow building with new version of VTK that has many bug fixes identified
     by the clang compiler
Allow building all of Slicer with the clang compiler.



IMPLEMENTATION RULES:

This repository does NOT follow standard git layout, because it is NOT a
standard repository.  This respository is intended allow rapid development
of topics that originate from the main Slicer/Slicer repository, or from
other topics in theis repository.

1) There is no "master" branch.  The master branch shall always be considered from the Slicer/Slicer master repository branch. There will be a slicer42_master branch that will be periodically kept in sync with the Slicer/Slicer master branch.

2) There will be a "hjmjohnson_slicer43" branch that will be exclusively mainted exclusively by Hans Johnson, and used as an integration branch for preparation of pushing onto the main slicer respoitory after November 2012. This branch will maintain a linear (svn like) history which means that it will be rebased off of the "slicer42_master" branch periodically.  As your topics mature, we will "cherry-pick" them into the "hjmjohnson_slicer43" branch upon request.  PLEASE DO NOT MERGE ITEMS INTO THIS BRANCH!

3) All other developers are encouraged to make their own topic branches.

=====================
This entire repository will be removed from GitHub in December 2012.  Do NOT reference this temporary repository in external packages.

=====================
git remote add slicer42_upstream https://github.com/Slicer/Slicer.git
git fetch slicer42_upstream
git checkout slicer42_master
git reset slicer42_upstream/master --hard          # <--- NOTE: This is a hard reset and will overwirte any local changes
git push origin slicer42_master:slicer42_master -f # <--- NOTE: FORCE this will overwrite any changes that merged into this branch!

=====================
git checkout hjmjohnson_slicer43
git rebase slicer42_upstream/master
git push origin hjmjohnson_slicer43:hjmjohnson_slicer43

