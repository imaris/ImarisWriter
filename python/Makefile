PYTHON=/c/Python37_x64/python.exe
WHL_FILE=PyImarisWriter_IMARIS-0.7-py3-none-any.whl

packages:
	echo "Create build"
	${PYTHON} -m build

uninstall:
	echo "Uninstalling PyImarisWriter"
	${PYTHON} -m pip uninstall PyImarisWriter

install:
	echo "Installing from wheel"
	${PYTHON} -m pip install dist/${WHL_FILE}	

install_tar:
	echo "Installing from tar file"
	${PYTHON} -m pip install dist/PyImarisWriter-1.0.tar.gz

upload_to_testpypi:
	echo "Installing from tar file"
	${PYTHON} -m twine upload --repository testpypi dist/${WHL_FILE}	
	
clean:
	echo "rm dist and build"
	rm -rf dist build

