setlocal

set os=windows
set arch=x86_64
set file=onnxruntime.lib
set version=v1.14.1-neuralnote.1
set dir=onnxruntime-%version%-%os%-%arch%
set archive=%dir%.tar.gz

:: logical OR is missing in batch o_O!
@set fetch=false
@if not exist "Lib/ModelData/features_model.ort" set fetch=true
@if not exist "%file%" set fetch=true
@if exist "%archive%" set fetch=true

if "%fetch%" == "true" (
	if not exist "%archive%" (
		curl -fSLO "https://github.com/tiborvass/libonnxruntime-neuralnote/releases/download/%version%/%archive%" || exit /b
	)
	del /s /f /q ThirdParty\%dir% ThirdParty\onnxruntime
	rmdir /s /q ThirdParty\%dir% ThirdParty\onnxruntime
	tar -C ThirdParty -x -v -f %archive%
	rename ThirdParty\%dir% onnxruntime
	move ThirdParty\onnxruntime\model.with_runtime_opt.ort Lib\ModelData\features_model.ort
	del %archive%
)

wmic get cpu NumberOfLogicalProcessors | findstr /V NumberOfLogicalProcessors > ncpus
set /p ncpus=<ncpus
del ncpus

set config=Release
cmake -S . -B build -DBUILD_UNIT_TESTS=ON || exit /b
cmake --build build -j %ncpus% --config %config% --target NeuralNote_Standalone --target UnitTests --target NeuralNote_VST3 --target NeuralNote_AU

.\build\Tests\UnitTests_artefacts\Release\UnitTests.exe || exit /b

echo
echo "Run .\build\NeuralNote_artefacts\Release\Standalone\NeuralNote.exe"
