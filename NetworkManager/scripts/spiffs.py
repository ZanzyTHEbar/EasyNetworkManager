# make a folder in the current directory called `data`
# copy the contents from the `data` folder in the current directory to the `data` folder in the `spiffs` folder in the current directory
import os
import shutil


def mkdir(path):
    try:
        os.mkdir(path)
    except OSError:
        print("Creation of the directory %s failed" % path)
    else:
        print("Successfully created the directory %s " % path)


def copytree(src, dst, symlinks=False, ignore=None):
    try:
        shutil.copytree(src, dst, symlinks, ignore)
    except OSError:
        print("Copy of the directory %s failed" % src)
    else:
        print("Successfully copied the directory %s " % src)


mkdir("../../../../../data")
copytree("../data", "../../../../../data")
