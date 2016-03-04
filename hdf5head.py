"""Short script to print the header from a Gadget-HDF5 formatted snapshot."""

from __future__ import print_function
from optparse import OptionParser
import h5py
parser = OptionParser()
parser.add_option("-f", "--file", dest="filename",
                  help="File to read", metavar="FILE")
parser.add_option("-v", "--verbose",
                  action="store_true", dest="verbose", default=False,
                  help="Label each quantity")

(options, args) = parser.parse_args()
filename = options.filename
if filename is None:
    raise RuntimeError("You must specify a file with the -f option")

verbose=options.verbose
#Try some other names if this one fails
if not h5py.is_hdf5(filename):
    file2 = filename+'.0.hdf5'
    if not h5py.is_hdf5(file2):
        raise RuntimeError("Neither {0} nor {1} are files".format(filename,file2))
    filename=file2

f=h5py.File(filename,'r')
for (k,v) in f["Header"].attrs.items():
    if verbose:
        print(k,"=",v)
    else:
        print(v)
if verbose:
    print(list(f.keys()))
    if f["Header"].attrs["NumPart_ThisFile"][0] > 0:
        print(list(f['PartType0'].keys()))
    if f["Header"].attrs["NumPart_ThisFile"][1] > 0:
        print(list(f['PartType1'].keys()))

f.close()
