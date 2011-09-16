proc Get_DEMONS_Installation_Path { } {
    set REGISTRATION_PACKAGE_FOLDER [Get_Installation_Path "LogDomainDemonsRegistration-0.0.5-Source/bin" "DemonsRegistration"]
    return $REGISTRATION_PACKAGE_FOLDER
}


# ----------------------------------------------------------------------------
proc DEMONSRegistration { fixedVolumeNode movingVolumeNode outVolumeNode backgroundLevel deformableType affineType} {
    variable SCENE
    variable LOGIC
    variable REGISTRATION_PACKAGE_FOLDER
    variable mrmlManager

    # Do not get rid of debug mode variable - it is sometimes very helpful !
    set DEMONS_DEBUG_MODE 0

    if { $DEMONS_DEBUG_MODE } {
        $LOGIC PrintText ""
        $LOGIC PrintText "DEBUG: ==========DEMONSRegistration DEBUG MODE ============="
        $LOGIC PrintText ""
    }

    $LOGIC PrintText "TCL: =========================================="
    $LOGIC PrintText "TCL: == Image Alignment CommandLine: $deformableType "
    $LOGIC PrintText "TCL: =========================================="

    # check arguments

    if { $fixedVolumeNode == "" || [$fixedVolumeNode GetImageData] == "" } {
        PrintError "DEMONSRegistration: fixed volume node not correctly defined"
        return ""
    }

    if { $movingVolumeNode == "" || [$movingVolumeNode GetImageData] == "" } {
        PrintError "DEMONSRegistration: moving volume node not correctly defined"
        return ""
    }

    if { $outVolumeNode == "" } {
        PrintError "DEMONSRegistration: output volume node not correctly defined"
        return ""
    }

    set fixedVolumeFileName [WriteDataToTemporaryDir $fixedVolumeNode Volume]
    if { $fixedVolumeFileName == "" } {
        # remove files
        return ""
    }
    set RemoveFiles "$fixedVolumeFileName"


    set movingVolumeFileName [WriteDataToTemporaryDir $movingVolumeNode Volume]
    if { $movingVolumeFileName == "" } {
        #remove files
        return ""
    }
    set RemoveFiles "$RemoveFiles $movingVolumeFileName"


    set outVolumeFileName [CreateTemporaryFileNameForNode $outVolumeNode]
    if { $outVolumeFileName == "" } {
        #remove files
        return ""
    }
    set RemoveFiles "$RemoveFiles $outVolumeFileName"

    ## DEMONS specific arguments

    set CMD "$REGISTRATION_PACKAGE_FOLDER/DemonsRegistration"


    set CMD "$CMD -f \"$fixedVolumeFileName\""
    set CMD "$CMD -m \"$movingVolumeFileName\""

    if { $affineType == [$mrmlManager GetRegistrationTypeFromString RegistrationTest] } {
        set CMD "$CMD -s 1.5 -i 20x5x0"
    } elseif { $affineType == [$mrmlManager GetRegistrationTypeFromString RegistrationFast] } {
        set CMD "$CMD -s 1.0 -i 30x20x10"
    } elseif { $affineType == [$mrmlManager GetRegistrationTypeFromString RegistrationSlow] } {
        set CMD "$CMD -s 1.5 -i 50x30x10"
    } else {
        PrintError "DEMONSRegistration: Unknown deformableType: $deformableType"
        return ""
    }


    # affine
    set outLinearTransformFileName [CreateFileName "ITKDeformationField"]

    set outTransformFileName $outLinearTransformFileName

    set CMD "$CMD -o \"$outVolumeFileName\""
    set CMD "$CMD -O \"$outLinearTransformFileName\""

    $LOGIC PrintText "TCL: Executing $CMD"
    catch { eval exec $CMD } errmsg
    $LOGIC PrintText "TCL: $errmsg"

    ## Read results back to scene
    if { [ReadDataFromDisk $outVolumeNode $outVolumeFileName Volume] == 0 } {
        if { [file exists $outVolumeFileName] == 0 } {
            set outTransformDirName ""
            PrintError "DEMONSRegistration: out volume file doesn't exists"
        }
    }

    if { [file exists $outTransformFileName] == 0 } {
        set outTransformFileName ""
        PrintError "DEMONSRegistration: out transform file doesn't exists"
    }

    foreach NAME $RemoveFiles {
        #file delete -force $NAME
    }

    # Remove Transformation from image
    $movingVolumeNode SetAndObserveTransformNodeID ""
    $SCENE Edited

    # return transformation directory name or ""
    puts "outTransformFileName: $outTransformFileName"
    return $outTransformFileName
}

