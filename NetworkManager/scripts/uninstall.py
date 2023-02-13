# delete the `data` folder in the current directory

import os


def rmdir(path):
    try:
        os.rmdir(path)
    except OSError:
        print("Deletion of the directory %s failed" % path)
    else:
        print("Successfully deleted the directory %s " % path)


rmdir("../../../../../data")