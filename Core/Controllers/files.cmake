SET(CPP_FILES
  mitkBaseController.cpp
  mitkCameraController.cpp
  mitkCameraRotationController.cpp
  mitkFocusManager.cpp
  mitkLimitedLinearUndo.cpp
  mitkMovieGenerator.cpp
  mitkMultiStepper.cpp
  mitkOperationEvent.cpp
  mitkProgressBar.cpp
  mitkRenderingManager.cpp
  mitkReportGenerator.cpp
  mitkSliceNavigationController.cpp
  mitkSlicesCoordinator.cpp
  mitkSlicesRotator.cpp
  mitkStatusBar.cpp
  mitkStepper.cpp
  mitkUndoController.cpp
  mitkUndoModel.cpp
  mitkVerboseLimitedLinearUndo.cpp
  mitkVtkInteractorCameraController.cpp
  mitkVtkLayerController.cpp
  # VtkQRenderWindowInteractor.cpp
)
IF(WIN32)
  SET(CPP_FILES ${CPP_FILES} mitkMovieGeneratorWin32.cpp)
ENDIF(WIN32)

IF(MITK_USE_OPEN_CV)
  SET(CPP_FILES ${CPP_FILES}  mitkMovieGeneratorOpenCV.cpp)
ENDIF(MITK_USE_OPEN_CV)

